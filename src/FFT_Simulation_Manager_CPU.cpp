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

#include "Internal.h"

#ifdef SUPPORT_FFTCPU
#include "FFT_Simulation_Manager_CPU_impl.h"
#include "FFT_Simulation_CPU_impl.h"
#include "Simulation_Util.h"
#include "restricted/GFSDK_WaveWorks_CPU_Scheduler.h"

#include "ThreadWrap.h"

void JobThreadFunc(void* p);

#ifdef TARGET_PLATFORM_NIXLIKE
void* PlatformJobThreadFunc(void* p)
{
	JobThreadFunc(p);
	return NULL;
}

namespace
{
	struct SYSTEM_INFO
	{
		DWORD dwNumberOfProcessors;
	};
	void GetSystemInfo(SYSTEM_INFO* info)
	{
		info->dwNumberOfProcessors = 8;
	}
}
#else
DWORD __GFSDK_STDCALL__ PlatformJobThreadFunc(void* p)
{
	JobThreadFunc(p);
	return 0;
}
#endif

#define MAX_CASCADES        GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades

namespace
{
	enum SimulationTaskType { T_UPDATE_H0, T_UPDATE_HT, T_FFT_XY_NxN, T_FFT_X_N, T_FFT_Y_N, T_UPDATE_TEXTURE };

	gfsdk_U32 packTaskData(SimulationTaskType stt, int cascadeIndex, int jobIndex)
	{
		return (jobIndex & 0x0000FFFF) | ((cascadeIndex << 16) & 0x00FF0000) | ((stt << 24) & 0xFF000000);
	}

	void unpackTaskData(gfsdk_U32 data, int& cascadeIndex, int& jobIndex)
	{
		cascadeIndex = ((data & 0x00FF0000) >> 16);
		jobIndex =  data & 0x0000FFFF;
	}

	SimulationTaskType getStt(gfsdk_U32 data)
	{
		return (SimulationTaskType)((data & 0xFF000000) >> 24);
	}
}

struct NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue : public GFSDK_WaveWorks_CPU_Scheduler_Interface
{
private:
	enum { MAX_THREADS = 32 };						//limitation due to WaitFor*Object*
	HANDLE m_job_threads[MAX_THREADS];
	HANDLE m_job_thread_kick_events[MAX_THREADS];	// the event to re-awaken an idle thread
	int m_job_threads_count;

	// timings variables
	float m_time_threads_work[MAX_THREADS]; // each thread updates its work time in ThreadMemberFunc
	float m_time_threads_start_to_stop;	// time between starting the 1st thread's work and completing the last thread's work
	float m_time_threads_work_total;	// sum of all time spent in threads doing work 

	TickType m_threadsStartTimestamp; // the timestamp taken at the moment the working threads start the work
	TickType m_threadsStopTimestamp; // the timestamp taken at the moment the working threads stop the work

	struct ThreadInfo {
		NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue* m_pThis;
		int m_thread_idx;
	};
	ThreadInfo m_threadInfos[MAX_THREADS];

	// We run a fixed-allocation ring-buffer for our work queue
	// maximum queue size is: *2 means h0->ht and texture update tasks, *6 is a maximum number of FFTs
	enum { WorkQueueSize = MAX_FFT_RESOLUTION*MAX_CASCADES*2+MAX_CASCADES*3 };
	gfsdk_U32 m_workqueue[WorkQueueSize];
	volatile int m_workqueue_head;
	volatile int m_workqueue_tail;	// One beyond the end of the last item in the queue
									// Ergo: queue is empty when m_workqueue_head == m_workqueue_tail
	volatile LONG m_work_items_in_flight;
	HANDLE m_workdone_event;

	// We maintain a shadow ring-buffer to manage task type information
	enum TypeTag { Tag_CLIENT = 0, Tag_EOF, Tag_THREAD_EXIT };
	TypeTag m_workqueue_tags[WorkQueueSize];

	CRITICAL_SECTION m_section;

	HANDLE m_idle_thread_kick_events[MAX_THREADS];
	volatile int m_num_idle_threads;

	bool m_KickIsActive;
	GFSDK_WaveWorks_CPU_Scheduler_Interface::ProcessTaskFn m_ProcessTaskFn;
	void* m_ProcessTaskContext;

	bool enable_CPU_timers;

	void ThreadMemberFunc(int thread_index)
	{
		TickType tStart,tStop;

		gfsdk_U32 taskData;
		TypeTag ttt;
		do
		{
			ttt = pop(m_job_thread_kick_events[thread_index], taskData);
			if(Tag_CLIENT == ttt) {
				// tying thread to thread's own core to ensure OS doesn't reallocathe thread to other cores which might corrupt QueryPerformanceCounter readings
				GFSDK_WaveWorks_Simulation_Util::tieThreadToCore((unsigned char)thread_index);
				// getting the timestamp
				if(enable_CPU_timers)
				{
					tStart = GFSDK_WaveWorks_Simulation_Util::getTicks();
				}

				m_ProcessTaskFn(m_ProcessTaskContext, taskData);

				// getting the timestamp and calculating time
				if(enable_CPU_timers)
				{
					tStop = GFSDK_WaveWorks_Simulation_Util::getTicks();
					m_time_threads_work[thread_index] += GFSDK_WaveWorks_Simulation_Util::getMilliseconds(tStart,tStop);
				}

				if(0==onTaskCompleted()) 
				{
					if(enable_CPU_timers)
					{
						// queue is empty, stop the timer
						m_threadsStopTimestamp = tStop;
						m_time_threads_start_to_stop = GFSDK_WaveWorks_Simulation_Util::getMilliseconds(m_threadsStartTimestamp,m_threadsStopTimestamp);
					}
					else
					{
						m_time_threads_start_to_stop = 0;
					}
				}
			}
			else if(Tag_THREAD_EXIT == ttt)
			{
				// Still need to make completion for exit events, in order to transition the work-done event correctly
				onTaskCompleted();
			}
		}
		while(ttt!=Tag_THREAD_EXIT);
	}
	void processAllTasks()
	{
		// Synchronous task processing
		bool done = false;
		do
		{
			gfsdk_U32 taskData;
			TypeTag ttt = pop(NULL,taskData);
			assert(Tag_CLIENT == ttt);

			m_ProcessTaskFn(m_ProcessTaskContext, taskData);

			if(0==onTaskCompleted())
			{
				done = true;
			}
		} while(!done);

		if(enable_CPU_timers)
		{
			// all done, stop the timer
			m_threadsStopTimestamp = GFSDK_WaveWorks_Simulation_Util::getTicks();
			m_time_threads_start_to_stop = GFSDK_WaveWorks_Simulation_Util::getMilliseconds(m_threadsStartTimestamp,m_threadsStopTimestamp);
		}
		else
		{
			m_time_threads_start_to_stop = 0;
		}
	}
	void onTaskPushed()
	{
		int updatedNumWorkItemsInFlight = InterlockedIncrement(&m_work_items_in_flight);
		assert(updatedNumWorkItemsInFlight > 0);
		if(1 == updatedNumWorkItemsInFlight)	// On transition from 0
			ResetEvent(m_workdone_event);
	}
	void onTasksPushed(int n)
	{
		if(n)
		{
			int updatedNumWorkItemsInFlight = customInterlockedAdd(&m_work_items_in_flight, n);
			assert(updatedNumWorkItemsInFlight > 0);
			if(n == updatedNumWorkItemsInFlight)	// On transition from 0
				ResetEvent(m_workdone_event);
		}
	}
	int onTaskCompleted()
	{
		int updatedNumWorkItemsInFlight = InterlockedDecrement(&m_work_items_in_flight);
		assert(updatedNumWorkItemsInFlight >= 0);
		if(0 == updatedNumWorkItemsInFlight)
		{
			EnterCriticalSection(&m_section);
			m_ProcessTaskFn = NULL;
			m_ProcessTaskContext = NULL;
			m_KickIsActive = false;
			LeaveCriticalSection(&m_section);
			SetEvent(m_workdone_event);
		}
		return updatedNumWorkItemsInFlight;
	}
	void push(gfsdk_U32 t, TypeTag ttt)
	{
		EnterCriticalSection(&m_section);
		m_workqueue[ m_workqueue_tail ] = t;
		m_workqueue_tags[ m_workqueue_tail ] = ttt;
		m_workqueue_tail = (m_workqueue_tail+1)%WorkQueueSize;
		onTaskPushed();
		LeaveCriticalSection(&m_section);
	}
	TypeTag pop(HANDLE callingThreadKickEvent, gfsdk_U32& dst)
	{
		EnterCriticalSection(&m_section);

		// NB: We only allow the calling thread to pop() tasks iff a kick has occured (we definitely don't want threads to pick up work
		// until the kick() has occured, all sorts of horrible race conditions could ensue...)
		if(m_KickIsActive && m_workqueue_tail!=m_workqueue_head)
		{
			dst = m_workqueue[ m_workqueue_head ];
			TypeTag ttt = m_workqueue_tags[ m_workqueue_head ];
			m_workqueue_head = (m_workqueue_head+1)%WorkQueueSize;
			if(m_workqueue_tail!=m_workqueue_head) {
				// There's still work in the queue - kick any idle threads to pick up the work
				kick();
			}
			LeaveCriticalSection(&m_section);
			return ttt;
		}

		if(callingThreadKickEvent)
		{
			// We're out of tasks and the caller provided a non-NULL kick event in the expectation that we are
			// able to go quiescent for a while, so push our thread onto the idle stack and wait for wake-up
			m_idle_thread_kick_events[m_num_idle_threads++] = callingThreadKickEvent;
			assert(m_num_idle_threads < MAX_THREADS);
			LeaveCriticalSection(&m_section);
			WaitForSingleObject(callingThreadKickEvent,INFINITE);
		}
		else
		{
			LeaveCriticalSection(&m_section);
		}

		return Tag_EOF;	// return EOF to signify that work ran out on first attempt to grab a task,
						// caller can then try again with reasonable expectation of success (because the thread
						// will have been woken up with a kick(), implying sufficient work is now available)
	}
	bool kick()
	{
		bool alreadyDidTheWork = false;
		if(m_job_threads_count)
		{
			EnterCriticalSection(&m_section);
			if(m_num_idle_threads)
			{
				--m_num_idle_threads;
				SetEvent(m_idle_thread_kick_events[m_num_idle_threads]);
			}
			LeaveCriticalSection(&m_section);
		}
		else
		{
			// No job threads, so do the work syncrhonously on this thread
			processAllTasks();
			alreadyDidTheWork = true;
		}

		return alreadyDidTheWork;
	}

	friend void JobThreadFunc(void* p);

public:
	NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue(const GFSDK_WaveWorks_Detailed_Simulation_Params& params)
	{
		m_workqueue_head = 0;
		m_workqueue_tail = 0;
		m_work_items_in_flight = 0;
		m_workdone_event = CreateEvent(NULL, TRUE, TRUE, NULL);		// create in signalled state, because our initial state is out-of-work
		InitializeCriticalSectionAndSpinCount(&m_section, 10000);	// better then default
		m_num_idle_threads = 0;
		m_KickIsActive = false;
		m_ProcessTaskFn = NULL;
		m_ProcessTaskContext = NULL;
		enable_CPU_timers = params.enable_CPU_timers;

		SYSTEM_INFO si;
		//getting the number of physical cores
		GetSystemInfo(&si);

		if(GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic == params.CPU_simulation_threading_model)
		{
			m_job_threads_count = si.dwNumberOfProcessors;
		}
		else if(GFSDK_WaveWorks_Simulation_CPU_Threading_Model_None == params.CPU_simulation_threading_model)
		{
			m_job_threads_count = 0;
		}
		else
		{
			m_job_threads_count = int(params.CPU_simulation_threading_model) < int(MAX_THREADS) ? int(params.CPU_simulation_threading_model) : int(MAX_THREADS);
		
			// limiting the number of worker threads to the number of processors
			if((unsigned int) m_job_threads_count > si.dwNumberOfProcessors) 
			{
				m_job_threads_count = si.dwNumberOfProcessors;
			}
		}

		//threads starts
		for(int t = 0; t < m_job_threads_count; t++)
		{
			m_threadInfos[t].m_pThis = this;
			m_threadInfos[t].m_thread_idx = t;
			m_job_thread_kick_events[t] = CreateEvent(NULL, FALSE, FALSE, NULL);	// kick-events are auto-resetting

			DWORD jobThreadId;
			m_time_threads_work[t] = 0;
			m_job_threads[t] = CreateThread(0, 0, PlatformJobThreadFunc, (void*)&m_threadInfos[t], 0, &jobThreadId);
		
			//pinning threads to particular cores does not provide noticeable perf benefits, 
			//so leaving OS to allocate cores for threads dynamically
			//SetThreadAffinityMask(m_job_threads[t], (1<<t)%(1<<(params.CPU_simulation_threads_count-1))); 
		}
	}
	~NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue()
	{
		waitForWorkDone();
		for(int t=0; t<m_job_threads_count; t++)
		{
			gfsdk_U32 taskData = t;
			push(taskData, Tag_THREAD_EXIT );
		}
		kick(NULL,NULL);
		waitForWorkDone();

		// Wait for all the threads to exit
		for(int t = 0; t < m_job_threads_count; t++)
		{
	#ifdef TARGET_PLATFORM_NIXLIKE
			pthread_join(*(pthread_t*)m_job_threads[t], NULL);
			delete (pthread_t*)m_job_threads[t];
	#else
			WaitForSingleObject(m_job_threads[t],INFINITE);
			CloseHandle(m_job_threads[t]);
	#endif
			CloseHandle(m_job_thread_kick_events[t]);
		}

		DeleteCriticalSection(&m_section);
		CloseHandle(m_workdone_event);
	}
	void push(gfsdk_U32 t)
	{
		push(t, Tag_CLIENT);
	}
	void push(const gfsdk_U32* t, int n)
	{
		EnterCriticalSection(&m_section);

		// First, copy up to the available queue space
		int n0 = WorkQueueSize-m_workqueue_tail;
		if(n0 > n)
		{
			n0 = n;
		}
		memcpy(m_workqueue + m_workqueue_tail, t, n0 * sizeof(*t));
		memset(m_workqueue_tags + m_workqueue_tail, Tag_CLIENT, n0 * sizeof(*m_workqueue_tags));
		m_workqueue_tail = (m_workqueue_tail+n0)%WorkQueueSize;
		if(n0 < n)
		{
			// Then copy any remainder at the start of the queue space
			const int n1 = n-n0;
			memcpy(m_workqueue + m_workqueue_tail, t + n0, n1 * sizeof(*t));
			memset(m_workqueue_tags + m_workqueue_tail, Tag_CLIENT, n1 * sizeof(*m_workqueue_tags));
			m_workqueue_tail = (m_workqueue_tail+n1)%WorkQueueSize;
		}
		onTasksPushed(n);

		LeaveCriticalSection(&m_section);
	}
	void pushRandom(gfsdk_U32 t)
	{
		EnterCriticalSection(&m_section);
		if(m_workqueue_tail==m_workqueue_head)
		{
			m_workqueue[ m_workqueue_tail ] = t;
			m_workqueue_tags[ m_workqueue_tail ] = Tag_CLIENT;
			m_workqueue_tail = (m_workqueue_tail+1)%WorkQueueSize;
		}
		else
		{
			const int num_workqueue = (m_workqueue_tail + WorkQueueSize - m_workqueue_head) % WorkQueueSize;
			int p = (m_workqueue_head + rand() % num_workqueue) % WorkQueueSize;
			m_workqueue[ m_workqueue_tail ] = m_workqueue[ p ];
			m_workqueue_tags[ m_workqueue_tail ] = m_workqueue_tags[ p ];
			m_workqueue_tail = (m_workqueue_tail+1)%WorkQueueSize;
			m_workqueue[ p ] = t;
			m_workqueue_tags[ p ] = Tag_CLIENT;
		}
		onTaskPushed();
		LeaveCriticalSection(&m_section);
	}
	void waitForWorkDone()
	{
		// If there are no job threads, there can never be any threads to wait for
		if(m_job_threads_count)
		{
			WaitForSingleObject(m_workdone_event, INFINITE);
		}

		// sum up the total time all the worker threads spent on work
		// and clearing the threads' work times before new simulation tick
		m_time_threads_work_total = 0;
		for(int i=0; i<m_job_threads_count; i++)
		{
			m_time_threads_work_total += m_time_threads_work[i];
			m_time_threads_work[i]=0;
		}
	}
	bool isWorkDone()
	{
		#ifdef TARGET_PLATFORM_NIXLIKE
			return FAKE_WAIT_OBJECT_0 == WaitForSingleObject(m_workdone_event, 0);
		#else
			return WAIT_OBJECT_0 == WaitForSingleObject(m_workdone_event, 0);
		#endif
	}
	bool kick(ProcessTaskFn taskHandler, void* pContext)
	{
		EnterCriticalSection(&m_section);

		if(enable_CPU_timers)
		{
			// Trigger the start/stop timer
			GFSDK_WaveWorks_Simulation_Util::tieThreadToCore(0);
			m_threadsStartTimestamp = GFSDK_WaveWorks_Simulation_Util::getTicks();
		}

		m_ProcessTaskFn = taskHandler;
		m_ProcessTaskContext = pContext;
		m_KickIsActive = true;
		const bool result = kick();

		LeaveCriticalSection(&m_section);

		return result;
	}

	// Stats
	float getThreadsStartToStopTime() const { return m_time_threads_start_to_stop; }
	float getThreadsWorkTotalTime() const { return m_time_threads_work_total; }
};

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::UpdateH0(gfsdk_U32 taskData)
{
	int cascadeIndex;
	int jobIndex;
	unpackTaskData(taskData, cascadeIndex, jobIndex);

	const int row = jobIndex;
	NVWaveWorks_FFT_Simulation_CPU_Impl* pCascade = m_Simulations[cascadeIndex];
	if(pCascade->UpdateH0(row)) {
		//push update Ht tasks - one per scan-line
		// (m_Simulations[task.m_cascade]->m_ref_count_update_ht == N already - this was done when the processing chain was first kicked)
		UINT N = pCascade->GetParams().fft_resolution;
		for(int t=0; t<int(N); t++)
		{
			gfsdk_U32 t1 = packTaskData(T_UPDATE_HT, cascadeIndex, t);
			m_queue->push(t1);
		}
	}
}

namespace
{
	int packFFTJobIndex(int XYZindex, int subIndex)
	{
		return (subIndex & 0x00000FFF) | ((XYZindex << 12) & 0x0000F000);
	}

	void unpackFFTJobIndex(int jobIndex, int& XYZindex, int& subIndex)
	{
		XYZindex = ((jobIndex & 0x0000F000) >> 12);
		subIndex =  jobIndex & 0x00000FFF;
	}
}

//Starts 3 FFT after all scan-lines will be processed
void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::UpdateHt(gfsdk_U32 taskData)
{
	int cascadeIndex;
	int jobIndex;
	unpackTaskData(taskData, cascadeIndex, jobIndex);

	const int row = jobIndex;
	NVWaveWorks_FFT_Simulation_CPU_Impl* pCascade = m_Simulations[cascadeIndex];
	if(pCascade->UpdateHt(row)) {

		/* Legacy path with monolithic FFT
		//push 3 FFT task and setup count to track finish all of this
		// (task.m_cascade->m_ref_count_FFT == 3 already - this was done when the processing chain was first kicked)
		for(int t=0; t<3; t++)
		{
			gfsdk_U32 t1 = packTaskData(T_FFT_XY_NxN, cascadeIndex, t);
			m_queue->pushRandom(t1); //mix tasks of different types to relax memory bus
		}
		*/

		int N = pCascade->GetNumRowsIn_FFT_X();
		for(int i=0; i<3; i++)
		{
			for(int row=0; row<int(N);row++)
			{
				int jobIndex = packFFTJobIndex(i,row);
				gfsdk_U32 t1 = packTaskData(T_FFT_X_N, cascadeIndex, jobIndex);
				m_queue->pushRandom(t1); //mix tasks of different types to relax memory bus
			}
		}
	}
}

// Legacy FFT path...
// We call FFT2D in parallel worker threads 
// and now we have 12 FFT2D calls that can be overlapped
// by update texture and spectrum tasks so overal performance was increased by factor of 2 on 4 cores + HT CPU
void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::ComputeFFT_XY_NxN(gfsdk_U32 taskData)
{
	int cascadeIndex;
	int jobIndex;
	unpackTaskData(taskData, cascadeIndex, jobIndex);

	const int index = jobIndex;
	NVWaveWorks_FFT_Simulation_CPU_Impl* pCascade = m_Simulations[cascadeIndex];
	if(pCascade->ComputeFFT_XY_NxN(index)) {
		//push update texture tasks - one per scan-line
		// (task.m_cascade->m_ref_count_update_texture == N already - this was done when the processing chain was first kicked)
		UINT N = pCascade->GetParams().fft_resolution;
		for(int t=0; t<int(N); t++)
		{
			gfsdk_U32 t1 = packTaskData(T_UPDATE_TEXTURE, cascadeIndex, t);
			m_queue->push(t1);
		}
	}
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::ComputeFFT_X_N(gfsdk_U32 taskData)
{
	int cascadeIndex;
	int jobIndex;
	unpackTaskData(taskData, cascadeIndex, jobIndex);

	int XYZindex, subIndex;
	unpackFFTJobIndex(jobIndex, XYZindex, subIndex);

	NVWaveWorks_FFT_Simulation_CPU_Impl* pCascade = m_Simulations[cascadeIndex];
	if(pCascade->ComputeFFT_X(XYZindex, subIndex)) {
		int N = pCascade->GetNumRowsIn_FFT_Y();
		for(int i=0; i<3; i++)
		{
			for(int col=0; col<int(N); col++)
			{
				int jobIndex = packFFTJobIndex(i,col);
				gfsdk_U32 t1 = packTaskData(T_FFT_Y_N, cascadeIndex, jobIndex);
				m_queue->pushRandom(t1); //mix tasks of different types to relax memory bus
			}
		}
	}
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::ComputeFFT_Y_N(gfsdk_U32 taskData)
{
	int cascadeIndex;
	int jobIndex;
	unpackTaskData(taskData, cascadeIndex, jobIndex);

	int XYZindex, subIndex;
	unpackFFTJobIndex(jobIndex, XYZindex, subIndex);

	NVWaveWorks_FFT_Simulation_CPU_Impl* pCascade = m_Simulations[cascadeIndex];
	if(pCascade->ComputeFFT_Y(XYZindex, subIndex)) {
		//push update texture tasks - one per scan-line
		// (task.m_cascade->m_ref_count_update_texture == N already - this was done when the processing chain was first kicked)
		UINT N = pCascade->GetParams().fft_resolution;
		for(int t=0; t<int(N); t++)
		{
			gfsdk_U32 t1 = packTaskData(T_UPDATE_TEXTURE, cascadeIndex, t);
			m_queue->push(t1);
		}
	}
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::UpdateTexture(gfsdk_U32 taskData)
{
	int cascadeIndex;
	int jobIndex;
	unpackTaskData(taskData, cascadeIndex, jobIndex);

	const int row = jobIndex;
	NVWaveWorks_FFT_Simulation_CPU_Impl* pCascade = m_Simulations[cascadeIndex];
	pCascade->UpdateTexture(row);
}

//worker threads' functions
void JobThreadFunc(void* p)
{
	NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue::ThreadInfo* pThreadInfo = (NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue::ThreadInfo*)p;
	pThreadInfo->m_pThis->ThreadMemberFunc(pThreadInfo->m_thread_idx);
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::ProcessTaskFn(void* pContext, gfsdk_U32 taskData)
{
	NVWaveWorks_FFT_Simulation_Manager_CPU_Impl* thisPtr = (NVWaveWorks_FFT_Simulation_Manager_CPU_Impl*)pContext;
	thisPtr->ProcessTask(taskData);
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::ProcessTask(gfsdk_U32 taskData)
{
	switch(getStt(taskData))
	{
		case T_UPDATE_H0:
			UpdateH0(taskData);
			break;
		case T_UPDATE_HT:
			UpdateHt(taskData);
			break;
		case T_FFT_XY_NxN:
			ComputeFFT_XY_NxN(taskData);
			break;
		case T_FFT_X_N:
			ComputeFFT_X_N(taskData);
			break;
		case T_FFT_Y_N:
			ComputeFFT_Y_N(taskData);
			break;
		case T_UPDATE_TEXTURE:
			UpdateTexture(taskData);
			break;
		default:
			break;
	}
}

NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::NVWaveWorks_FFT_Simulation_Manager_CPU_Impl(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler) :
	m_NextKickID(0),
	m_IsWaitingInFlightKick(false),
	m_HasPendingFlip(false),
	m_InFlightKickID(0),
	m_HasReadyKick(false),
	m_ReadyKickID(0),
	m_pushBuffer(0),
	m_pushBufferCapacity(0),
	m_enable_CPU_timers(params.enable_CPU_timers)
{
	// User scheduler only offered for NDA builds
#if defined(WAVEWORKS_NDA_BUILD)
	if(pOptionalScheduler)
	{
		m_defaultQueue = 0;
		m_queue = pOptionalScheduler;
	}
	else
#endif
	{
		m_defaultQueue = new NVWaveWorks_FFT_Simulation_CPU_Impl_Workqueue(params);
		m_queue = m_defaultQueue;
	}
}

NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::~NVWaveWorks_FFT_Simulation_Manager_CPU_Impl()
{
	m_queue->waitForWorkDone();		// In case we're using some other queue
	SAFE_DELETE(m_defaultQueue);	// This will close all the threads etc.
	assert(0 == m_Simulations.size());	// It is an error to destroy a non-empty manager
	m_Simulations.erase_all();
	SAFE_DELETE_ARRAY(m_pushBuffer);
}

NVWaveWorks_FFT_Simulation* NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::createSimulation(const GFSDK_WaveWorks_Detailed_Simulation_Params::Cascade& params)
{
	NVWaveWorks_FFT_Simulation_CPU_Impl* pResult = new NVWaveWorks_FFT_Simulation_CPU_Impl(params);
	m_Simulations.push_back(pResult);
	return pResult;
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::releaseSimulation(NVWaveWorks_FFT_Simulation* pSimulation)
{
	// finalize all threads before release
	if(m_IsWaitingInFlightKick)
	{
		waitTasksCompletion();
		m_HasPendingFlip = false;	// But don't bother flipping
	}

	//remove from list
	m_Simulations.erase(pSimulation);

	SAFE_DELETE(pSimulation);
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::beforeReinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, bool reinitOnly)
{
	bool reinitCanBePipelined = false;
	if(reinitOnly)
	{
		// Can potentially be pipelined (i.e. no need to WFI the CPU pipeline to change state)
		reinitCanBePipelined = true;

		assert(m_Simulations.size() == params.num_cascades);
		for(int i=0; i<m_Simulations.size(); i++)
		{
			NVWaveWorks_FFT_Simulation_CPU_Impl* c = m_Simulations[i];

			bool bRelease = false;
			bool bAllocate = false;
			bool bReinitH0 = false;
			bool bReinitGaussAndOmega = false;
			c->calcReinit(params.cascades[i], bRelease, bAllocate, bReinitH0, bReinitGaussAndOmega);

			if(bRelease || bAllocate || bReinitGaussAndOmega)
			{
				// Can't pipeline if release/alloc required or if Gauss/Omega need update
				reinitCanBePipelined = false;
				break;
			}
		}
	}

	if(reinitCanBePipelined)
	{
		for(int i=0; i<m_Simulations.size(); i++)
		{
			NVWaveWorks_FFT_Simulation_CPU_Impl* c = m_Simulations[i];
			c->pipelineNextReinit();
		}
	}
	else
	{
		// WFI needed

		// need this to ensure tasks in separate threads are not working with stale data
		if(m_IsWaitingInFlightKick)
		{
			waitTasksCompletion();
			m_HasPendingFlip = false;	// But don't bother flipping
		}

		// re-initing will clear displacement textures etc. and fill them with junk
		m_HasReadyKick = false;
	}

	return S_OK;
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::ensurePushBufferCapacity(size_t n)
{
	if(n > m_pushBufferCapacity)
	{
		SAFE_DELETE_ARRAY(m_pushBuffer);

		size_t new_capacity = m_pushBufferCapacity ? m_pushBufferCapacity : 1;
		while(new_capacity < n)
		{
			new_capacity <<= 1;
		}

		m_pushBuffer = new gfsdk_U32[new_capacity];
		m_pushBufferCapacity = new_capacity;
	}
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::kick(Graphics_Context* pGC, double dSimTime, gfsdk_U64& kickID)
{
	HRESULT hr;

	if(0 == m_Simulations.size())
		return S_OK;

	kickID = m_NextKickID;

	if(m_IsWaitingInFlightKick)
	{
		waitTasksCompletion();
		flip();
	}
	else if(m_HasPendingFlip)
	{
		flip();
	}
	else
	{
		// If there's no kick in flight, we don't call flip(), and therefore any previous results are still available
		// for rendering i.e. the staging cursor is unaffected
	}

	//map textures for all cascades and setup new tasks
	for(int i=0; i<m_Simulations.size(); i++)
	{
		NVWaveWorks_FFT_Simulation_CPU_Impl* c = m_Simulations[i];

		UINT N = c->GetParams().fft_resolution;

		SimulationTaskType kickOffTaskType = T_UPDATE_HT;
		int kickOffRowCount = int(N);
		if(c->IsH0UpdateRequired())
		{
			kickOffTaskType = T_UPDATE_H0;
			++kickOffRowCount;	// h0 wave vector space needs to be inclusive for the ht calc
		}

		//push all new tasks into queue
		ensurePushBufferCapacity(kickOffRowCount);
		for(int t=0; t<kickOffRowCount; t++)
		{
			m_pushBuffer[t]  = packTaskData(kickOffTaskType, i, t);
		}
		m_queue->push(m_pushBuffer, kickOffRowCount);

		V_RETURN(c->OnInitiateSimulationStep(pGC,dSimTime));

		// Kicking tasks is always guaranteed to update H0 if necessary, so clear the flag to make
		// sure the main-thread state is synchronized
		c->SetH0UpdateNotRequired();
	}

	//kick a thread to start work
	const bool alreadyDidTheWork = m_queue->kick(ProcessTaskFn, this);

	//track the kick
	m_IsWaitingInFlightKick = true;
	m_InFlightKickID = m_NextKickID;

	if(alreadyDidTheWork)
	{
		// If the queue clears 'immediately', the kick() effectively operated synchronously,
		// so flip here and now to make the results available for rendering immediately
		waitTasksCompletion();
		flip();
	}

	++m_NextKickID;

	return S_OK;
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::flip()
{
	assert(m_HasPendingFlip);

	//unmap and flip completed textures
	for(int ix = 0; ix != m_Simulations.size(); ++ix) {
		m_Simulations[ix]->OnCompleteSimulationStep(m_InFlightKickID);
	}

	m_HasPendingFlip = false;
	m_HasReadyKick = true;
	m_ReadyKickID = m_InFlightKickID;
}

void NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::waitTasksCompletion()
{
	assert(m_IsWaitingInFlightKick);

	TickType tStart,tStop;

	if(m_enable_CPU_timers)
	{
		// tying thread to core #0 to ensure OS doesn't reallocathe thread to other cores which might corrupt QueryPerformanceCounter readings
		GFSDK_WaveWorks_Simulation_Util::tieThreadToCore(0);
		// getting the timestamp
		tStart = GFSDK_WaveWorks_Simulation_Util::getTicks();
	}

	m_queue->waitForWorkDone();

	if(m_enable_CPU_timers)
	{
		// getting the timestamp and calculating time
		tStop = GFSDK_WaveWorks_Simulation_Util::getTicks();
		m_time_wait_for_tasks_completion = GFSDK_WaveWorks_Simulation_Util::getMilliseconds(tStart,tStop);
	}
	else
	{
		m_time_wait_for_tasks_completion = 0;
	}

	// the tasks completed, ergo there is no longer a kick in flight
	m_IsWaitingInFlightKick = false;
	m_HasPendingFlip = true;
}

bool NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::getStagingCursor(gfsdk_U64* pKickID)
{
	if(pKickID && m_HasReadyKick)
	{
		*pKickID = m_ReadyKickID;
	}

	return m_HasReadyKick;
}

NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::advanceStagingCursor(bool block)
{
	if(!m_IsWaitingInFlightKick && !m_HasPendingFlip)
	{
		// There may not be a kick in-flight. If not, the staging cursor cannot change during this call, so return
		// immediately
		return AdvanceCursorResult_None;
	}
	else if(m_IsWaitingInFlightKick && !block)
	{
		// Non-blocking call, so do a little peek ahead to test whether the tasks are complete and we can advance
		if(!m_queue->isWorkDone())
		{
			return AdvanceCursorResult_WouldBlock;
		}
	}

	// Wait for the pending work to complete
	if(m_IsWaitingInFlightKick)
	{
		waitTasksCompletion();
	}
	assert(m_HasPendingFlip);
	flip();

	return AdvanceCursorResult_Succeeded;
}

NVWaveWorks_FFT_Simulation_Manager::WaitCursorResult NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::waitStagingCursor()
{
	if(!m_IsWaitingInFlightKick)
	{
		// There may not be a kick in-flight. If not, the staging cursor cannot change during this call, so return
		// immediately
		return WaitCursorResult_None;
	}

	waitTasksCompletion();

	return WaitCursorResult_Succeeded;
}

bool NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::getReadbackCursor(gfsdk_U64* pKickID)
{
	if(pKickID && m_HasReadyKick)
	{
		*pKickID = m_ReadyKickID;
	}

	return m_HasReadyKick;
}

NVWaveWorks_FFT_Simulation_Manager::AdvanceCursorResult NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::advanceReadbackCursor(bool /*block*/)
{
	// The CPU pipeline makes results available for readback as soon as they are staged, so there are never any
	// readbacks in-flight
	return AdvanceCursorResult_None;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::archiveDisplacements()
{
	HRESULT hr;

	if(!m_HasReadyKick)
	{
		return E_FAIL;
	}

	for(NVWaveWorks_FFT_Simulation_CPU_Impl** pSim = m_Simulations.begin(); pSim != m_Simulations.end(); ++pSim)
	{
		V_RETURN((*pSim)->archiveDisplacements(m_ReadyKickID));
	}

	return S_OK;
}

HRESULT NVWaveWorks_FFT_Simulation_Manager_CPU_Impl::getTimings(GFSDK_WaveWorks_Simulation_Manager_Timings& timings)
{
	if(m_defaultQueue)
	{
		timings.time_start_to_stop = m_defaultQueue->getThreadsStartToStopTime();
		timings.time_total = m_defaultQueue->getThreadsWorkTotalTime();
	}
	else
	{
		timings.time_start_to_stop = 0.f;
		timings.time_total = 0.f;
	}

	timings.time_wait_for_completion = m_time_wait_for_tasks_completion;
	return S_OK;
}

#endif //SUPPORT_FFTCPU
