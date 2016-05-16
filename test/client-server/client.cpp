// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright © 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#define _HAS_EXCEPTIONS 0

#include "client.h"

#include "socket_wrapper.h"
#include "message_types.h"

#include <winsock.h>

//#include <D3dx9.h>

#include <vector>
#include <assert.h>

namespace
{
	int ConnectServerSocket(SOCKET socket)
	{
		std::string commandLineRemoteHostName;
		LPCSTR remoteHostName = "localhost";	// default

		// Parse the command-line and look for host address
		int nNumArgs;
		LPWSTR* pstrArgList = CommandLineToArgvW( GetCommandLineW(), &nNumArgs );
		for( int iArg = 1; iArg < nNumArgs; iArg++ )
		{
			LPWSTR arg = pstrArgList[iArg];
			if( *arg == L'/' || *arg == L'-' )
			{
				++arg;
				LPCWSTR hostAddrKeyword = L"hostaddr:";
				if(NULL != wcsstr(arg,hostAddrKeyword)) {

					LPCWSTR hostAddrValue = arg + wcslen(hostAddrKeyword);

					// Convert the host address
					size_t char_count;
					if(0 == wcstombs_s(&char_count,NULL,0,hostAddrValue,0)) {
						commandLineRemoteHostName.resize(char_count);
						wcstombs_s(&char_count,&commandLineRemoteHostName[0],char_count+1,hostAddrValue,char_count);
						remoteHostName = commandLineRemoteHostName.c_str();
					}

				}
			}
		}
		LocalFree( pstrArgList );

		// Host lookup
		hostent* host = gethostbyname(remoteHostName);
		if(0 == host) {
			printf("ERROR: Failed to lookup host %s\n",remoteHostName);
			return -1;
		}

		if(host->h_addrtype != AF_INET) {
			printf("ERROR: host %s is not IPv4\n",remoteHostName);
			return -1;
		}

		if(host->h_length != sizeof(in_addr)) {
			printf("ERROR: host address for %s is not a sockaddr_in\n",remoteHostName);
			return -1;
		}

		if(NULL == host->h_addr_list) {
			printf("ERROR: host address for %s is empty\n",remoteHostName);
			return -1;
		}

		in_addr* pAddr = (in_addr*)*(host->h_addr_list);
		if(NULL == pAddr) {
			printf("ERROR: host address for %s is NULL\n",remoteHostName);
			return -1;
		}

		// Connect to host
		sockaddr_in sock_addr;
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_port = htons(ServerPort);
		sock_addr.sin_addr = *pAddr;

		if(connect(socket,(sockaddr*)&sock_addr,sizeof(sock_addr))) {
			printf("ERROR: failed to connect to host %s\n",remoteHostName);
			return -1;
		}

		return 0;
	}

	void CleanupServerSocket(SOCKET socket)
	{
		if(socket != INVALID_SOCKET) {
			closesocket(socket);
			WSACleanup();
		}
	}

	const int kNumRemoteMarkerRequestsInFlight = 3;	// Don't flood the client-server pipe
}

struct ClientImpl
{
	ClientImpl(SOCKET_TYPE platformSocket) :
		m_socket(platformSocket),
		m_sockwrap(platformSocket),
		m_LastWallclockTime(0),
		m_QPFTicksPerSec(0),
		m_simulationTime(0.0),
		m_pMarkerPositions(NULL),
		m_numMarkerPositions(0),
		m_numRemoteMarkerRequestsInFlight(0)
	{
		InitWallclockTimer();
	}

	~ClientImpl()
	{
		CleanupServerSocket(m_socket);
		if(m_pMarkerPositions) {
			delete [] m_pMarkerPositions;
		}
	}

	// Member functions
	void InitWallclockTimer();
	float GetElapsedWallclockTime();

	int DoOneTimeSyncToServer();

	void UpdateRemoteMarkerPositions(const gfsdk_float4* pPositions, int num_markers);

	// Member variables
	SOCKET_TYPE m_socket;
	SocketWrapper m_sockwrap;
	std::vector<int> m_recvMessages;

	LONGLONG m_LastWallclockTime;
	LONGLONG m_QPFTicksPerSec;

	double m_simulationTime;

	SimulationConfig simulationConfig;

	gfsdk_float4* m_pMarkerPositions;
	int m_numMarkerPositions;
	int m_numRemoteMarkerRequestsInFlight;
};

void ClientImpl::InitWallclockTimer()
{
	// Record tick frequency
    LARGE_INTEGER qwTicksPerSec;
    QueryPerformanceFrequency( &qwTicksPerSec );
    m_QPFTicksPerSec = qwTicksPerSec.QuadPart;

	// Record last-time, so that we are zero-based
    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );
	m_LastWallclockTime = qwTime.QuadPart;
}

float ClientImpl::GetElapsedWallclockTime()
{
    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );

	const float result = ( qwTime.QuadPart - m_LastWallclockTime ) / (float) m_QPFTicksPerSec;
	m_LastWallclockTime = qwTime.QuadPart;

	return result;
}

int ClientImpl::DoOneTimeSyncToServer()
{
	// Sync to 0 by default, in case we early-out for some reason
	m_simulationTime = 0.0;

	if(!m_sockwrap.is_usable()) {
		return -1;
	}

	// Use Christian's algorithm to initialise time
	int msg = MessageTypeID_ClientRequestTimeFromServer;
	m_sockwrap.enqueue_send_message(&msg, sizeof(msg));

    // Zero the 'last time', so that the next call will give us the time delta
	GetElapsedWallclockTime();

	// Hey look, this is a busy wait - intentional, we want to measure the intrinsic latency on the round trip,
	// any extrinsic latency is therefore undesirable
	while(!m_sockwrap.has_recv_message() && m_sockwrap.is_usable()) {
		m_sockwrap.pump();
	}
	
	if(m_sockwrap.has_recv_message())
	{
		const float round_trip_time = GetElapsedWallclockTime();	// Note that the next elapsed will be delta'd
																	// relative to this moment, when we did the sync
																	// which is as it should be

		void* raw_msg = m_sockwrap.recv_message();
		int* pMsgType = (int*)raw_msg;
		switch(*pMsgType)
		{
		case MessageTypeID_ServerSendTimeToClient:
			{
				Message<double>* pMsg = (Message<double>*)raw_msg;
				m_simulationTime = pMsg->payload + 0.5 * round_trip_time;	// Christian's algo1
				m_sockwrap.consume_recv_message();
			}
			break;
		}
	}
	
	// Next, request simulation config
	if(m_sockwrap.is_usable())
	{
		msg = MessageTypeID_ClientRequestConfigFromServer;
		m_sockwrap.enqueue_send_message(&msg, sizeof(msg));

		// A less busy wait
		m_sockwrap.pump();
		while(!m_sockwrap.has_recv_message() && m_sockwrap.is_usable()) {
			Sleep(1);
			m_sockwrap.pump();
		}

		if(m_sockwrap.has_recv_message())
		{
			void* raw_msg = m_sockwrap.recv_message();
			int* pMsgType = (int*)raw_msg;
			switch(*pMsgType)
			{
			case MessageTypeID_ServerSendConfigToClient:
				{
					// We got config, apply it
					Message<SimulationConfig>* pMsg = (Message<SimulationConfig>*)raw_msg;
					simulationConfig = pMsg->payload;
					m_sockwrap.consume_recv_message();
				}
				break;
			}
		}
	}

	return m_sockwrap.is_usable() ? 0 : -1;
}

void ClientImpl::UpdateRemoteMarkerPositions(const gfsdk_float4* pPositions, int num_markers)
{
	// Update markers allocation if necessary
	if(num_markers > m_numMarkerPositions) {
		delete [] m_pMarkerPositions;
		m_pMarkerPositions = new gfsdk_float4[num_markers];
	}
	m_numMarkerPositions = num_markers;

	for(int markerIx = 0; markerIx != num_markers; ++markerIx) {
		m_pMarkerPositions[markerIx] = pPositions[markerIx];
	}
	--m_numRemoteMarkerRequestsInFlight;
}

Client* Client::Create()
{
	// Start networking
	WSADATA wsData;
	if(WSAStartup(0x202,&wsData)) {
		printf("ERROR: WSAStartup failed\n");
		return NULL;
	}

	// Init server socket
	SOCKET serverSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(INVALID_SOCKET == serverSocket) {
		printf("ERROR: Failed to create server socket\n");
		WSACleanup();
		return NULL;
	}

	if(ConnectServerSocket(serverSocket)) {
		CleanupServerSocket(serverSocket);
		return NULL;
	}

	// We have a connected socket, sync it
	ClientImpl* pImpl = new ClientImpl(serverSocket);
	if(pImpl->DoOneTimeSyncToServer()) {
		delete pImpl;
		return NULL;
	}

	// All done, return wrapper
	Client* pResult = new Client();
	pResult->pImpl = pImpl;
	return pResult;
}

Client::~Client()
{
	delete pImpl;
}

void Client::Destroy(Client* pClient)
{
	if(pClient) {
		delete pClient;
	}
}

bool Client::IsUsable() const
{
	return pImpl->m_sockwrap.is_usable();
}

void Client::Tick()
{
	pImpl->m_simulationTime += pImpl->GetElapsedWallclockTime();

	while(SocketWrapper::PumpResult_Traffic == pImpl->m_sockwrap.pump())
	{
		while(pImpl->m_sockwrap.has_recv_message()) 
		{
			void* raw_msg = pImpl->m_sockwrap.recv_message();
			int* pMsgType = (int*)raw_msg;
			int msgType = *pMsgType;
			switch(msgType)
			{
			case MessageTypeID_ServerSendConfigToClient:
				{
					Message<SimulationConfig>* pMsg = (Message<SimulationConfig>*)raw_msg;
					pImpl->simulationConfig = pMsg->payload;
				}
				break;
			case MessageTypeID_ServerSendMarkersToClient:
				{
					// Markers incoming, update them
					struct Payload
					{
						int num_markers;
						int pad1;
						int pad2;
						int pad3;
						gfsdk_float4 marker_positions;
					};

					Message<Payload>* pMsg = (Message<Payload>*)raw_msg;
					pImpl->UpdateRemoteMarkerPositions(&pMsg->payload.marker_positions,pMsg->payload.num_markers);
				}
				break;
			}

			pImpl->m_sockwrap.consume_recv_message();

			pImpl->m_recvMessages.push_back(msgType);
		}
	}
}

bool Client::HasPendingMessage() const
{
	return !pImpl->m_recvMessages.empty();
}

int Client::ConsumePendingMessage()
{
	const int result = pImpl->m_recvMessages.front();
	pImpl->m_recvMessages.erase(pImpl->m_recvMessages.begin());
	return result;
}

const SimulationConfig& Client::GetSimulationConfig() const
{
	return pImpl->simulationConfig;
}

double Client::GetSimulationTime() const
{
	return pImpl->m_simulationTime;
}

bool Client::GetRemoteMarkerPositions(const gfsdk_float4*& pPositions, size_t& numMarkers) const
{
	if(NULL == pImpl->m_pMarkerPositions) {
		return false;
	}

	pPositions = pImpl->m_pMarkerPositions;
	numMarkers = pImpl->m_numMarkerPositions;

	return true;
}

void Client::RequestRemoteMarkerPositions(const gfsdk_float2* pMarkerCoords, size_t numMarkers)
{
	if(pImpl->m_sockwrap.is_usable() && pImpl->m_numRemoteMarkerRequestsInFlight < kNumRemoteMarkerRequestsInFlight)
	{
		// Send a request for marker positions
		struct Request
		{
			int num_markers;
			int pad;
			gfsdk_float2 marker_coords;
		};

		const size_t msg_alloc = sizeof(Message<Request>) + numMarkers * sizeof(gfsdk_float2);
		Message<Request>* pMessage = (Message<Request>*)malloc(msg_alloc);
		pMessage->messageTypeID = MessageTypeID_ClientRequestMarkersFromServer;
		pMessage->payload.num_markers = (int)numMarkers;
		memcpy(&pMessage->payload.marker_coords, pMarkerCoords, numMarkers * sizeof(pMessage->payload.marker_coords));

		pImpl->m_sockwrap.enqueue_send_message(pMessage, (unsigned int)msg_alloc);
		++pImpl->m_numRemoteMarkerRequestsInFlight;
		free(pMessage);
	}
}