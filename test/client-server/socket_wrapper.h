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

#ifndef _SOCKET_WRAPPER_H
#define _SOCKET_WRAPPER_H

struct SocketWrapperImpl;

#if defined(__linux__ ) || defined(__ORBIS__)
typedef int SOCKET_TYPE;
#else
typedef int socklen_t;
#if defined(_WIN64)
	typedef unsigned __int64 SOCKET_TYPE;
#else
	typedef unsigned int SOCKET_TYPE;
#endif
#endif

int init_networking();
int term_networking();
int set_non_blocking(SOCKET_TYPE s);
bool wouldblock();

/*********************************************************************************
 Super-simple wrapper class for doing send/receive on non-blocking sockets
*********************************************************************************/
class SocketWrapper
{
public:

	SocketWrapper(SOCKET_TYPE platformSocket);
	~SocketWrapper();

	bool is_usable() const;

	void enqueue_send_message(void* msgData, unsigned int msgLength);

	bool has_recv_message() const;
	unsigned int recv_message_length() const;
	void* recv_message() const;
	void consume_recv_message();

	enum PumpResult 
	{
		PumpResult_NoTraffic = 0,	// The internal send/recv queues are both empty, and there was no new traffic on this pump
		PumpResult_Traffic,
		PumpResult_Error
	};

	PumpResult pump();

private:

	SocketWrapperImpl* pImpl;
};

/*********************************************************************************
 Misc x-plat helpers...
*********************************************************************************/

////////////////////////////////////////////////////////////////////////////////
#if defined(__linux__) || defined(__ORBIS__)
////////////////////////////////////////////////////////////////////////////////
#define closesocket close
#define INVALID_SOCKET (-1)
////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#if defined(__ORBIS__)
////////////////////////////////////////////////////////////////////////////////
int gethostname(char* buff, int buffsize);
#define SOMAXCONN 128
////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////

#endif	// _MESSAGETYPES_H
