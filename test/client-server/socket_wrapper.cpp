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

#include "socket_wrapper.h"
#include <stdio.h>


////////////////////////////////////////////////////////////////////////////////
#if defined(__linux__) || defined(__ORBIS__)
////////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <errno.h>

bool wouldblock()
{
    return EWOULDBLOCK == errno || EAGAIN == errno;
}

int init_networking() { return 0; }
int term_networking() { return 0; }

#define SOCKET_ERROR (-1)

////////////////////////////////////////////////////////////////////////////////
#endif // __linux__ || __ORBIS__
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#if defined(__linux__)
////////////////////////////////////////////////////////////////////////////////

#include <netdb.h> 

int set_non_blocking(SOCKET_TYPE s)
{
    int flags = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}


////////////////////////////////////////////////////////////////////////////////
#elif defined(__ORBIS__)
////////////////////////////////////////////////////////////////////////////////

#include <libnetctl.h>
#include <string.h>

int set_non_blocking(SOCKET_TYPE s)
{
	const int nonzero = -1;
	return setsockopt(s,SCE_NET_SOL_SOCKET,SCE_NET_SO_NBIO,&nonzero,sizeof(nonzero));
}

int gethostname(char* buff, int buffsize)
{
	SceNetCtlInfo ci;
	if(sceNetCtlGetInfo(SCE_NET_CTL_INFO_DHCP_HOSTNAME,&ci))
	{
		return -1;
	}

	if(strlen(ci.dhcp_hostname) >= buffsize)
	{
		return -1;
	}

	if(0 == ci.dhcp_hostname[0])
	{
		// Effectively no name
		return -1;
	}

	strcpy(buff,ci.dhcp_hostname);

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
#else   // !__linux__ && !__ORBIS__
////////////////////////////////////////////////////////////////////////////////

#include <winsock2.h>

int set_non_blocking(SOCKET_TYPE s)
{
    u_long iMode=1;
    return ioctlsocket(s,FIONBIO,&iMode);
}

bool wouldblock()
{
	const int lse = WSAGetLastError();
	return WSAEWOULDBLOCK == lse;
}

int init_networking()
{
	// Start networking
	WSADATA wsData;
	if(WSAStartup(0x202,&wsData)) {
		fprintf(stderr,"ERROR: WSAStartup failed\n");
		return -1;
	}

    return 0;
}

int term_networking()
{
    WSACleanup();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////


#define _HAS_EXCEPTIONS 0
#include <vector>
#include <assert.h>

struct SocketWrapperImpl
{
	SOCKET_TYPE socket;
	std::vector<char> recvMessageBytes;
	std::vector<char> sendMessageBytes;

	bool hasError;
	bool isConnected;
};

SocketWrapper::SocketWrapper(SOCKET_TYPE platformSocket) :
	pImpl(new SocketWrapperImpl)
{
	pImpl->socket = platformSocket;
	pImpl->hasError = false;

	// Assume connected and error-free until we know otherwise
	pImpl->isConnected = true;

	// Set the socket to non-blocking mode
	if(set_non_blocking(pImpl->socket)) {
		fprintf(stderr,"ERROR: Failed to set socket to non-blocking\n");
		pImpl->hasError = true;
	}
}

SocketWrapper::~SocketWrapper()
{
	delete pImpl;
}

void SocketWrapper::enqueue_send_message(void* msgData, unsigned int msgLength)
{
	// We add in bytes for the message length, which makes the message longer...
	unsigned int modifiedMsgLength = sizeof(msgLength) + msgLength;

	pImpl->sendMessageBytes.insert(pImpl->sendMessageBytes.end(),(char*)&modifiedMsgLength,((char*)&modifiedMsgLength)+sizeof(modifiedMsgLength));
	pImpl->sendMessageBytes.insert(pImpl->sendMessageBytes.end(),(char*)msgData,((char*)msgData)+msgLength);
}

bool SocketWrapper::is_usable() const
{
	return pImpl->isConnected && !pImpl->hasError;
}

bool SocketWrapper::has_recv_message() const
{
	if(pImpl->recvMessageBytes.size() > sizeof(unsigned int)) {
		unsigned int* pMessageLength = (unsigned int*)&pImpl->recvMessageBytes[0];
		if(pImpl->recvMessageBytes.size() >= *pMessageLength) {
			return true;
		}
	}

	return false;
}

unsigned int SocketWrapper::recv_message_length() const
{
	assert(has_recv_message());

	unsigned int* pMessageLength = (unsigned int*)&pImpl->recvMessageBytes[0];
	return *pMessageLength;
}

void* SocketWrapper::recv_message() const
{
	assert(has_recv_message());
	return &pImpl->recvMessageBytes[sizeof(unsigned int)];
}

void SocketWrapper::consume_recv_message()
{
	assert(has_recv_message());

	unsigned int* pMessageLength = (unsigned int*)&pImpl->recvMessageBytes[0];
	pImpl->recvMessageBytes.erase(pImpl->recvMessageBytes.begin(),pImpl->recvMessageBytes.begin()+*pMessageLength);
}

SocketWrapper::PumpResult SocketWrapper::pump()
{
	PumpResult result = pImpl->recvMessageBytes.empty() ? PumpResult_NoTraffic : PumpResult_Traffic;

	// Always check recv
	char recvbuffer[256];
	int recvresult = recv(pImpl->socket, recvbuffer, sizeof(recvbuffer), 0);
	if(SOCKET_ERROR == recvresult) {
		if(!wouldblock()) {
			fprintf(stderr,"ERROR: Failed to recv client socket\n");
			pImpl->hasError = true;
			result = PumpResult_Error;
		}
	} else if(0 == recvresult) {
		pImpl->isConnected = false;
	} else {
		result = PumpResult_Traffic;
		pImpl->recvMessageBytes.insert(pImpl->recvMessageBytes.end(), recvbuffer, recvbuffer + recvresult);
	}

	// Only need to pump send when there is stuff to send
	if(!pImpl->sendMessageBytes.empty()) {
		result = PumpResult_Traffic;			// Just attempting to send qualifies as traffic
		int sendresult = send(pImpl->socket,(char*)&pImpl->sendMessageBytes[0],(int)pImpl->sendMessageBytes.size(),0);
		if(SOCKET_ERROR == sendresult) {
			if(!wouldblock()) {
				fprintf(stderr,"ERROR: Failed to send on client socket\n");
				pImpl->hasError = true;
				result = PumpResult_Error;
			}
		} else {
			pImpl->sendMessageBytes.erase(pImpl->sendMessageBytes.begin(),pImpl->sendMessageBytes.begin()+sendresult);
		}
	}

	return result;
}
