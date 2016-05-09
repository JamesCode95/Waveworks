// serverapp.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <math.h>

#if !defined(__ORBIS__)
#include <signal.h>
#endif

#if defined(__linux__) || defined(__ORBIS__)
#define TARGET_PLATFORM_NIXLIKE
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#else
#include <tchar.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#if defined(__linux__) 
#include <termios.h>
#include <ifaddrs.h>
#endif

#if defined(__ORBIS__) 
#include <sys/socket.h>
#include <net.h>
#include <libnetctl.h>
#include <kernel.h>
#endif

#ifdef _XBOX_ONE
#include <collection.h>
#endif

#if defined(__ORBIS__) 
#include <GFSDK_WaveWorks_Orbis.h>
#else
#include <GFSDK_WaveWorks.h>
#endif

#include "message_types.h"
#include "socket_wrapper.h"

#include "ThreadWrap.h"

volatile bool g_DoQuit = false;

#if defined(__ORBIS__) 
unsigned int sceLibcHeapExtendedAlloc = 1;  /* Switch to dynamic allocation */
size_t       sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT; /* no upper limit for heap area */
#endif

/*********************************************************************************
 PRINTF REDIRECTION ON CONSOLES
*********************************************************************************/
#ifdef _XBOX_ONE
#define printf(...) xbone_printf(__VA_ARGS__)

void xbone_printf(const char* fmt, ...)
{
	// Fwd. to debug spew
	va_list arg;
	va_start(arg, fmt);
	const int numChars = _vscprintf(fmt,arg)+1;
	const int bufferSize = (numChars) * sizeof(char);
	va_end(arg);

	char* pBuffer = new char[numChars];
	va_start(arg, fmt);
	_vsprintf_p(pBuffer,bufferSize,fmt,arg);
	va_end(arg);

	OutputDebugStringA(pBuffer);

	delete pBuffer;

	// Also fwd to stdout...
	va_list args;
	va_start (args, fmt);
	vprintf (fmt, args);
	va_end (args);
}

#endif

/*********************************************************************************
 TIMING
*********************************************************************************/

volatile double g_CoordinatedTime = 0.0;
CRITICAL_SECTION g_CoordinatedTimeCriticalSection;

// A somewhat vile WAR to make sure we can compile on older Linuxes, however in our defense
// we do check for _RAW support by relaxing down to no-_RAW if a call to clock_gettime() fails
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

static int g_clk_id = CLOCK_MONOTONIC_RAW;

double GetCoordinatedTime()
{
	EnterCriticalSection(&g_CoordinatedTimeCriticalSection);
	double result = g_CoordinatedTime;
	LeaveCriticalSection(&g_CoordinatedTimeCriticalSection);
	return result;
}

#ifdef TARGET_PLATFORM_NIXLIKE
timespec g_BaseTime = {0,0};

double CalcDiff(const timespec& xarg, const timespec& yarg)
{
    timespec x = xarg;
    timespec y = yarg;

    /* Perform the carry for the later subtraction by updating y. */
    if (x.tv_nsec < y.tv_nsec) {
     int numsec = (y.tv_nsec - x.tv_nsec) / 1000000000 + 1;
     y.tv_nsec -= 1000000000 * numsec;
     y.tv_sec += numsec;
    }
    if (x.tv_nsec - y.tv_nsec > 1000000000) {
     int numsec = (x.tv_nsec - y.tv_nsec) / 1000000000;
     y.tv_nsec += 1000000000 * numsec;
     y.tv_sec -= numsec;
    }

    /* Compute the time remaining to wait.
      tv_nsec is certainly positive. */
    timespec diff;
    diff.tv_sec = x.tv_sec - y.tv_sec;
    diff.tv_nsec = x.tv_nsec - y.tv_nsec;

    return (double) diff.tv_sec + 0.000000001 * (double) diff.tv_nsec;
}

void UpdateCoordinatedTime()
{
	EnterCriticalSection(&g_CoordinatedTimeCriticalSection);

    timespec currTime;
    clock_gettime(g_clk_id, &currTime);

	g_CoordinatedTime = CalcDiff(currTime,g_BaseTime);

	LeaveCriticalSection(&g_CoordinatedTimeCriticalSection);
}

void InitCoordinatedTime()
{
	InitializeCriticalSection(&g_CoordinatedTimeCriticalSection);

	EnterCriticalSection(&g_CoordinatedTimeCriticalSection);

	// Record base-time, so that we are zero-based
    if(clock_gettime(g_clk_id, &g_BaseTime))
    {
        // Fall back to no-_RAW if necessary
        g_clk_id = CLOCK_MONOTONIC;
        clock_gettime(g_clk_id, &g_BaseTime);
    }

	// Init coordinated time
	g_CoordinatedTime = 0.0;

	LeaveCriticalSection(&g_CoordinatedTimeCriticalSection);
}


#else // !__linux__ ...
LONGLONG g_BaseTime = 0;
LONGLONG g_QPFTicksPerSec = 0;

void UpdateCoordinatedTime()
{
	EnterCriticalSection(&g_CoordinatedTimeCriticalSection);

    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );

	g_CoordinatedTime = (double) ( qwTime.QuadPart - g_BaseTime ) / (double) g_QPFTicksPerSec;

	LeaveCriticalSection(&g_CoordinatedTimeCriticalSection);
}

void InitCoordinatedTime()
{
	InitializeCriticalSection(&g_CoordinatedTimeCriticalSection);

	EnterCriticalSection(&g_CoordinatedTimeCriticalSection);

	// Record tick frequency
    LARGE_INTEGER qwTicksPerSec;
    QueryPerformanceFrequency( &qwTicksPerSec );
    g_QPFTicksPerSec = qwTicksPerSec.QuadPart;

	// Record base-time, so that we are zero-based
    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );
	g_BaseTime = qwTime.QuadPart;

	// Init coordinated time
	g_CoordinatedTime = 0.0;

	LeaveCriticalSection(&g_CoordinatedTimeCriticalSection);
}

#endif // __linux__

/*********************************************************************************
 SIMULATION
*********************************************************************************/
SimulationConfig g_SimConfig;
int g_SimConfigVersion = 0;
CRITICAL_SECTION g_SimulationCritSec;
GFSDK_WaveWorks_Simulation_Params g_SimParams;
GFSDK_WaveWorks_Simulation_Settings g_SimSettings;
GFSDK_WaveWorks_SimulationHandle g_hSim = NULL;
GFSDK_WaveWorks_Simulation_Stats g_SimStats;
const float kSimulationTickTargetInterval = 0.01f;
const float kPrintStatsTargetInterval = 1.f;
float g_SimulationTickActualInterval = 0.f;
double g_LastSimulationTick = 0.0;
double g_LastPrintStats = 0.0;
bool g_EnablePrintStats = false;

void ReleaseSimulation()
{
	EnterCriticalSection(&g_SimulationCritSec);

	if(g_hSim) {
		GFSDK_WaveWorks_Simulation_Destroy(g_hSim);
		g_hSim = NULL;
	}

	GFSDK_WaveWorks_ReleaseNoGraphics();

	LeaveCriticalSection(&g_SimulationCritSec);
}

int SetSimulationConfig(const SimulationConfig& src)
{
	EnterCriticalSection(&g_SimulationCritSec);

	g_SimConfig = src;
	++g_SimConfigVersion;
	UpdateWaveWorksParams(g_SimParams, g_SimConfig);

	if(gfsdk_waveworks_result_OK != GFSDK_WaveWorks_Simulation_UpdateProperties(g_hSim, g_SimSettings, g_SimParams)) {
		LeaveCriticalSection(&g_SimulationCritSec);
		fprintf(stderr,"ERROR: WaveWorks Simulation_UpdateProperties failed\n");
		ReleaseSimulation();
		return -1;
	}

	LeaveCriticalSection(&g_SimulationCritSec);
	return 0;
}

int GetSimulationConfigVersion()
{
	EnterCriticalSection(&g_SimulationCritSec);
	int result = g_SimConfigVersion;
	LeaveCriticalSection(&g_SimulationCritSec);

	return result;
}

int GetSimulationConfig(SimulationConfig& dst)
{
	EnterCriticalSection(&g_SimulationCritSec);
	int result = g_SimConfigVersion;
	dst = g_SimConfig;
	LeaveCriticalSection(&g_SimulationCritSec);

	return result;
}

void UpdateSimulationParams()
{
	EnterCriticalSection(&g_SimulationCritSec);
	UpdateWaveWorksParams(g_SimParams, g_SimConfig);
	LeaveCriticalSection(&g_SimulationCritSec);
}

int InitSimulation()
{
	InitializeCriticalSection(&g_SimulationCritSec);

	EnterCriticalSection(&g_SimulationCritSec);

#if defined(__ORBIS__)
	sceKernelLoadStartModule("lib_gfsdk_waveworks.ps4.prx", 0, 0, 0, NULL, NULL);
#endif

	g_SimConfig.wind_dir_x = 0.8f;
	g_SimConfig.wind_dir_y = 0.6f;
	g_SimConfig.wind_speed = 2.f;
    g_SimConfig.wind_dependency = 0.98f;
    g_SimConfig.time_scale = 0.5f;
	g_SimConfig.small_wave_fraction = 0.f;

	UpdateSimulationParams();

	g_SimSettings.detail_level = GFSDK_WaveWorks_Simulation_DetailLevel_Normal;
	g_SimSettings.fft_period = 1000.f;
	g_SimSettings.use_Beaufort_scale = true;
	g_SimSettings.readback_displacements = true;
	g_SimSettings.num_readback_FIFO_entries	= 0;
	g_SimSettings.aniso_level = 0;
	g_SimSettings.CPU_simulation_threading_model = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	g_SimSettings.num_GPUs = 1;
	g_SimSettings.enable_CUDA_timers = true;
	g_SimSettings.enable_gfx_timers	 = false;
	g_SimSettings.enable_CPU_timers	 = true;

    printf("INFO: WaveWorks build = %s\n",GFSDK_WaveWorks_GetBuildString());

	if(gfsdk_waveworks_result_OK != GFSDK_WaveWorks_InitNoGraphics(NULL,GFSDK_WAVEWORKS_API_GUID)) {
		LeaveCriticalSection(&g_SimulationCritSec);
		fprintf(stderr,"ERROR: WaveWorks InitNoGraphics failed\n");
		return -1;
	}

	if(gfsdk_waveworks_result_OK != GFSDK_WaveWorks_Simulation_CreateNoGraphics(g_SimSettings,g_SimParams,&g_hSim)) {
		LeaveCriticalSection(&g_SimulationCritSec);
		fprintf(stderr,"ERROR: WaveWorks Simulation_CreateNoGraphics failed\n");
		ReleaseSimulation();
		return -1;
	}

	// Prime the sim
	do {
		UpdateCoordinatedTime();
		g_LastSimulationTick = GetCoordinatedTime();
		GFSDK_WaveWorks_Simulation_SetTime(g_hSim,g_LastSimulationTick);
		GFSDK_WaveWorks_Simulation_KickNoGraphics(g_hSim,NULL);
	} while(gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hSim,NULL));

    // Prime global stats at the same time
    g_LastPrintStats = g_LastSimulationTick;
    GFSDK_WaveWorks_Simulation_GetStats(g_hSim, g_SimStats);

	LeaveCriticalSection(&g_SimulationCritSec);

	return 0;
}

#define FOR_ALL_STATS \
PER_STAT(CPU_main_thread_wait_time) \
PER_STAT(CPU_threads_start_to_finish_time) \
PER_STAT(CPU_threads_total_time) \
PER_STAT(GPU_simulation_time) \
PER_STAT(GPU_FFT_simulation_time) \
PER_STAT(GPU_gfx_time) \
PER_STAT(GPU_update_time)

void FilterStats(const GFSDK_WaveWorks_Simulation_Stats& src, GFSDK_WaveWorks_Simulation_Stats& dst, float deltaTime)
{
    const float lambda = expf(-deltaTime);
    const float invl = 1.f - lambda;

    #define PER_STAT(x) dst.x = lambda * dst.x + invl * src.x;
        FOR_ALL_STATS
    #undef PER_STAT

    g_SimulationTickActualInterval = lambda * g_SimulationTickActualInterval + invl * deltaTime;
}

void PrintStats(const GFSDK_WaveWorks_Simulation_Stats& stats)
{
    printf("INFO: <stats>\n");
    #define PER_STAT(x) printf("INFO:    " #x ": %.2fms\n", stats.x);
        FOR_ALL_STATS
    #undef PER_STAT
    printf("INFO:    TicksPerSec: %.1f\n", 1.f/g_SimulationTickActualInterval);
    printf("INFO: <\\stats>\n");
}

void TickSimulation()
{
	EnterCriticalSection(&g_SimulationCritSec);

	UpdateCoordinatedTime();
	const double t = GetCoordinatedTime();

	float timeSinceLastTick = float(t - g_LastSimulationTick);
	if(timeSinceLastTick > kSimulationTickTargetInterval)
	{
		g_LastSimulationTick = t;
		GFSDK_WaveWorks_Simulation_SetTime(g_hSim,g_LastSimulationTick);
		GFSDK_WaveWorks_Simulation_KickNoGraphics(g_hSim,NULL);

        GFSDK_WaveWorks_Simulation_Stats thisTickStats;
        GFSDK_WaveWorks_Simulation_GetStats(g_hSim, thisTickStats);
        FilterStats(thisTickStats, g_SimStats, timeSinceLastTick);        // Ad-hoc low-pass filter on stats

        float timeSinceLastPrintStats = float(t - g_LastPrintStats);
        if(timeSinceLastPrintStats > kPrintStatsTargetInterval)
        {
            g_LastPrintStats = t;
            if(g_EnablePrintStats)
            {
                PrintStats(g_SimStats);
            }
        }
	}

	LeaveCriticalSection(&g_SimulationCritSec);
}

void SimulationGetPositions(const gfsdk_float2* inSamplePoints, gfsdk_float4* outDisplacements, gfsdk_U32 numSamples)
{
	EnterCriticalSection(&g_SimulationCritSec);

	GFSDK_WaveWorks_Simulation_GetDisplacements(g_hSim,inSamplePoints,outDisplacements,numSamples);

	// Add the sample positions back to the raw displacements
	for(gfsdk_U32 sampleIx = 0; sampleIx != numSamples; ++sampleIx)
	{
		outDisplacements[sampleIx].x += inSamplePoints[sampleIx].x;
		outDisplacements[sampleIx].y += inSamplePoints[sampleIx].y;
	}

	LeaveCriticalSection(&g_SimulationCritSec);
}



/*********************************************************************************
 PER-CLIENT THREAD
*********************************************************************************/
#ifdef TARGET_PLATFORM_NIXLIKE
    typedef FAKE_HANDLE HANDLE;
    typedef void* THREADFUNC_RETTYPE;
    #define THREADFUNC_CALL THREADFUNC_RETTYPE

    void Sleep(FAKE_DWORD dwMilliseconds)
    {
        timespec req, rem;
        req.tv_sec = 0;
        req.tv_nsec = dwMilliseconds * 1000000;
        nanosleep(&req,&rem);
    }

    #define FALSE (0)

	#ifdef __ORBIS__
	const char* inet_ntoa(const in_addr& addr)
	{
		static char buff[256];
		return sceNetInetNtop(SCE_NET_AF_INET, &addr.s_addr, buff, sizeof(buff)/sizeof(buff[0]));
	}
	#endif

#else
    typedef DWORD THREADFUNC_RETTYPE;
    #define THREADFUNC_CALL THREADFUNC_RETTYPE _stdcall
#endif

struct ClientThreadInfo
{
	HANDLE threadUpEvent;
	SOCKET_TYPE clientSocket;
	sockaddr_in clientAddr;
};

volatile int g_RunningThreadCount = 0;
CRITICAL_SECTION g_RunningThreadCountCritSec;

int GetRunningThreadCount()
{
	int result;
	EnterCriticalSection(&g_RunningThreadCountCritSec);
	result = g_RunningThreadCount;
	LeaveCriticalSection(&g_RunningThreadCountCritSec);
	return result;
}

namespace
{
    struct RequestMarkers
    {
	    int num_markers;
		int pad;
	    gfsdk_float2 marker_coords;
    };

    struct ReplyMarkers
    {
	    int num_markers;
		int pad1;
		int pad2;
		int pad3;
	    gfsdk_float4 marker_positions;
    };
}

THREADFUNC_CALL clientThread(void* p)
{
	const ClientThreadInfo clientThreadInfo = *(ClientThreadInfo*)p;

	EnterCriticalSection(&g_RunningThreadCountCritSec);
	++g_RunningThreadCount;
	LeaveCriticalSection(&g_RunningThreadCountCritSec);

	// Info safely copied, thread count safely updated, signal the spawning thread to continue
	SetEvent(clientThreadInfo.threadUpEvent);

	// Log some info
	const in_addr& addr = clientThreadInfo.clientAddr.sin_addr;
	printf("INFO: accepted connection from %s\n",inet_ntoa(addr));

	// Set the socket to non-blocking mode
	if(set_non_blocking(clientThreadInfo.clientSocket)) {
		fprintf(stderr,"ERROR: Failed to set client socket to non-blocking\n");
		return (THREADFUNC_RETTYPE)-1;
	}

	// Set up a send/recv wrapper, and pump it
	// NB: we carefully scope the wrapper so that it goes out of scope before we close the socket
	{
		bool hasSentSimulationConfigToClient = false;
		int lastSimulationConfigVersionSentToClient = 0;
		SocketWrapper sockwrapper(clientThreadInfo.clientSocket);
		while(!g_DoQuit && sockwrapper.is_usable()) {
			if(SocketWrapper::PumpResult_NoTraffic == sockwrapper.pump())
			{
				// Check for client needing a config update
				if(hasSentSimulationConfigToClient && GetSimulationConfigVersion() > lastSimulationConfigVersionSentToClient)
				{
					Message<SimulationConfig> msg;
					msg.messageTypeID = MessageTypeID_ServerSendConfigToClient;
					lastSimulationConfigVersionSentToClient = GetSimulationConfig(msg.payload);
					sockwrapper.enqueue_send_message(&msg, sizeof(msg));
				}
				else
				{
					// Avoid busy waits (because they hog CPU!)
					Sleep(1);
				}
			} else {
				while(sockwrapper.has_recv_message() && sockwrapper.is_usable())
				{
					void* raw_msg = sockwrapper.recv_message();
					int* pMsgType = (int*)raw_msg;
					switch(*pMsgType)
					{
					case MessageTypeID_ClientRequestTimeFromServer:
						{
							Message<double> msg;
							msg.messageTypeID = MessageTypeID_ServerSendTimeToClient;
							msg.payload = GetCoordinatedTime();
							sockwrapper.enqueue_send_message(&msg, sizeof(msg));
						}
						break;
					case MessageTypeID_ClientRequestConfigFromServer:
						{
							Message<SimulationConfig> msg;
							msg.messageTypeID = MessageTypeID_ServerSendConfigToClient;
							lastSimulationConfigVersionSentToClient = GetSimulationConfig(msg.payload);
							sockwrapper.enqueue_send_message(&msg, sizeof(msg));
							hasSentSimulationConfigToClient = true;
						}
						break;
					case MessageTypeID_ClientRequestMarkersFromServer:
						{
							Message<RequestMarkers>* pInMsg = (Message<RequestMarkers>*)raw_msg;

							const unsigned int replySize = sizeof(Message<ReplyMarkers>) + pInMsg->payload.num_markers * sizeof(gfsdk_float4);
							Message<ReplyMarkers>* pOutMsg = (Message<ReplyMarkers>*)malloc(replySize);
							pOutMsg->messageTypeID = MessageTypeID_ServerSendMarkersToClient;
							pOutMsg->payload.num_markers = pInMsg->payload.num_markers;
							SimulationGetPositions(&pInMsg->payload.marker_coords,&pOutMsg->payload.marker_positions,pOutMsg->payload.num_markers);
							sockwrapper.enqueue_send_message(pOutMsg, replySize);
							free(pOutMsg);
						}
						break;
					}
					sockwrapper.consume_recv_message();
				}
			}
		}
	}

	printf("INFO: closing connection from %s\n",inet_ntoa(addr));
	closesocket(clientThreadInfo.clientSocket);

	EnterCriticalSection(&g_RunningThreadCountCritSec);
	--g_RunningThreadCount;
	LeaveCriticalSection(&g_RunningThreadCountCritSec);

	return 0;
}



/*********************************************************************************
 SERVER
*********************************************************************************/
#if defined(__linux__) || defined(__ORBIS__)

    typedef char TCHAR;
    #define _tmain main

    class TerminalInputNixLike
    {
    public:

        bool hasch() const
        {
            struct timeval tv;
            fd_set rdfs;
            
            tv.tv_sec = 0;
            tv.tv_usec = 0;

            FD_ZERO(&rdfs);
            FD_SET(STDIN_FILENO, &rdfs);

            select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
            return 0!=FD_ISSET(STDIN_FILENO, &rdfs);
        }

        TCHAR ch() const
        {
            char buf=0;
            if(read(0,&buf,1)<0)
                fprintf(stderr,"read()");

			// Crude hack to window-dress crappy echo on PS4
			#if defined(__ORBIS__)
			fflush(stdout);
			fprintf(stdout,"\n");
			#endif

            return buf;
        }
    };
#endif

#if defined(__ORBIS__)
	void print_host_interfaces(SOCKET_TYPE, const char* fmt)
	{
		SceNetCtlInfo ci;
		if(sceNetCtlGetInfo(SCE_NET_CTL_INFO_IP_ADDRESS,&ci))
		{
			return;
		}

		printf(fmt, "eth", ci.ip_address, "(IP4)"); 
	}

	typedef TerminalInputNixLike TerminalInput;

#elif defined(__linux__)
	void print_host_interfaces(SOCKET_TYPE, const char* fmt)
	{
		struct ifaddrs * ifAddrStruct=NULL;
		struct ifaddrs * ifa=NULL;
		void * tmpAddrPtr=NULL;

		getifaddrs(&ifAddrStruct);

		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
			if(ifa ->ifa_addr) {
				if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
					// is a valid IP4 Address
					tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
					char addressBuffer[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
					printf(fmt, ifa->ifa_name, addressBuffer, "(IP4)"); 
				} else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
					// is a valid IP6 Address
					tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
					char addressBuffer[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
					printf(fmt, ifa->ifa_name, addressBuffer, "(IP6)"); 
				}
			}
		}
		if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
	}

    class TerminalInput : public TerminalInputNixLike
    {
    public:
        TerminalInput()
        {
            fflush(stdout);
            if(tcgetattr(0, &m_original_term)<0)
                fprintf(stderr,"tcsetattr()");
            struct termios newterm = m_original_term;
            newterm.c_lflag&=~ICANON;
            newterm.c_lflag&=~ECHO;
            newterm.c_cc[VMIN]=0;
            newterm.c_cc[VTIME]=0;
            if(tcsetattr(0, TCSANOW, &newterm)<0)
                fprintf(stderr,"tcsetattr ICANON %s", strerror(errno));

            atexit(term_restore);
        }

    private:

        static void term_restore(void)
        {
            // Restore original
            fflush(stdout);
            if(tcsetattr(0, TCSANOW, &m_original_term)<0) {
                fprintf(stderr,"tcsetattr ICANON %s", strerror(errno));
            } else {
                printf("INFO: terminal mode restored\n");
            }
        }

        static struct termios m_original_term;
    };

    struct termios TerminalInput::m_original_term;

#else // !__linux__ && !__ORBIS__...

	void print_host_interfaces(SOCKET_TYPE sock, const char* fmt)
	{
		INTERFACE_INFO InterfaceList[16];
		unsigned long nBytesReturned;
		if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList, sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR)
		{
			fprintf(stderr,"ERROR: failed to retrieve interface list from listening socket\n");
			return;
		}

		// NB: assumes IPv4, which is fine because we create the socket with AF_INET
		const int numInterfaces = nBytesReturned/sizeof(InterfaceList[0]);
		for(int i = 0; i != numInterfaces; ++i)
		{
			printf(fmt, "", inet_ntoa(InterfaceList[i].iiAddress.AddressIn.sin_addr), "(IP4)" );
		}
	}

#ifdef _XBOX_ONE

	using namespace Windows::Xbox::Input;
	using namespace Windows::Foundation::Collections;

    class TerminalInput
    {
    public:

		TerminalInput()
		{
			m_ch = '\0';
		}

        bool hasch()
		{
			bool result = false;

			auto allGamepads = Gamepad::Gamepads;
			if( allGamepads->Size > 0 )
			{
				IGamepad^ gamepad = allGamepads->GetAt( 0 );
				IGamepadReading^ gamepadReading = gamepad->GetCurrentReading();
				if(m_prevGamepadReading)
				{
					if(gamepadReading->IsBPressed && !m_prevGamepadReading->IsBPressed)
					{
						m_ch = 'q';
						result = true;
					}
					else if(gamepadReading->IsYPressed && !m_prevGamepadReading->IsYPressed)
					{
						m_ch = 's';
						result = true;
					}
					else if(gamepadReading->IsDPadLeftPressed && !m_prevGamepadReading->IsDPadLeftPressed)
					{
						m_ch = '-';
						result = true;
					}
					else if(gamepadReading->IsDPadRightPressed && !m_prevGamepadReading->IsDPadRightPressed)
					{
						m_ch = '+';
						result = true;
					}
				}

				m_prevGamepadReading = gamepadReading;
			}

			return result;
		}
        TCHAR ch() const { return m_ch; }

	private:

		IGamepadReading^ m_prevGamepadReading;
		TCHAR m_ch;
    };

#else // _XBOX_ONE

    class TerminalInput
    {
    public:

        bool hasch() const { return 0!=_kbhit(); }
        TCHAR ch() const { return _gettch(); }
    };

#endif

#endif

const unsigned short kServerListenPort = ServerPort;

void sighandler(int s){
	if(s == SIGINT) {
		g_DoQuit = true;
	}
}

int run_the_server(SOCKET_TYPE listeningSocket, TerminalInput& termin)
{
	int retval = 0;

	// Get listening socket addr
	sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	listen_addr.sin_port=htons(kServerListenPort);

	// Bind the socket
    if (bind(listeningSocket,(sockaddr*)&listen_addr, sizeof(listen_addr))) {
		fprintf(stderr,"ERROR: Failed to bind listening socket\n");
		return -1;
	}

	// Get the socket listening
    if (listen(listeningSocket,SOMAXCONN)) {
		fprintf(stderr,"ERROR: Failed to set listening socket to listen\n");
		return -1;
	}

	// Set the socket to non-blocking mode
	if(set_non_blocking(listeningSocket)) {
		fprintf(stderr,"ERROR: Failed to set listening socket to non-blocking\n");
		return -1;
	}

    // Output the hostname and IP addresses of the system, for info
    char szHostName[256];
    if(0 == gethostname(szHostName, (sizeof(szHostName)/sizeof(szHostName[0]))-1))
    {
        printf("INFO: listening on host %s\n",szHostName);
    }
    else
    {
        printf("WARNING: listening on unknown host (hostname lookup failed)\n");
    }

    printf("INFO: host interfaces...\n");
	print_host_interfaces(listeningSocket, "INFO:    %s -> %s %s\n");
    printf("INFO: ...end of host interfaces\n");

	// Hook Ctrl-C gracefully
	#if !defined(__ORBIS__)
	signal(SIGINT, sighandler);
	#endif

	// Init the sim
    InitCoordinatedTime();
	if(InitSimulation()) {
		return -1;
	}

	// Accept connections
	InitializeCriticalSection(&g_RunningThreadCountCritSec);
	while(!g_DoQuit) {

		TickSimulation();

		sockaddr_in accept_addr;
		socklen_t accept_addr_size = sizeof(accept_addr);
		SOCKET_TYPE acceptedSocket = accept(listeningSocket,(sockaddr*)&accept_addr,&accept_addr_size);
		if(acceptedSocket == INVALID_SOCKET) {
			if(wouldblock()) {
				Sleep(1);
			} else {
				// Once we get here, an element of grace is required to shutdown because we potentially
				// have threads in flight
				fprintf(stderr,"ERROR: accept() failed on listening socket\n");
				retval = -1;
				g_DoQuit = true;
			}
		} else {
			// Start server thread for connected client
			ClientThreadInfo clientThreadInfo;
			clientThreadInfo.clientAddr = accept_addr;
			clientThreadInfo.clientSocket = acceptedSocket;
			clientThreadInfo.threadUpEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
			CreateThread(NULL,0,clientThread,(void*)&clientThreadInfo,0,NULL);
			WaitForSingleObject(clientThreadInfo.threadUpEvent,INFINITE);
			CloseHandle(clientThreadInfo.threadUpEvent);
		}

		if(termin.hasch()) {
			switch(termin.ch())
			{
            case TCHAR('s'):
                g_EnablePrintStats = !g_EnablePrintStats;
                break;
			case TCHAR('q'):
				g_DoQuit = true;
				break;
			case TCHAR('-'):
				{
					SimulationConfig cfg;
					GetSimulationConfig(cfg);
					printf("INFO: old wind_speed=%.2f\n",cfg.wind_speed);
					cfg.wind_speed *= 1.f/1.1f;
					if(SetSimulationConfig(cfg)) {
						// Something failed, so quit
						g_DoQuit = true;
					}
					printf("INFO: new wind_speed=%.2f\n",cfg.wind_speed);
				}
				break;
			case TCHAR('+'):
				{
					SimulationConfig cfg;
					GetSimulationConfig(cfg);
					printf("INFO: old wind_speed=%.2f\n",cfg.wind_speed);
					cfg.wind_speed *= 1.1f;
					if(SetSimulationConfig(cfg)) {
						// Something failed, so quit
						g_DoQuit = true;
					}
					printf("INFO: new wind_speed=%.2f\n",cfg.wind_speed);
				}
				break;
			}
		}
	}

	// Wait for threads to come home
	printf("INFO: server shutting down, waiting for client threads to graceful exit...\n");
	while(GetRunningThreadCount())
	{
		Sleep(1);
	}
	printf("INFO: ...done, all threads are safely home\n");

	ReleaseSimulation();

	return retval;
}


/*********************************************************************************
 APP
*********************************************************************************/

#ifdef _XBOX_ONE
[Platform::MTAThread] int main( Platform::Array< Platform::String^ >^ /*params*/ )
#else
int _tmain(int /*argc*/, TCHAR** /*argv[]*/)
#endif
{
    TerminalInput term;

	// Start networking
	if(init_networking()) {
		fprintf(stderr,"ERROR: init_networking failed\n");
		return -1;
	}

	// Init listening socket
	SOCKET_TYPE listeningSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(INVALID_SOCKET == listeningSocket) {
		fprintf(stderr,"ERROR: Failed to create listening socket\n");
		term_networking();
		return -1;
	}

	// Run the server
	const int result = run_the_server(listeningSocket,term);

	// Cleanup
	closesocket(listeningSocket);
	term_networking();

	return result;
}

