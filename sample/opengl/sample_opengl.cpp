// ocean_cufft_app.cpp : Defines the entry point for the console application.
//

#include "math_code.h"
#include "ocean_surface.h"
#include "GFSDK_WaveWorks.h"
#include <windows.h> // for QueryPerformanceFrequency/QueryPerformanceCounter
#include "AntTweakBar.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

OceanSurface*						g_pOceanSurf = NULL;
GFSDK_WaveWorks_SimulationHandle	g_hOceanSimulation = NULL;
GFSDK_WaveWorks_Simulation_Params	g_ocean_simulation_param;
GFSDK_WaveWorks_Simulation_Settings	g_ocean_simulation_settings;
GFSDK_WaveWorks_Quadtree_Params		g_ocean_param_quadtree;
GFSDK_WaveWorks_Quadtree_Stats		g_ocean_stats_quadtree;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation_filtered;
GFSDK_WAVEWORKS_GLFunctions         g_GLFunctions;
int									g_max_detail_level;
long								g_FPS;
long								g_Second;
long								g_FrameNumber;

HINSTANCE							g_hInstance;				// GL window class instance
HWND								g_hWnd;						// GL window class handle
HDC									g_hDC;						// GL window device context handle
HGLRC								g_hRC;						// GL rendering context
LRESULT	CALLBACK					WndProc(HWND, UINT, WPARAM, LPARAM);	// forward declaration For WndProc
bool								g_ExitApp = false;
bool								g_WindowActive = true;
MSG									g_Msg;						// Windows message structure
bool								g_PressedKeys[256];			// Array of pressed keys

TwBar*								g_pStatsBar;
TwBar*								g_pControlsBar;

// Controls
bool								g_Wireframe = false;
bool								g_SimulateWater = true;
bool								g_ShowStats = false;

// Callbacks for Controls AntTweakBar
static void TW_CALL CB_SetShowStats(const void *value, void * /*clientData*/)
{ 
	g_ShowStats = *(const bool *)value;  
	if(g_ShowStats)
	{
		TwDefine("Stats visible=true");
	}
	else
	{
		TwDefine("Stats visible=false");
	}
}
static void TW_CALL CB_GetShowStats(void *value, void * /*clientData*/)
{ 
	*(bool *)value = g_ShowStats;
}

static void TW_CALL CB_SetUseBeaufort(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_settings.use_Beaufort_scale = *(const bool *)value;  
	if(g_ocean_simulation_settings.use_Beaufort_scale)
	{
		g_ocean_simulation_param.wind_speed = min(12.0f, g_ocean_simulation_param.wind_speed);
		TwDefine("Controls/'Wind Speed' min=0 max = 12 step=0.1 precision=1");
		TwDefine("Controls/'Wave Amplitude' visible = false");
		TwDefine("Controls/'Chop Scale' visible = false");
	}
	else
	{
		TwDefine("Controls/'Wind Speed' min=0 max = 50 step=0.1 precision=1");
		TwDefine("Controls/'Wave Amplitude' visible = true");
		TwDefine("Controls/'Chop Scale' visible = true");
	}
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
}
static void TW_CALL CB_GetUseBeaufort(void *value, void * /*clientData*/)
{ 
	*(bool *)value = g_ocean_simulation_settings.use_Beaufort_scale;
}

static void TW_CALL CB_SetWaveAmplitude(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_param.wave_amplitude = *(const float *)value;  
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
}
static void TW_CALL CB_GetWaveAmplitude(void *value, void * /*clientData*/)
{ 
	*(float *)value = g_ocean_simulation_param.wave_amplitude;
}

static void TW_CALL CB_SetChopScale(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_param.choppy_scale = *(const float *)value;  
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
}
static void TW_CALL CB_GetChopScale(void *value, void * /*clientData*/)
{ 
	*(float *)value = g_ocean_simulation_param.choppy_scale;
}

static void TW_CALL CB_SetWindSpeed(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_param.wind_speed = *(const float *)value;  
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
}
static void TW_CALL CB_GetWindSpeed(void *value, void * /*clientData*/)
{ 
	*(float *)value = g_ocean_simulation_param.wind_speed;
}

static void TW_CALL CB_SetWindDependency(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_param.wind_dependency = *(const float *)value;  
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
}
static void TW_CALL CB_GetWindDependency(void *value, void * /*clientData*/)
{ 
	*(float *)value = g_ocean_simulation_param.wind_dependency;
}

static void TW_CALL CB_SetTimeScale(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_param.time_scale = *(const float *)value;  
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
}
static void TW_CALL CB_GetTimeScale(void *value, void * /*clientData*/)
{ 
	*(float *)value = g_ocean_simulation_param.time_scale;
}

static void TW_CALL CB_SetAnisoLevel(const void *value, void * /*clientData*/)
{ 
	g_ocean_simulation_settings.aniso_level = *(const gfsdk_U8 *)value;  
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
}
static void TW_CALL CB_GetAnisoLevel(void *value, void * /*clientData*/)
{ 
	*(gfsdk_U8 *)value = g_ocean_simulation_settings.aniso_level;
}

static void TW_CALL CB_SetDetailLevel(const void *value, void * /*clientData*/)
{ 
	if(GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_GL2(*(const GFSDK_WaveWorks_Simulation_DetailLevel *)value))
	{
		g_ocean_simulation_settings.detail_level = *(const GFSDK_WaveWorks_Simulation_DetailLevel *)value;  
		GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	}
}
static void TW_CALL CB_GetDetailLevel(void *value, void * /*clientData*/)
{ 
	*(gfsdk_U8 *)value = (gfsdk_U8)g_ocean_simulation_settings.detail_level;
}

// Helper functions
void PrintGLInfo(void)
{
   printf ("\nVendor: %s", glGetString (GL_VENDOR));
   printf ("\nRenderer: %s", glGetString (GL_RENDERER));
   printf ("\nVersion: %s", glGetString (GL_VERSION));
   printf ("\nGLSL: %s", glGetString (GL_SHADING_LANGUAGE_VERSION));
   checkError ("dumpInfo");
}

// Rendering functions
void RenderFrame(void)
{
	static LARGE_INTEGER freq, counter, old_counter;

	if(g_SimulateWater)
	do {
		GFSDK_WaveWorks_Simulation_SetTime(g_hOceanSimulation, g_pOceanSurf->total_time);
		GFSDK_WaveWorks_Simulation_KickGL2(g_hOceanSimulation, NULL);
	} while(gfsdk_waveworks_result_NONE==GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL));

	// getting simulation timings
	GFSDK_WaveWorks_Simulation_GetStats(g_hOceanSimulation,g_ocean_stats_simulation);

	// rendering
	g_pOceanSurf->Render(g_hOceanSimulation, g_ocean_simulation_settings, g_Wireframe);

	// getting quadtree stats
	GFSDK_WaveWorks_Quadtree_GetStats(g_pOceanSurf->hOceanQuadTree, g_ocean_stats_quadtree);

	// Drawing AntTweakBars
	TwDraw();
	SwapBuffers(g_hDC);				
	g_FrameNumber++;

	// timing
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);

	g_pOceanSurf->delta_time = (float)(((double)(counter.QuadPart) - (double)(old_counter.QuadPart))/(double)freq.QuadPart);
	g_pOceanSurf->total_time += g_pOceanSurf->delta_time;
	if(g_pOceanSurf->total_time>=36000.0f) g_pOceanSurf->total_time=0;
	old_counter = counter;
	
	if((long)(floor(g_pOceanSurf->total_time)) > g_Second)
	{
		g_FPS = g_FrameNumber/((long)(floor(g_pOceanSurf->total_time)) - g_Second);
		g_Second = (long)(ceil(g_pOceanSurf->total_time));
		g_FrameNumber = 0;
	}

	// exponential filtering for stats
	g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time			= g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time*0.98f + 0.02f*g_ocean_stats_simulation.CPU_main_thread_wait_time;
	g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time  = g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time*0.98f + 0.02f*g_ocean_stats_simulation.CPU_threads_start_to_finish_time;
	g_ocean_stats_simulation_filtered.CPU_threads_total_time			= g_ocean_stats_simulation_filtered.CPU_threads_total_time*0.98f + 0.02f*g_ocean_stats_simulation.CPU_threads_total_time;
	g_ocean_stats_simulation_filtered.GPU_simulation_time				= g_ocean_stats_simulation_filtered.GPU_simulation_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_simulation_time;
	g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time			= g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_FFT_simulation_time;
	g_ocean_stats_simulation_filtered.GPU_gfx_time						= g_ocean_stats_simulation_filtered.GPU_gfx_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_gfx_time;
	g_ocean_stats_simulation_filtered.GPU_update_time					= g_ocean_stats_simulation_filtered.GPU_update_time*0.98f + 0.02f*g_ocean_stats_simulation.GPU_update_time;
}

// Input functions
void ProcessWSADKeys()
{
	if(g_pOceanSurf)
		{
		float direction[3];
		float strafe[3];
		float up[3] = {0.0f,0.0f,1.0f};
	
		direction[0]=g_pOceanSurf->LookAtPosition[0] - g_pOceanSurf->CameraPosition[0];
		direction[1]=g_pOceanSurf->LookAtPosition[1] - g_pOceanSurf->CameraPosition[1];
		direction[2]=g_pOceanSurf->LookAtPosition[2] - g_pOceanSurf->CameraPosition[2];
		vec3Normalize(direction);
		vec3CrossProductNormalized(strafe,up,direction);

		if(g_PressedKeys[87]) //'w'
		{
			g_pOceanSurf->CameraPosition[0]+=0.5f*direction[0];
			g_pOceanSurf->CameraPosition[1]+=0.5f*direction[1];
			g_pOceanSurf->CameraPosition[2]+=0.5f*direction[2];
			g_pOceanSurf->LookAtPosition[0]+=0.5f*direction[0];
			g_pOceanSurf->LookAtPosition[1]+=0.5f*direction[1];
			g_pOceanSurf->LookAtPosition[2]+=0.5f*direction[2];
		}
		if(g_PressedKeys[83]) //'s'
		{
			g_pOceanSurf->CameraPosition[0]-=0.5f*direction[0];
			g_pOceanSurf->CameraPosition[1]-=0.5f*direction[1];
			g_pOceanSurf->CameraPosition[2]-=0.5f*direction[2];
			g_pOceanSurf->LookAtPosition[0]-=0.5f*direction[0];
			g_pOceanSurf->LookAtPosition[1]-=0.5f*direction[1];
			g_pOceanSurf->LookAtPosition[2]-=0.5f*direction[2];
		}
		if(g_PressedKeys[65]) //'a'
		{
			g_pOceanSurf->CameraPosition[0]+=0.5f*strafe[0];
			g_pOceanSurf->CameraPosition[1]+=0.5f*strafe[1];
			g_pOceanSurf->CameraPosition[2]+=0.5f*strafe[2];
			g_pOceanSurf->LookAtPosition[0]+=0.5f*strafe[0];
			g_pOceanSurf->LookAtPosition[1]+=0.5f*strafe[1];
			g_pOceanSurf->LookAtPosition[2]+=0.5f*strafe[2];
		}
		if(g_PressedKeys[68]) //'d'
		{
			g_pOceanSurf->CameraPosition[0]-=0.5f*strafe[0];
			g_pOceanSurf->CameraPosition[1]-=0.5f*strafe[1];
			g_pOceanSurf->CameraPosition[2]-=0.5f*strafe[2];
			g_pOceanSurf->LookAtPosition[0]-=0.5f*strafe[0];
			g_pOceanSurf->LookAtPosition[1]-=0.5f*strafe[1];
			g_pOceanSurf->LookAtPosition[2]-=0.5f*strafe[2];
		}
	}
}

void ProcessMouseMotion(int MouseX, int MouseY, int LButtonPressed)
{
	if(g_pOceanSurf)
	{
		float initial_direction[4]={1.0f,0.0f,0.0f,1.0f};
		float direction[4];
		float r1[4][4],r2[4][4],rotation[4][4];
	
		if(LButtonPressed)
		{
			g_pOceanSurf->MouseDX = (float)MouseX - (float)g_pOceanSurf->MouseX;
			g_pOceanSurf->MouseDY = (float)MouseY - (float)g_pOceanSurf->MouseY;
		}
		else
		{
			g_pOceanSurf->MouseDX = 0;
			g_pOceanSurf->MouseDY = 0;
		}
		g_pOceanSurf->MouseX = MouseX;
		g_pOceanSurf->MouseY = MouseY;


		g_pOceanSurf->Alpha+=g_pOceanSurf->MouseDX*0.002f;
		g_pOceanSurf->Beta+=g_pOceanSurf->MouseDY*0.002f;
		if(g_pOceanSurf->Beta>PI*0.49f)g_pOceanSurf->Beta = PI*0.49f;
		if(g_pOceanSurf->Beta<-PI*0.49f)g_pOceanSurf->Beta = -PI*0.49f;
		mat4CreateRotation(r1,g_pOceanSurf->Alpha,'z');
		mat4CreateRotation(r2,-g_pOceanSurf->Beta,'y');
		mat4Mat4Mul(rotation,r1,r2);
		vec4Mat4Mul(direction,initial_direction,rotation);
		g_pOceanSurf->LookAtPosition[0]=g_pOceanSurf->CameraPosition[0]+direction[0];
		g_pOceanSurf->LookAtPosition[1]=g_pOceanSurf->CameraPosition[1]+direction[1];
		g_pOceanSurf->LookAtPosition[2]=g_pOceanSurf->CameraPosition[2]+direction[2];
	}
}

// GL Window related functions
void KillGLWindow(void)								
{
	// do we have a rendering context?
	if (g_hRC)											
	{
		// are we able to release the DC and RC?
		if (!wglMakeCurrent(NULL,NULL))					
		{
			MessageBox(NULL,L"Release of DC and RC failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		// are we able to delete the RC?
		if (!wglDeleteContext(g_hRC))						
		{
			MessageBox(NULL,L"Release rendering context failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		g_hRC = NULL;										
	}

	// are we able to release the DC
	if (g_hDC && !ReleaseDC(g_hWnd,g_hDC))					
	{
		MessageBox(NULL,L"Release device context failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		g_hDC = NULL;
	}

	// are we able to destroy the window?
	if (g_hWnd && !DestroyWindow(g_hWnd))					
	{
		MessageBox(NULL,L"Could not release window handle.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		g_hWnd = NULL;	
	}

	// are we able to unregister class?
	if (!UnregisterClass(L"OpenGL",g_hInstance))
	{
		MessageBox(NULL,L"Could not unregister class.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		g_hInstance = NULL;	
	}
}

BOOL CreateGLWindow(LPCWSTR title, int width, int height, byte bpp)
{
	GLuint		PixelFormat;			// holds the results after searching for a match
	WNDCLASS	wc;						// windows class structure
	DWORD		dwExStyle;				// window extended style
	DWORD		dwStyle;				// window style
	RECT		WindowRect;				// grabs rectangle upper left / lower right values
	WindowRect.left=(long)0;			// set left value to 0
	WindowRect.right=(long)width;		// set right value to requested width
	WindowRect.top=(long)0;				// set top value to 0
	WindowRect.bottom=(long)height;		// set bottom value to requested height

	g_hInstance			= GetModuleHandle(NULL);				// grab an instance for our window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// redraw on size, and own DC for window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc handles messages
	wc.cbClsExtra		= 0;									// no extra window data
	wc.cbWndExtra		= 0;									// no extra window data
	wc.hInstance		= g_hInstance;							// set the instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// load the default icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// load the arrow pointer
	wc.hbrBackground	= NULL;									// no background required for GL
	wc.lpszMenuName		= NULL;									// we don't want a menu
	wc.lpszClassName	= L"OpenGL";							// set the class name
	
	// attempt to register the window class
	if (!RegisterClass(&wc))									
	{
		MessageBox(NULL,L"Failed to register the window class.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			
	dwStyle = WS_OVERLAPPEDWINDOW;							
	
	// adjust window to true requested size
	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		

	// create the window
	g_hWnd = CreateWindowEx(dwExStyle,									// extended style for the window
									L"OpenGL",							// class name
									title,								// window title
									dwStyle |							// defined window style
									WS_CLIPSIBLINGS |					// required window style
									WS_CLIPCHILDREN,					// required window style
									0, 0,								// window position
									WindowRect.right-WindowRect.left,	// calculate window width
									WindowRect.bottom-WindowRect.top,	// calculate window height
									NULL,								// no parent window
									NULL,								// no menu
									g_hInstance,						// instance
									NULL);								// dont pass anything to WM_CREATE
	if (!g_hWnd)								
	{
		KillGLWindow();								
		MessageBox(NULL,L"Window creation error.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								
	}

	static	PIXELFORMATDESCRIPTOR pfd =				
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// size of this pixel format descriptor
		1,											// version number
		PFD_DRAW_TO_WINDOW |						// format must support window
		PFD_SUPPORT_OPENGL |						// format must support OpenGL
		PFD_DOUBLEBUFFER,							// must support double buffering
		PFD_TYPE_RGBA,								// request an RGBA format
		bpp,										// select our color depth
		0, 0, 0, 0, 0, 0,							// color bits ignored
		0,											// no alpha buffer
		0,											// shift bit ignored
		0,											// no accumulation buffer
		0, 0, 0, 0,									// accumulation bits ignored
		32,											// 32bit z-buffer (depth buffer)  
		0,											// no stencil buffer
		0,											// no auxiliary buffer
		PFD_MAIN_PLANE,								// main drawing layer
		0,											// reserved
		0, 0, 0										// layer masks ignored
	};
	
	// did we get a device context?
	g_hDC = GetDC(g_hWnd);
	if (!g_hDC)							
	{
		KillGLWindow();								
		MessageBox(NULL,L"Can't create a GL device context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								
	}
	
	// did windows find a matching pixel format?
	PixelFormat = ChoosePixelFormat(g_hDC,&pfd);
	if (!PixelFormat)	
	{
		KillGLWindow();								
		MessageBox(NULL,L"Can't find a suitable pixelformat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								
	}
	
	// are we able to set the pixel format?
	if(!SetPixelFormat(g_hDC,PixelFormat,&pfd))		
	{
		KillGLWindow();								
		MessageBox(NULL,L"Can't set the pixelformat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								
	}
	
	// are we able to get a rendering context?
	g_hRC=wglCreateContext(g_hDC);
	if (!g_hRC)				
	{
		KillGLWindow();								
		MessageBox(NULL,L"Can't create a GL rendering context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								
	}

	// try to activate the rendering context
	if(!wglMakeCurrent(g_hDC,g_hRC))					
	{
		KillGLWindow();								
		MessageBox(NULL,L"Can't activate the GL rendering context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								
	}

	ShowWindow(g_hWnd,SW_SHOW);						
	SetForegroundWindow(g_hWnd);						
	SetFocus(g_hWnd);									
	return TRUE;									
}


int main(void)
{
	gfsdk_waveworks_result res;
	if(!CreateGLWindow(L"sample_opengl", 1280, 720, 32))
	{
		return 0;
	}

	PrintGLInfo();

	// Creating OceanSurface instance

	g_ocean_simulation_settings.fft_period						= 1000.0f;
	g_ocean_simulation_settings.detail_level					= (GFSDK_WaveWorks_Simulation_DetailLevel)g_max_detail_level;
	g_ocean_simulation_settings.readback_displacements			= false;
	g_ocean_simulation_settings.num_readback_FIFO_entries		= 0;
	g_ocean_simulation_settings.aniso_level						= 4;
	g_ocean_simulation_settings.CPU_simulation_threading_model  = GFSDK_WaveWorks_Simulation_CPU_Threading_Model_Automatic;
	g_ocean_simulation_settings.use_Beaufort_scale				= true;
	g_ocean_simulation_settings.num_GPUs						= 1;
	g_ocean_simulation_settings.use_texture_arrays				= true;
	g_ocean_simulation_settings.enable_CUDA_timers				= true;

	g_pOceanSurf = new OceanSurface(g_ocean_simulation_settings.use_texture_arrays);

	// Initializing GL functions to be passed to WaveWorks, in alphabetic order
	g_GLFunctions.glGetError =  g_pOceanSurf->glGetError;
	g_GLFunctions.glGetIntegerv = g_pOceanSurf->glGetIntegerv;
	g_GLFunctions.glTexParameterf = g_pOceanSurf->glTexParameterf;
	g_GLFunctions.glTexParameteri = g_pOceanSurf->glTexParameteri;
	g_GLFunctions.glTexImage2D = g_pOceanSurf->glTexImage2D;
	g_GLFunctions.glTexSubImage2D = g_pOceanSurf->glTexSubImage2D;
	g_GLFunctions.glClear = g_pOceanSurf->glClear;
	g_GLFunctions.glClearColor = g_pOceanSurf->glClearColor;
	g_GLFunctions.glColorMask = g_pOceanSurf->glColorMask;
	g_GLFunctions.glDisable = g_pOceanSurf->glDisable;
	g_GLFunctions.glViewport = g_pOceanSurf->glViewport;
	g_GLFunctions.glBindTexture = g_pOceanSurf->glBindTexture;
	g_GLFunctions.glDeleteTextures = g_pOceanSurf->glDeleteTextures;
	g_GLFunctions.glGenTextures = g_pOceanSurf->glGenTextures;
	g_GLFunctions.glDrawElements = g_pOceanSurf->glDrawElements;
	g_GLFunctions.glActiveTexture = g_pOceanSurf->glActiveTexture;
	g_GLFunctions.glBindBuffer = g_pOceanSurf->glBindBuffer;
	g_GLFunctions.glDeleteBuffers = g_pOceanSurf->glDeleteBuffers;
	g_GLFunctions.glGenBuffers = g_pOceanSurf->glGenBuffers;
	g_GLFunctions.glBufferData = g_pOceanSurf->glBufferData;
	g_GLFunctions.glMapBufferRange = g_pOceanSurf->glMapBufferRange;
	g_GLFunctions.glUnmapBuffer = g_pOceanSurf->glUnmapBuffer;
	g_GLFunctions.glAttachShader = g_pOceanSurf->glAttachShader;
	g_GLFunctions.glCompileShader = g_pOceanSurf->glCompileShader;
	g_GLFunctions.glCreateProgram = g_pOceanSurf->glCreateProgram;
	g_GLFunctions.glCreateShader = g_pOceanSurf->glCreateShader;
	g_GLFunctions.glDeleteProgram = g_pOceanSurf->glDeleteProgram;
	g_GLFunctions.glDeleteShader = g_pOceanSurf->glDeleteShader;
	g_GLFunctions.glDisableVertexAttribArray = g_pOceanSurf->glDisableVertexAttribArray;
	g_GLFunctions.glEnableVertexAttribArray = g_pOceanSurf->glEnableVertexAttribArray;
	g_GLFunctions.glGetAttribLocation = g_pOceanSurf->glGetAttribLocation;
	g_GLFunctions.glGetProgramiv = g_pOceanSurf->glGetProgramiv;
	g_GLFunctions.glGetProgramInfoLog = g_pOceanSurf->glGetProgramInfoLog;
	g_GLFunctions.glGetShaderiv = g_pOceanSurf->glGetShaderiv;
	g_GLFunctions.glGetShaderInfoLog = g_pOceanSurf->glGetShaderInfoLog;
	g_GLFunctions.glGetUniformLocation = g_pOceanSurf->glGetUniformLocation;
	g_GLFunctions.glLinkProgram = g_pOceanSurf->glLinkProgram;
	g_GLFunctions.glShaderSource = g_pOceanSurf->glShaderSource;
	g_GLFunctions.glUseProgram = g_pOceanSurf->glUseProgram;
	g_GLFunctions.glUniform1f = g_pOceanSurf->glUniform1f;
	g_GLFunctions.glUniform1i = g_pOceanSurf->glUniform1i;
	g_GLFunctions.glUniform3fv = g_pOceanSurf->glUniform3fv;
	g_GLFunctions.glUniform4fv = g_pOceanSurf->glUniform4fv;
	g_GLFunctions.glVertexAttribPointer = g_pOceanSurf->glVertexAttribPointer;
	g_GLFunctions.glUniformMatrix3x4fv = g_pOceanSurf->glUniformMatrix3x4fv;
	g_GLFunctions.glBindFramebuffer = g_pOceanSurf->glBindFramebuffer;
	g_GLFunctions.glDeleteFramebuffers = g_pOceanSurf->glDeleteFramebuffers;
	g_GLFunctions.glGenFramebuffers = g_pOceanSurf->glGenFramebuffers;
	g_GLFunctions.glCheckFramebufferStatus = g_pOceanSurf->glCheckFramebufferStatus;
	g_GLFunctions.glGenerateMipmap = g_pOceanSurf->glGenerateMipmap;
	g_GLFunctions.glFramebufferTexture2D = g_pOceanSurf->glFramebufferTexture2D;
	g_GLFunctions.glPatchParameteri = g_pOceanSurf->glPatchParameteri;
	g_GLFunctions.glGenQueries = g_pOceanSurf->glGenQueries;
	g_GLFunctions.glDeleteQueries = g_pOceanSurf->glDeleteQueries;
	g_GLFunctions.glQueryCounter = g_pOceanSurf->glQueryCounter;
	g_GLFunctions.glGetQueryObjectui64v = g_pOceanSurf->glGetQueryObjectui64v;
	g_GLFunctions.glGetActiveAttrib = g_pOceanSurf->glGetActiveAttrib;
	g_GLFunctions.glFramebufferTextureLayer = g_pOceanSurf->glFramebufferTextureLayer;
	g_GLFunctions.glBlitFramebuffer = g_pOceanSurf->glBlitFramebuffer;
	g_GLFunctions.glTexImage3D = g_pOceanSurf->glTexImage3D;
	g_GLFunctions.glReadBuffer = g_pOceanSurf->glReadBuffer;
	g_GLFunctions.glDrawBuffers = g_pOceanSurf->glDrawBuffers;


	// initializing WaveWorks
	fprintf(stdout, "\nInitializing:\n");
	res = GFSDK_WaveWorks_InitGL2(&g_GLFunctions, NULL, GFSDK_WAVEWORKS_API_GUID);
	if(res == gfsdk_waveworks_result_OK)
	{
		fprintf(stdout, "GFSDK_WaveWorks_InitGL2: OK\n");
	}
	else
	{
		fprintf(stdout, "GFSDK_WaveWorks_InitGL2 ERROR: %i, exiting..\n", res);
		return res;
	}

	// initializing QuadTree
	g_ocean_param_quadtree.min_patch_length		= 40.0f;
	g_ocean_param_quadtree.upper_grid_coverage	= 64.0f;
	g_ocean_param_quadtree.mesh_dim				= 128;
	g_ocean_param_quadtree.sea_level			= 0.f; 
	g_ocean_param_quadtree.auto_root_lod		= 10;
	g_ocean_param_quadtree.tessellation_lod		= 100.0f;
	g_ocean_param_quadtree.geomorphing_degree	= 1.f;
	g_ocean_param_quadtree.enable_CPU_timers	= true;
	res = g_pOceanSurf->InitQuadTree(g_ocean_param_quadtree);
	if(res == gfsdk_waveworks_result_OK)
	{
		fprintf(stdout, "InitQuadTree: OK\n");
	}
	else
	{
		fprintf(stdout, "InitQuadTree ERROR: %i, exiting..\n", res);
		return res;
	}

	// checking available detail level
	int detail_level = 0;
	for(; detail_level != Num_GFSDK_WaveWorks_Simulation_DetailLevels; ++detail_level) {
		if(!GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_GL2((GFSDK_WaveWorks_Simulation_DetailLevel)detail_level))
			break;
	}
	if (0 == detail_level)
	{
		fprintf(stdout, "Fatal Error: No supported detail levels. \n");
		return false;
	}

	g_max_detail_level = (GFSDK_WaveWorks_Simulation_DetailLevel)(detail_level - 1);

	// initializing simulation
	g_ocean_simulation_param.time_scale				= 0.5f;
	g_ocean_simulation_param.wave_amplitude			= 1.0f;
	g_ocean_simulation_param.wind_dir.x				= 0.8f;
	g_ocean_simulation_param.wind_dir.y				= 0.6f;
	g_ocean_simulation_param.wind_speed				= 9.0f;
	g_ocean_simulation_param.wind_dependency		= 0.98f;
	g_ocean_simulation_param.choppy_scale			= 1.f;
	g_ocean_simulation_param.small_wave_fraction	= 0.f;
	g_ocean_simulation_param.foam_dissipation_speed	= 0.6f;
	g_ocean_simulation_param.foam_falloff_speed		= 0.985f;
	g_ocean_simulation_param.foam_generation_amount	= 0.12f;
	g_ocean_simulation_param.foam_generation_threshold = 0.37f;

	res = GFSDK_WaveWorks_Simulation_CreateGL2(g_ocean_simulation_settings, g_ocean_simulation_param, (void*) g_hRC, &g_hOceanSimulation);
	if(res == gfsdk_waveworks_result_OK)
	{
		fprintf(stdout, "GFSDK_WaveWorks_Simulation_CreateGL2: OK\n");
	}
	else
	{
		fprintf(stdout, "GFSDK_WaveWorks_Simulation_CreateGL2 ERROR: %i, exiting..\n", res);
		return res;
	}
	GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));

	// initializing AntTweakBars
	if(TwInit(TW_OPENGL, NULL)==0)
	{
		fprintf (stdout, "Error initializing AntTweakBar library, exiting..\n");
		// Cleanup WaveWorks
		delete g_pOceanSurf;
		GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
		GFSDK_WaveWorks_ReleaseGL2();

		// shutting down
		KillGLWindow();									
		return -1;							
	}
	TwWindowSize(1280, 720);

	// Status bar
	g_pStatsBar = TwNewBar("Stats");
	TwDefine(" Stats help='Times listed in Stats are Milliseconds'");
	TwDefine(" Stats size='400 200'");
	TwDefine(" Stats position='10 10'");
	TwDefine(" Stats alpha=0");
	TwDefine(" Stats movable=false");
	TwDefine(" Stats iconifiable=false");
	TwDefine(" Stats resizable=false");
	TwDefine(" Stats refresh=0.1");
	TwDefine(" Stats visible=false");
	TwAddVarRO(g_pStatsBar, "Frames Per Second", TW_TYPE_INT32, &g_FPS, "");
	TwAddVarRO(g_pStatsBar, "Quadtree Patches Drawn", TW_TYPE_INT32, &g_ocean_stats_quadtree.num_patches_drawn, "");
	TwAddSeparator(g_pStatsBar, NULL, NULL);
	TwAddVarRO(g_pStatsBar, "CPU: Main Thread Wait Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.CPU_main_thread_wait_time, "precision=2");
	TwAddVarRO(g_pStatsBar, "CPU: Threads Start To Finish Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.CPU_threads_start_to_finish_time, "precision=2");
	TwAddVarRO(g_pStatsBar, "CPU: Threads Total Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.CPU_threads_total_time, "precision=2");
	TwAddVarRO(g_pStatsBar, "GPU: FFT Simulation Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.GPU_FFT_simulation_time, "precision=2");
	TwAddVarRO(g_pStatsBar, "GPU: GFX Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.GPU_gfx_time, "precision=2");
	TwAddVarRO(g_pStatsBar, "GPU: Simulation Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.GPU_simulation_time, "precision=2");
	TwAddVarRO(g_pStatsBar, "CPU: Update Time", TW_TYPE_FLOAT, &g_ocean_stats_simulation_filtered.GPU_update_time, "precision=2");

	// Controls bar
	g_pControlsBar = TwNewBar("Controls");
	TwDefine(" Controls size='300 700'"); 
	TwDefine(" Controls position='970 10'");
	TwDefine(" Controls alpha=0");
	TwDefine(" Controls movable=false");
	TwDefine(" Controls iconifiable=false");
	TwDefine(" Controls resizable=false");
	TwDefine(" Controls valueswidth=100");
	TwAddVarRW(g_pControlsBar, "Wireframe", TW_TYPE_BOOLCPP, &g_Wireframe, NULL);
	TwAddVarRW(g_pControlsBar, "Simulate Water", TW_TYPE_BOOLCPP, &g_SimulateWater, NULL);
	TwAddVarCB(g_pControlsBar, "Show Stats", TW_TYPE_BOOLCPP, CB_SetShowStats, CB_GetShowStats, &g_ShowStats, NULL);
	TwAddSeparator(g_pControlsBar, NULL, NULL);
	TwAddVarCB(g_pControlsBar, "Use Beaufort Presets", TW_TYPE_BOOLCPP, CB_SetUseBeaufort, CB_GetUseBeaufort, &g_ocean_simulation_settings.use_Beaufort_scale, NULL);
	TwAddVarCB(g_pControlsBar, "Wave Amplitude", TW_TYPE_FLOAT, CB_SetWaveAmplitude, CB_GetWaveAmplitude, &g_ocean_simulation_param.wave_amplitude, "min=0 max=2 step=0.1 precision=1 visible=false");
	TwAddVarCB(g_pControlsBar, "Chop Scale", TW_TYPE_FLOAT, CB_SetChopScale, CB_GetChopScale, &g_ocean_simulation_param.choppy_scale, "min=0 max=1 step=0.1 precision=1 visible=false");
	TwAddVarCB(g_pControlsBar, "Wind Speed", TW_TYPE_FLOAT, CB_SetWindSpeed, CB_GetWindSpeed, &g_ocean_simulation_param.wind_speed, "min=0 max = 12 step=0.1 precision=1");
	TwAddVarCB(g_pControlsBar, "Wind Dependency", TW_TYPE_FLOAT, CB_SetWindDependency, CB_GetWindDependency, &g_ocean_simulation_param.wind_dependency, "min=0 max=1 step=0.1 precision=1");
	TwAddVarCB(g_pControlsBar, "Time Scale", TW_TYPE_FLOAT, CB_SetTimeScale, CB_GetTimeScale, &g_ocean_simulation_param.time_scale, "min=0.1 max=2 step=0.1 precision=1");
	TwAddVarCB(g_pControlsBar, "Aniso Level", TW_TYPE_INT8, CB_SetAnisoLevel, CB_GetAnisoLevel, &g_ocean_simulation_settings.aniso_level, "min=1 max=16 step=1");
	TwAddSeparator(g_pControlsBar, NULL, NULL);
	TwEnumVal QualityEV[] = { {GFSDK_WaveWorks_Simulation_DetailLevel_Normal, "Normal"}, {GFSDK_WaveWorks_Simulation_DetailLevel_High, "High"}, {GFSDK_WaveWorks_Simulation_DetailLevel_Extreme, "Extreme"}};
	TwType qualityType;
	qualityType = TwDefineEnum("Simulation Detail Level", QualityEV, 3);
	TwAddVarCB(g_pControlsBar, "Simulation Detail Level", qualityType, CB_SetDetailLevel, CB_GetDetailLevel, &g_ocean_simulation_settings.detail_level, NULL);

	fprintf(stdout, "Entering main loop\n", res);

	// entering main loop
	while(!g_ExitApp)									
	{
		if (PeekMessage(&g_Msg,NULL,0,0,PM_REMOVE))	
		{
			if (g_Msg.message==WM_QUIT)				
			{
				g_ExitApp=TRUE;						
			}
			else									
			{
				TranslateMessage(&g_Msg);			
				DispatchMessage(&g_Msg);			
			}
		}
		else										
		{
			// draw the scene if there are no messages
			if (g_WindowActive)						
			{
				if (g_PressedKeys[VK_ESCAPE])							
				{
					g_ExitApp = TRUE;					
				}
				else								
				{
					ProcessWSADKeys();
					RenderFrame();
					// updating stats string for Stats AntTweakBar
					
				}
			}
		}
	}

	// Cleanup AntTweakBars
	TwTerminate();

	// Cleanup WaveWorks
	delete g_pOceanSurf;
	GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
	GFSDK_WaveWorks_ReleaseGL2();

	// shutting down
	KillGLWindow();									
	return (int)g_Msg.wParam;							
}


// message processing function
LRESULT CALLBACK WndProc(HWND hWnd,	UINT uMsg, WPARAM wParam, LPARAM lParam)			
{

	// pass messages to AntTweakBars
    if( TwEventWin(hWnd, uMsg, wParam, lParam) ) // send event message to AntTweakBar
       return 0; // event has been handled by AntTweakBar

	switch (uMsg)									
	{
		case WM_ACTIVATE:							// watch for window activate message
		{
			if (!HIWORD(wParam))					// check minimization state
			{
				g_WindowActive = TRUE;				// program is active
			}
			else
			{
				g_WindowActive = FALSE;				// program is no longer active
			}
			return 0;								// return to the message loop
		}

		case WM_SYSCOMMAND:							
		{
			switch (wParam)							
			{
				case SC_SCREENSAVE:					// screensaver trying to start?
				case SC_MONITORPOWER:				// monitor trying to enter powersave?
				return 0;							// prevent from happening
			}
			break;									
		}

		case WM_CLOSE:								
		{
			g_ExitApp = true;
			PostQuitMessage(0);						
			return 0;								
		}

		case WM_KEYDOWN:							
		{
			g_PressedKeys[wParam] = TRUE;		
			switch(wParam)
			{
				case 219: // "["
					g_ocean_simulation_settings.detail_level = GFSDK_WaveWorks_Simulation_DetailLevel((g_ocean_simulation_settings.detail_level + g_max_detail_level) % (g_max_detail_level+1));
					GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
					GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
					break;
				case 221: // "]"
					g_ocean_simulation_settings.detail_level = GFSDK_WaveWorks_Simulation_DetailLevel((g_ocean_simulation_settings.detail_level + 1) % (g_max_detail_level+1));
					GFSDK_WaveWorks_Simulation_UpdateProperties(g_hOceanSimulation, g_ocean_simulation_settings, g_ocean_simulation_param);
					GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
					break;
				default:
					break;
			}
			return 0;								
		}

		case WM_KEYUP:								
		{
			g_PressedKeys[wParam] = FALSE;					
			return 0;								
		}

		case WM_SIZE:								
		{
			if(g_pOceanSurf)
			{
				g_pOceanSurf->ScreenWidth = LOWORD(lParam);
				g_pOceanSurf->ScreenHeight = HIWORD(lParam);
				TwWindowSize(g_pOceanSurf->ScreenWidth, g_pOceanSurf->ScreenHeight);
				int pos[2] = {g_pOceanSurf->ScreenWidth - 300 - 10, 10};
				int size[2] = {300, g_pOceanSurf->ScreenHeight - 20};
				TwSetParam(g_pControlsBar,NULL,"position",TW_PARAM_INT32, 2, &pos);
				TwSetParam(g_pControlsBar,NULL,"size",TW_PARAM_INT32, 2, &size);
			}
			return 0;	
		}

		case WM_MOUSEMOVE:
		{
			ProcessMouseMotion(LOWORD(lParam), HIWORD(lParam), (wParam & MK_LBUTTON) > 0? true:false);
		}
	}
	
	// pass all remaining messages to DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
