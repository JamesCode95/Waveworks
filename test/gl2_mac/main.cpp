// ocean_cufft_app.cpp : Defines the entry point for the console application.
//



#include "math_code.h"
#include "ocean_surface.h"
#include "GFSDK_WaveWorks.h"
#include <string>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

OceanSurface*						g_pOceanSurf = NULL;
GFSDK_WaveWorks_SimulationHandle	g_hOceanSimulation = NULL;
GFSDK_WaveWorks_Simulation_Params	g_ocean_simulation_param;
GFSDK_WaveWorks_Simulation_Settings	g_ocean_simulation_settings;
GFSDK_WaveWorks_Quadtree_Params		g_ocean_param_quadtree;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation;
GFSDK_WaveWorks_Quadtree_Stats      g_ocean_stats_quadtree;
GFSDK_WaveWorks_Simulation_Stats    g_ocean_stats_simulation_filtered;
GFSDK_WAVEWORKS_GLFunctions         g_GLFunctions;
int									g_max_detail_level;

bool								g_ExitApp = false;
bool								g_WindowActive = true;
bool								g_PressedKeys[256];				// Array of pressed keys
int                                 g_GLUTWindowHandle;
long long                           g_Microseconds;
long long                           g_OldMicroseconds;

int WindowWidth	= 1280;
int WindowHeight = 720;
bool LButtonPressed = false;


void PrintGLInfo(void)
{
    fprintf (stdout, "\nVendor: %s", glGetString (GL_VENDOR));
    fprintf (stdout, "\nRenderer: %s", glGetString (GL_RENDERER));
    fprintf (stdout, "\nVersion: %s", glGetString (GL_VERSION));
    fprintf (stdout, "\nGLSL: %s", glGetString (GL_SHADING_LANGUAGE_VERSION));
    checkError ("dumpInfo");
}

void KeyboardDownFunc(unsigned char key, int x, int y)
{
    g_PressedKeys[key] = true;
}

void KeyboardUpFunc(unsigned char key, int x, int y)
{
    g_PressedKeys[key] = false;
}

void ProcessKeys(void)
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
        
		if(g_PressedKeys[119]) //'w'
		{
			g_pOceanSurf->CameraPosition[0]+=0.5f*direction[0];
			g_pOceanSurf->CameraPosition[1]+=0.5f*direction[1];
			g_pOceanSurf->CameraPosition[2]+=0.5f*direction[2];
			g_pOceanSurf->LookAtPosition[0]+=0.5f*direction[0];
			g_pOceanSurf->LookAtPosition[1]+=0.5f*direction[1];
			g_pOceanSurf->LookAtPosition[2]+=0.5f*direction[2];
		}
		if(g_PressedKeys[115]) //'s'
		{
			g_pOceanSurf->CameraPosition[0]-=0.5f*direction[0];
			g_pOceanSurf->CameraPosition[1]-=0.5f*direction[1];
			g_pOceanSurf->CameraPosition[2]-=0.5f*direction[2];
			g_pOceanSurf->LookAtPosition[0]-=0.5f*direction[0];
			g_pOceanSurf->LookAtPosition[1]-=0.5f*direction[1];
			g_pOceanSurf->LookAtPosition[2]-=0.5f*direction[2];
		}
		if(g_PressedKeys[97]) //'a'
		{
			g_pOceanSurf->CameraPosition[0]+=0.5f*strafe[0];
			g_pOceanSurf->CameraPosition[1]+=0.5f*strafe[1];
			g_pOceanSurf->CameraPosition[2]+=0.5f*strafe[2];
			g_pOceanSurf->LookAtPosition[0]+=0.5f*strafe[0];
			g_pOceanSurf->LookAtPosition[1]+=0.5f*strafe[1];
			g_pOceanSurf->LookAtPosition[2]+=0.5f*strafe[2];
		}
		if(g_PressedKeys[100]) //'d'
		{
			g_pOceanSurf->CameraPosition[0]-=0.5f*strafe[0];
			g_pOceanSurf->CameraPosition[1]-=0.5f*strafe[1];
			g_pOceanSurf->CameraPosition[2]-=0.5f*strafe[2];
			g_pOceanSurf->LookAtPosition[0]-=0.5f*strafe[0];
			g_pOceanSurf->LookAtPosition[1]-=0.5f*strafe[1];
			g_pOceanSurf->LookAtPosition[2]-=0.5f*strafe[2];
		}
        
        if(g_PressedKeys[27]) // "esc"
        {
            throw "time to exit";
        }
	}
}

void RenderFrame(void)
{
	// Fill the simulation pipeline - this loop should execute once in all cases except the first
	// iteration, when the simulation pipeline is first 'primed'
	do {
		GFSDK_WaveWorks_Simulation_SetTime(g_hOceanSimulation, g_pOceanSurf->total_time);
		GFSDK_WaveWorks_Simulation_KickGL2(g_hOceanSimulation,NULL);
	} while(gfsdk_waveworks_result_NONE == GFSDK_WaveWorks_Simulation_GetStagingCursor(g_hOceanSimulation,NULL));
    
	// getting simulation timings
	GFSDK_WaveWorks_Simulation_GetStats(g_hOceanSimulation,g_ocean_stats_simulation);
	

    
	g_pOceanSurf->Render(g_hOceanSimulation, g_ocean_simulation_settings);
    
	glutSwapBuffers();
    
	// timing
	timeval time;
    gettimeofday(&time,NULL);
    g_Microseconds = time.tv_sec*1000000 + time.tv_usec;
    
	g_pOceanSurf->delta_time = (float)(g_Microseconds - g_OldMicroseconds)/1000000.0f;
	g_pOceanSurf->total_time += g_pOceanSurf->delta_time;
	if(g_pOceanSurf->total_time>=36000.0f) g_pOceanSurf->total_time=0;
	
    g_OldMicroseconds = g_Microseconds;
	
	g_pOceanSurf->frame_number++;
    
    ProcessKeys();
}

void MouseFunc(int Button, int State, int MouseX, int MouseY)
{
    if((Button == GLUT_LEFT_BUTTON) && (State == GLUT_DOWN)) LButtonPressed = true;
	if((Button == GLUT_LEFT_BUTTON) && (State == GLUT_UP)) LButtonPressed = false;
    g_pOceanSurf->MouseX = MouseX;
    g_pOceanSurf->MouseY = MouseY;
}
void MouseMotionFunc(int MouseX, int MouseY)
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

void Reshape(int w, int h)
{
    g_pOceanSurf->ScreenWidth = w;
    g_pOceanSurf->ScreenHeight = h;
}

int main(int argc, char* argv[])
{
    // changing the current directory to executable directory
    char currentdir[65536];
    char dir[65536];
    char file[65536];
    
    getcwd(currentdir,65536);
    splitToDirAndFile(argv[0],dir,file);
    chdir(dir);
    

    
	gfsdk_waveworks_result res;

    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
    
    glutInitWindowSize((int) WindowWidth, (int) WindowHeight);
    
    /* create the window and store the handle to it */
    g_GLUTWindowHandle = glutCreateWindow("OpenGL Test Application");

    
	std::string cmdline;
	for(int i=0 ; i<argc ; i++)
	{
		cmdline += argv[i];
		cmdline += " ";
	}
    
	PrintGLInfo();
	g_ocean_simulation_settings.use_texture_arrays				= true;
    
	// Creating OceanSurface instance
	g_pOceanSurf = new OceanSurface(g_ocean_simulation_settings.use_texture_arrays);
    
	// Initializing GL functions to be passed to WaveWorks
	g_GLFunctions.glGetError =  glGetError;
	g_GLFunctions.glGetIntegerv = glGetIntegerv;
	g_GLFunctions.glTexParameterf = glTexParameterf;
	g_GLFunctions.glTexParameteri = glTexParameteri;
	g_GLFunctions.glTexImage2D = glTexImage2D;
	g_GLFunctions.glTexSubImage2D = glTexSubImage2D;
	g_GLFunctions.glClear = glClear;
	g_GLFunctions.glClearColor = glClearColor;
	g_GLFunctions.glColorMask = glColorMask;
	g_GLFunctions.glDisable = glDisable;
	g_GLFunctions.glViewport = glViewport;
	g_GLFunctions.glBindTexture = glBindTexture;
	g_GLFunctions.glDeleteTextures = glDeleteTextures;
	g_GLFunctions.glGenTextures = glGenTextures;
	g_GLFunctions.glDrawElements = glDrawElements;
	g_GLFunctions.glActiveTexture = glActiveTexture;
	g_GLFunctions.glBindBuffer = glBindBuffer;
	g_GLFunctions.glDeleteBuffers = glDeleteBuffers;
	g_GLFunctions.glGenBuffers = glGenBuffers;
	g_GLFunctions.glBufferData = glBufferData;
	g_GLFunctions.glMapBufferRange = glMapBufferRange;
	g_GLFunctions.glUnmapBuffer = glUnmapBuffer;
	g_GLFunctions.glAttachShader = glAttachShader;
	g_GLFunctions.glCompileShader = glCompileShader;
	g_GLFunctions.glCreateProgram = glCreateProgram;
	g_GLFunctions.glCreateShader = glCreateShader;
	g_GLFunctions.glDeleteProgram = glDeleteProgram;
	g_GLFunctions.glDeleteShader = glDeleteShader;
	g_GLFunctions.glDisableVertexAttribArray = glDisableVertexAttribArray;
	g_GLFunctions.glEnableVertexAttribArray = glEnableVertexAttribArray;
	g_GLFunctions.glGetAttribLocation = glGetAttribLocation;
	g_GLFunctions.glGetProgramiv = glGetProgramiv;
	g_GLFunctions.glGetProgramInfoLog = glGetProgramInfoLog;
	g_GLFunctions.glGetShaderiv = glGetShaderiv;
	g_GLFunctions.glGetShaderInfoLog = glGetShaderInfoLog;
	g_GLFunctions.glGetUniformLocation = glGetUniformLocation;
	g_GLFunctions.glLinkProgram = glLinkProgram;
	g_GLFunctions.glShaderSource = glShaderSource;
	g_GLFunctions.glUseProgram = glUseProgram;
	g_GLFunctions.glUniform1f = glUniform1f;
	g_GLFunctions.glUniform1i = glUniform1i;
	g_GLFunctions.glUniform3fv = glUniform3fv;
	g_GLFunctions.glUniform4fv = glUniform4fv;
	g_GLFunctions.glVertexAttribPointer = glVertexAttribPointer;
	g_GLFunctions.glUniformMatrix3x4fv = glUniformMatrix3x4fv;
	g_GLFunctions.glBindFramebuffer = glBindFramebuffer;
	g_GLFunctions.glDeleteFramebuffers = glDeleteFramebuffers;
	g_GLFunctions.glGenFramebuffers = glGenFramebuffers;
	g_GLFunctions.glCheckFramebufferStatus = glCheckFramebufferStatus;
	g_GLFunctions.glGenerateMipmap = glGenerateMipmap;
	g_GLFunctions.glFramebufferTexture2D = glFramebufferTexture2D;
	g_GLFunctions.glPatchParameteri = glPatchParameteri;
	g_GLFunctions.glGenQueries = glGenQueries;
	g_GLFunctions.glDeleteQueries = glDeleteQueries;
	g_GLFunctions.glQueryCounter = glQueryCounter;
	g_GLFunctions.glGetQueryObjectui64v = glGetQueryObjectui64v;
	g_GLFunctions.glGetActiveAttrib = glGetActiveAttrib;
	g_GLFunctions.glFramebufferTextureLayer = glFramebufferTextureLayer;
	g_GLFunctions.glBlitFramebuffer = glBlitFramebuffer;
	g_GLFunctions.glTexImage3D = glTexImage3D;
	g_GLFunctions.glReadBuffer = glReadBuffer;
	g_GLFunctions.glDrawBuffers = glDrawBuffers;
    
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
	if(0 == detail_level)
		return false;
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
    
	g_ocean_simulation_settings.fft_period						= 1000.0f;
	g_ocean_simulation_settings.detail_level					= GFSDK_WaveWorks_Simulation_DetailLevel_Normal;
	g_ocean_simulation_settings.readback_displacements			= false;
	g_ocean_simulation_settings.aniso_level						= 4;
	g_ocean_simulation_settings.CPU_simulation_threading_model	= GFSDK_WaveWorks_Simulation_CPU_Threading_Model_2;
	g_ocean_simulation_settings.use_Beaufort_scale				= true;
	g_ocean_simulation_settings.num_GPUs						= 1;
    
	res = GFSDK_WaveWorks_Simulation_CreateGL2(g_ocean_simulation_settings, g_ocean_simulation_param, (void*) 1 /*g_hRC*/, &g_hOceanSimulation);
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
	GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin(g_pOceanSurf->m_hOceanQuadTree, GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate(g_hOceanSimulation));
    
	fprintf(stdout, "Entering main loop\n");
    
	// entering main loop
    glutIdleFunc(RenderFrame);
    glutDisplayFunc(RenderFrame);
    glutReshapeFunc(Reshape);
    glutMouseFunc(MouseFunc);
    glutMotionFunc(MouseMotionFunc);
    glutKeyboardFunc(KeyboardDownFunc);
    glutKeyboardUpFunc(KeyboardUpFunc);
    
    try
    {
        glutMainLoop();
    }
    catch (const char* msg)
    {
        // do nothing, just exit GLUT main loop
    }

	// Cleanup WaveWorks
	delete g_pOceanSurf;
	GFSDK_WaveWorks_Simulation_Destroy(g_hOceanSimulation);
	GFSDK_WaveWorks_ReleaseGL2();
    
}

