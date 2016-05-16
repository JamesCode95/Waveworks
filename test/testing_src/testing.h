#ifndef TESTING_H
#define TESTING_H

#include <string>
#include <vector>
#include "..\include\GFSDK_WaveWorks_Types.h"
#include "..\include\GFSDK_WaveWorks.h"
#include <stdio.h>

struct Utility
{
	static void split(const std::string& s, char c, std::vector<std::string>& v) 
	{
		using namespace std;

		string::size_type i = 0;
		string::size_type j = s.find(c);

		while (j != string::npos) 
		{
			v.push_back(s.substr(i, j-i));
			i = ++j;
			j = s.find(c, j);

			if (j == string::npos)
			{
				v.push_back(s.substr(i, s.length( )));
			}
		}
	}

	static bool writeTGAFile(const char *filename, int width, int height, unsigned char *imageData)
	{
		FILE* f = fopen(filename, "wb");

		if (!f) return false;

		unsigned char TGAheader[12] = {0,0,2,0,0,0,0,0,0,0,0,0};
		unsigned char header[6] = {((unsigned char)(width%256)), ((unsigned char)(width/256)), ((unsigned char)(height%256)), ((unsigned char)(height/256)),24,0};

		fwrite(TGAheader, sizeof(unsigned char), 12, f);
		fwrite(header, sizeof(unsigned char), 6, f);
		fwrite(imageData, sizeof(unsigned char), width*height*3, f);

		fclose(f);

		return true;
	}
};

struct TestParams
{
	bool UseReadbacks;
	GFSDK_WaveWorks_Simulation_DetailLevel QualityMode;
	std::string ScreenshotDirectory;
	std::string MediaDirectory;

	explicit TestParams(std::string strCommandLine)
		: UseReadbacks(false)
		, QualityMode( GFSDK_WaveWorks_Simulation_DetailLevel_Normal )

		, m_hOceanSimulation(NULL)
		, m_isTestingComplete(false)
		, m_shouldTakeScreen(false)
		, m_allowAA(true)
		, m_frameIndex(0)
		, m_windowStart(-1)
		, m_windowEnd(-1)
		, m_sliceFrame(-1)
	{
		using namespace std;

		// First split command line by into argument groups (- delimited).
		vector<string> args;
		Utility::split( strCommandLine, '-', args );

		const unsigned int iArgStart = 1;
		for( unsigned int iArg = iArgStart; iArg < args.size() ; iArg++ )
		{
			// Split single argument into parameters (space delimited)
			vector<string> keyValuePair;
			Utility::split( args[iArg], ' ', keyValuePair );

			if(keyValuePair.size() != 0)
			{
				// Handle flag args
				if( keyValuePair[0] == "readback" )
				{
					UseReadbacks = stoi( keyValuePair[1] ) == 0 ? false : true;
				}
				else if( keyValuePair[0] == "quality" )
				{
					QualityMode = (GFSDK_WaveWorks_Simulation_DetailLevel)(stoi( keyValuePair[1] ));
				}
				else if( keyValuePair[0] == "startframe" )
				{
					m_windowStart = stoi( keyValuePair[1] );
				}
				else if( keyValuePair[0] == "endframe" )
				{
					m_windowEnd = stoi( keyValuePair[1] );
				}				
				else if( keyValuePair[0] == "sliceframe" )
				{
					m_sliceFrame = stoi( keyValuePair[1] );
				}
				else if( keyValuePair[0] == "screenshot" )
				{
					m_shouldTakeScreen = true;
					ScreenshotDirectory = std::string(keyValuePair[1]);
				}
				else if (keyValuePair[0] == "mediadir")
				{
					MediaDirectory = std::string(keyValuePair[1]);
				}
				else if( keyValuePair[0] == "noaa" )
				{
					m_allowAA = false;
				}
			}
		}

		m_isTestingComplete = m_frameIndex > m_sliceFrame && m_frameIndex > m_windowStart && m_frameIndex > m_windowEnd;
	}

	void HookSimulation(GFSDK_WaveWorks_SimulationHandle handle) 
	{ 
		m_hOceanSimulation = handle; 
	}

	bool IsTestingComplete() const		{ return m_isTestingComplete; }
	bool ShouldTakeScreenshot() const	{ return m_shouldTakeScreen; }
	bool AllowAA() const				{ return m_allowAA; }

	void Tick()
	{
		bool withinAverageWindow = (m_windowStart!=-1) && (m_windowEnd!=-1) && (m_frameIndex>=m_windowStart) && (m_frameIndex<m_windowEnd);

		if(m_hOceanSimulation != NULL && m_sliceFrame != -1 && m_frameIndex == m_sliceFrame )
		{
			GFSDK_WaveWorks_Simulation_GetStats(m_hOceanSimulation, m_sliceStats);
		}

		if( m_hOceanSimulation != NULL && withinAverageWindow )
		{
			GFSDK_WaveWorks_Simulation_Stats stats;

			GFSDK_WaveWorks_Simulation_GetStats(m_hOceanSimulation, stats);

			m_averageStats.CPU_main_thread_wait_time		+= stats.CPU_main_thread_wait_time;
			m_averageStats.CPU_threads_start_to_finish_time += stats.CPU_threads_start_to_finish_time;
			m_averageStats.CPU_threads_total_time			+= stats.CPU_threads_total_time;
			m_averageStats.GPU_simulation_time				+= stats.GPU_simulation_time;
			m_averageStats.GPU_FFT_simulation_time			+= stats.GPU_FFT_simulation_time;
			m_averageStats.GPU_gfx_time						+= stats.GPU_gfx_time;
			m_averageStats.GPU_update_time					+= stats.GPU_update_time;
		}

		if(m_hOceanSimulation != NULL && m_windowEnd != -1 && m_frameIndex == m_windowEnd)
		{
			const int kAverageWindowSize = m_windowEnd - m_windowStart;
			m_averageStats.CPU_main_thread_wait_time		/= kAverageWindowSize;
			m_averageStats.CPU_threads_start_to_finish_time /= kAverageWindowSize;
			m_averageStats.CPU_threads_total_time			/= kAverageWindowSize;
			m_averageStats.GPU_simulation_time				/= kAverageWindowSize;
			m_averageStats.GPU_FFT_simulation_time			/= kAverageWindowSize;
			m_averageStats.GPU_gfx_time						/= kAverageWindowSize;
			m_averageStats.GPU_update_time					/= kAverageWindowSize;
		}

		m_isTestingComplete = m_frameIndex > m_sliceFrame && m_frameIndex > m_windowStart && m_frameIndex > m_windowEnd;

		++m_frameIndex;
	}

private:
	GFSDK_WaveWorks_SimulationHandle m_hOceanSimulation;

	GFSDK_WaveWorks_Simulation_Stats m_sliceStats;
	GFSDK_WaveWorks_Simulation_Stats m_averageStats;

	bool m_isTestingComplete;
	bool m_shouldTakeScreen;
	bool m_allowAA;

	int m_frameIndex;
	int m_windowStart;
	int m_windowEnd;
	int m_sliceFrame;

};

#endif // TESTING_H