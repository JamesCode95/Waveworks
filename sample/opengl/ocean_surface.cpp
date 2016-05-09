// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#define _HAS_EXCEPTIONS 0
#include "ocean_surface.h"
#include "math_code.h"
#include <vector>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif 

enum OceanSurfaceTextureUnits
{
	FoamDiffuseTextureUnit = 0,
	FoamIntensityTextureUnit,
	Num_OceanSurfaceTextureUnits
};

bool file_exists(const char * filename)
{    
	FILE * file = NULL;
	fopen_s(&file,filename, "r");
	if (file)
	{        
		fclose(file);        
		return true;    
	}    
	return false;
}

void search_file(char* found_filename, size_t numberOfElements, char* filename)
{
    // Get the exe name, and exe path
	static const char* baseDir = NULL;
	if(!baseDir)
	{
		static char strExePath[MAX_PATH] = {0};
		char* strLastSlash = NULL;
		GetModuleFileNameA( NULL, strExePath, MAX_PATH );
		strExePath[MAX_PATH-1]=0;
		strLastSlash = strrchr( strExePath, '\\' );
		if( strLastSlash )
		{
			// Chop the exe name from the exe path
			*strLastSlash = 0;
		}

		baseDir = strExePath;
	}

	char* search_paths[] = {    "",
								"..\\",
								"..\\..\\",
								"..\\media\\",
								"..\\..\\media\\",
								"..\\..\\..\\media\\"
								};
	enum { NumPaths = sizeof(search_paths)/sizeof(search_paths[0]) };
	char trial_filename[1024];
	for(int i=0; i<NumPaths; i++)
	{
		sprintf_s(trial_filename, sizeof(trial_filename)/sizeof(trial_filename[0]), "%s\\%s%s", baseDir, search_paths[i], filename);
		if(file_exists(trial_filename))
		{
			strcpy_s(found_filename, numberOfElements, trial_filename);
			return;
		}
	}
	strcpy_s(found_filename, numberOfElements, filename);
}

gfsdk_waveworks_result OceanSurface::InitQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params)
{
	if(NULL == hOceanQuadTree)
	{
		return GFSDK_WaveWorks_Quadtree_CreateGL2(params, WaterProgram, &hOceanQuadTree);
	}
	else
	{
		return GFSDK_WaveWorks_Quadtree_UpdateParams(hOceanQuadTree, params);
	}
}

void OceanSurface::CreateTextures(void)
{
	LoadTexture("foam.dds",&FoamDiffuseTextureID);
	LoadTexture("foam_intensity_perlin2_rgb.dds",&FoamIntensityTextureID);
}

OceanSurface::OceanSurface(bool use_texture_arrays)
{
	// initializing GL function addresses
	InitGLFunctions();

	// loading & compiling water shader
	WaterProgram = LoadProgram("water.glsl");

	// initializing quadtree 
	hOceanQuadTree				= NULL;

	// get the attribute count, so that we can search for any attrib shader inputs needed by waveworks
	GLint numAttrs = 0;
	glGetProgramiv(WaterProgram,GL_ACTIVE_ATTRIBUTES,&numAttrs);

	// initializing quadtree shader input mappings
	UINT NumQuadtreeShaderInputs	= GFSDK_WaveWorks_Quadtree_GetShaderInputCountGL2();
	pQuadTreeShaderInputMappings	= new UINT [NumQuadtreeShaderInputs];
	GFSDK_WaveWorks_ShaderInput_Desc* quadtree_descs =  new GFSDK_WaveWorks_ShaderInput_Desc[NumQuadtreeShaderInputs];
	for(unsigned int i = 0; i < NumQuadtreeShaderInputs; i++)
	{
		GFSDK_WaveWorks_Quadtree_GetShaderInputDescGL2(i, &(quadtree_descs[i]));
		switch(quadtree_descs[i].Type)
		{
		case GFSDK_WaveWorks_ShaderInput_Desc::GL_FragmentShader_UniformLocation:
		case GFSDK_WaveWorks_ShaderInput_Desc::GL_VertexShader_UniformLocation:
		case GFSDK_WaveWorks_ShaderInput_Desc::GL_TessEvalShader_UniformLocation:
			pQuadTreeShaderInputMappings[i] = glGetUniformLocation(WaterProgram, quadtree_descs[i].Name);
			break;
		case GFSDK_WaveWorks_ShaderInput_Desc::GL_AttribLocation:
			pQuadTreeShaderInputMappings[i] = (UINT)GFSDK_WaveWorks_UnusedShaderInputRegisterMapping;
			for(GLint attrIndex = 0; attrIndex != numAttrs; ++attrIndex)
			{
				char attribName[256];
				GLint size;
				GLenum type;
				GLsizei nameLen;
				glGetActiveAttrib(WaterProgram,attrIndex,sizeof(attribName),&nameLen,&size,&type,attribName);
				if(GFSDK_WaveWorks_GLAttribIsShaderInput(attribName,quadtree_descs[i]))
				{
					pQuadTreeShaderInputMappings[i] = glGetAttribLocation(WaterProgram, attribName);
				}
			}

			break;
		}
	}
	
	// initializing simulation shader input mappings
	UINT NumSimulationShaderInputs	= GFSDK_WaveWorks_Simulation_GetShaderInputCountGL2();
	pSimulationShaderInputMappings	= new UINT [NumSimulationShaderInputs];
	GFSDK_WaveWorks_ShaderInput_Desc* simulation_descs =  new GFSDK_WaveWorks_ShaderInput_Desc[NumSimulationShaderInputs];

	for(unsigned int i = 0; i < NumSimulationShaderInputs; i++)
	{
		GFSDK_WaveWorks_Simulation_GetShaderInputDescGL2(i, &(simulation_descs[i]));
		pSimulationShaderInputMappings[i] = glGetUniformLocation(WaterProgram, simulation_descs[i].Name);
	}

	// reserve texture units for WaveWorks for use during ocean surface rendering
	for(unsigned int i = 0; i != GFSDK_WaveWorks_Simulation_GetTextureUnitCountGL2(use_texture_arrays); ++i)
	{
		glPool.Reserved_Texture_Units[i] = Num_OceanSurfaceTextureUnits + i;
	}

	// creating textures
	CreateTextures();

	// binding textures to shader
	FoamDiffuseTextureBindLocation = glGetUniformLocation(WaterProgram, "g_texFoamDiffuseMap");
	FoamIntensityTextureBindLocation = glGetUniformLocation(WaterProgram, "g_texFoamIntensityMap");

	// initializing the rest of variables
	CameraPosition[0] = -20.0f;
	CameraPosition[1] = 20.0f;
	CameraPosition[2] = 20.0f;
	LookAtPosition[0] = 0.0f;
	LookAtPosition[1] = 0.0f;
	LookAtPosition[2] = 0.0f;

	Alpha = 0;
	Beta = 0;

	ScreenWidth = 1280;
	ScreenHeight = 720;
	total_time = 0;

	// cleanup
	SAFE_DELETE_ARRAY(quadtree_descs);
	SAFE_DELETE_ARRAY(simulation_descs);
}



OceanSurface::~OceanSurface()
{
	if(hOceanQuadTree)
	{
		GFSDK_WaveWorks_Quadtree_Destroy(hOceanQuadTree);
	}

	SAFE_DELETE_ARRAY(pQuadTreeShaderInputMappings);
	SAFE_DELETE_ARRAY(pSimulationShaderInputMappings);
}

void checkError(const char *functionName)
{
   GLenum error;
   while (( error = glGetError() ) != GL_NO_ERROR) {
      fprintf (stderr, "\nGL error 0x%X detected in %s", error, functionName);
   }
}

void OceanSurface::Render(GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_Simulation_Settings settings, bool Wireframe)
{
	// rendering to main buffer
	SetupNormalView();

	// setting up main buffer & GL state
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.38f, 0.45f, 0.56f, 0.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0,0,ScreenWidth,ScreenHeight);
	if(Wireframe)
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
	}
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// rendering water to main buffer
	// setting up program
	glUseProgram(WaterProgram);

	glUniformMatrix4fv(glGetUniformLocation(WaterProgram, "u_ViewProjMatrix"), 1, GL_FALSE, (const GLfloat *) NormalViewProjMatrix);

	//fprintf (stderr, "\n\nGFSDK_WaveWorks_Quadtree_GetShaderInputCountGL2: %i",nm);
	gfsdk_float4x4 ViewMatrix;
	gfsdk_float4x4 ProjMatrix;
	ViewMatrix._11 = NormalViewMatrix[0][0];
	ViewMatrix._12 = NormalViewMatrix[0][1];
	ViewMatrix._13 = NormalViewMatrix[0][2];
	ViewMatrix._14 = NormalViewMatrix[0][3];
	ViewMatrix._21 = NormalViewMatrix[1][0];
	ViewMatrix._22 = NormalViewMatrix[1][1];
	ViewMatrix._23 = NormalViewMatrix[1][2];
	ViewMatrix._24 = NormalViewMatrix[1][3];
	ViewMatrix._31 = NormalViewMatrix[2][0];
	ViewMatrix._32 = NormalViewMatrix[2][1];
	ViewMatrix._33 = NormalViewMatrix[2][2];
	ViewMatrix._34 = NormalViewMatrix[2][3];
	ViewMatrix._41 = NormalViewMatrix[3][0];
	ViewMatrix._42 = NormalViewMatrix[3][1];
	ViewMatrix._43 = NormalViewMatrix[3][2];
	ViewMatrix._44 = NormalViewMatrix[3][3];

	ProjMatrix._11 = NormalProjMatrix[0][0];
	ProjMatrix._12 = NormalProjMatrix[0][1];
	ProjMatrix._13 = NormalProjMatrix[0][2];
	ProjMatrix._14 = NormalProjMatrix[0][3];
	ProjMatrix._21 = NormalProjMatrix[1][0];
	ProjMatrix._22 = NormalProjMatrix[1][1];
	ProjMatrix._23 = NormalProjMatrix[1][2];
	ProjMatrix._24 = NormalProjMatrix[1][3];
	ProjMatrix._31 = NormalProjMatrix[2][0];
	ProjMatrix._32 = NormalProjMatrix[2][1];
	ProjMatrix._33 = NormalProjMatrix[2][2];
	ProjMatrix._34 = NormalProjMatrix[2][3];
	ProjMatrix._41 = NormalProjMatrix[3][0];
	ProjMatrix._42 = NormalProjMatrix[3][1];
	ProjMatrix._43 = NormalProjMatrix[3][2];
	ProjMatrix._44 = NormalProjMatrix[3][3];

	// binding user textures
	glActiveTexture(GL_TEXTURE0 + FoamDiffuseTextureUnit);
	glBindTexture(GL_TEXTURE_2D, FoamDiffuseTextureID); 
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, settings.aniso_level);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);	
	glUniform1i(FoamDiffuseTextureBindLocation, FoamDiffuseTextureUnit);

	glActiveTexture(GL_TEXTURE0 + FoamIntensityTextureUnit);
	glBindTexture(GL_TEXTURE_2D, FoamIntensityTextureID); 
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, settings.aniso_level);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);	
	glUniform1i(FoamIntensityTextureBindLocation, FoamIntensityTextureUnit);

	GFSDK_WaveWorks_Simulation_SetRenderStateGL2(hSim, ViewMatrix, pSimulationShaderInputMappings, glPool);
	GFSDK_WaveWorks_Quadtree_DrawGL2(hOceanQuadTree, ViewMatrix, ProjMatrix, pQuadTreeShaderInputMappings);
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
}

void OceanSurface::SetupNormalView()
{
	float scene_z_near = 1.0f;
	float scene_z_far = 10000.0f;
	float camera_fov = 3.14f*0.5f;
	mat4CreateView(NormalViewMatrix, CameraPosition, LookAtPosition);
	mat4CreateProjection(NormalProjMatrix,scene_z_near,scene_z_near/tan(camera_fov*0.5f)*(float)ScreenHeight/(float)ScreenWidth,scene_z_near/tan(camera_fov*0.5f),scene_z_far);
	mat4Mat4Mul(NormalViewProjMatrix,NormalProjMatrix,NormalViewMatrix);
}


GLuint OceanSurface::CompileShader(char *text, GLenum type) //returns shader handle or 0 if error
{
	GLint length, compiled;

	GLuint shader;
	GLsizei maxlength=65535;
	char infolog[65535];
	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar **)&text, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS,  &compiled);
	if (!compiled) {
		fprintf(stderr,"\nshader [%i] compilation error",type);
		fprintf(stderr,"\n...\n%s\n...\n",text);
		glGetShaderInfoLog(shader, maxlength, &length, infolog);
		fprintf(stderr,"\ninfolog: %s",infolog);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

const char* GetIncludeFilename(char* line)
{
	char* curr = line;

	// Simple little state machine to parse a #include line...

	// Leading wspace
	while(*curr && isspace(*curr))
		++curr;

	// Check for #include...
	const char* includeTag = "#include";
	if(curr != strstr(curr,includeTag))
		return NULL;
	curr += strlen(includeTag);

	// Consume more wspace
	while(*curr && isspace(*curr))
		++curr;

	// Check for leading quote
	if(*curr != '\"')
		return NULL;
	curr ++;
	const char* pResult = curr;

	// Advance to closing quote
	while(*curr && *curr != '\"')
		++curr;

	if(*curr != '\"')
		return NULL;

	// Replace with NULL term
	*curr = '\0';

	return pResult;
}

int LoadProgramSource(const char* filename, std::vector<char>& loadedSrc)
{
	FILE *f;
	fopen_s(&f, filename,"rt");
	if(f==NULL)
	{
		fprintf(stderr,"\ncould not open [%s]\n",filename);
		return -1;
	} 

	char lineBuffer[1024];
	while(!feof(f))
	{
		char* currLine = fgets(lineBuffer,sizeof(lineBuffer),f);
		if(currLine)
		{
			// GLSL does not support #include
			// We process #includes here for simplicity, but most real-world apps will want to do this pre-process step offline
			const char* includeFileName = GetIncludeFilename(currLine);
			if(includeFileName)
			{
				// Assume relative to curr file
				char drv[_MAX_DRIVE];
				char dir[_MAX_DIR];
				_splitpath_s(filename,drv,_MAX_DRIVE,dir,_MAX_DIR,NULL,0,NULL,0);
				char includePath[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME];
				size_t bufferSpace = sizeof(includePath)/sizeof(includePath[0]);
				char* includePathCurr = includePath;
				size_t slen = strlen(drv);
				strcpy_s(includePathCurr,bufferSpace,drv);
				includePathCurr +=  slen;
				bufferSpace -= slen;
				slen = strlen(dir);
				strcpy_s(includePathCurr,bufferSpace,dir);
				includePathCurr +=  slen;
				bufferSpace -= slen;
				slen = strlen(includeFileName);
				strcpy_s(includePathCurr,bufferSpace,includeFileName);
				includePathCurr +=  slen;
				bufferSpace -= slen;
				if(LoadProgramSource(includePath,loadedSrc))
					return -1;
			}
			else
			{
				loadedSrc.insert(loadedSrc.end(),currLine,currLine+strlen(currLine));
			}
		}
	}

	fclose(f);
	return 0;
}

GLuint OceanSurface::LoadProgram(char* filename) //returns program object handle or 0 if error
{
	char tmp_text[65535];
	const char *tmp_pos_start; 
	const char *tmp_pos_end; 

	GLuint result = 0;
	GLenum program;

	GLint compiled;

	char trial_filename[1024];
	search_file(trial_filename, sizeof(trial_filename)/sizeof(trial_filename[0]), filename);

	fprintf(stderr,"\nLoading & compiling shader [%s]: ",trial_filename);

	std::vector<char> loadedSrc;
	if(LoadProgramSource(trial_filename,loadedSrc))
	{
		return 0;
	}
	loadedSrc.push_back('\0');
	const char* programSrc = &loadedSrc[0];
	GLint length = (GLint)loadedSrc.size();

	program = glCreateProgram();

	// looking for vertex shader
	tmp_pos_start = strstr(programSrc,"<<<VSTEXT>>>");
	if(tmp_pos_start!=NULL)
	{
		tmp_pos_start+=12;
		tmp_pos_end = strstr(tmp_pos_start,"<<<");	
		if (tmp_pos_end==NULL) tmp_pos_end = programSrc + length;
		strncpy_s(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
		tmp_text[tmp_pos_end-tmp_pos_start]=0;
		result = CompileShader(tmp_text,GL_VERTEX_SHADER);
		if(result)
		{
			glAttachShader(program,result);
			glDeleteShader(result);
		}
	}

	// looking for tessellation control shader
	tmp_pos_start = strstr(programSrc,"<<<TCTEXT>>>");
	if(tmp_pos_start!=NULL)
	{
		tmp_pos_start+=12;
		tmp_pos_end = strstr(tmp_pos_start,"<<<");	
		if (tmp_pos_end==NULL) tmp_pos_end = programSrc + length;
		strncpy_s(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
		tmp_text[tmp_pos_end-tmp_pos_start]=0;
		result = CompileShader(tmp_text,GL_TESS_CONTROL_SHADER);
		if(result)
		{
			glAttachShader(program,result);
			glDeleteShader(result);
		}
	}
	
	// looking for tessellation evaluation shader
	tmp_pos_start = strstr(programSrc,"<<<TETEXT>>>");
	if(tmp_pos_start!=NULL)
	{
		tmp_pos_start+=12;
		tmp_pos_end = strstr(tmp_pos_start,"<<<");	
		if (tmp_pos_end==NULL) tmp_pos_end = programSrc + length;
		strncpy_s(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
		tmp_text[tmp_pos_end-tmp_pos_start]=0;
		result = CompileShader(tmp_text,GL_TESS_EVALUATION_SHADER);
		if(result)
		{
			glAttachShader(program,result);
			glDeleteShader(result);
		}
	}

	// looking for geometry shader
	tmp_pos_start = strstr(programSrc,"<<<GSTEXT>>>");
	if(tmp_pos_start!=NULL)
	{
		tmp_pos_start+=12;
		tmp_pos_end = strstr(tmp_pos_start,"<<<");	
		if (tmp_pos_end==NULL) tmp_pos_end = programSrc + length;
		strncpy_s(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
		tmp_text[tmp_pos_end-tmp_pos_start]=0;
		result = CompileShader(tmp_text,GL_GEOMETRY_SHADER);
		if(result)
		{
			glAttachShader(program,result);
			glDeleteShader(result);
		}
	}

	// looking for fragment shader
	tmp_pos_start = strstr(programSrc,"<<<FSTEXT>>>");
	if(tmp_pos_start!=NULL)
	{
		tmp_pos_start+=12;
		tmp_pos_end = strstr(tmp_pos_start,"<<<");	
		if (tmp_pos_end==NULL) tmp_pos_end = programSrc + length;
		strncpy_s(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
		tmp_text[tmp_pos_end-tmp_pos_start]=0;
		result = CompileShader(tmp_text,GL_FRAGMENT_SHADER);
		if(result)
		{
			glAttachShader(program,result);
			glDeleteShader(result);
		}
	}

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &compiled);
	if (!compiled) {
		const GLsizei maxlength=65535;
		char infolog[maxlength];
		fprintf(stderr,"program link error\n");
		glGetProgramInfoLog(program, maxlength, &length, infolog);
		fprintf(stderr,"\ninfolog: %s\n",infolog);
		return 0;
	}
	return program;
}

gliGenericImage* ReadDDSFile(const char *filename, int *bufsize, int *numMipmaps) 
{
	gliGenericImage *genericImage;
	DDSURFACEDESC2 ddsd;
	char filecode[4];
	FILE *fp;
	int factor,uncompressed_components,format_found;
	/* try to open the file */
	fopen_s(&fp, filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr,"\nUnable to load texture file [%s]",filename);
		return NULL;
	}
	/* verify the type of file */
	fread(filecode, 1, 4, fp);
	if (strncmp(filecode, "DDS ", 4) != 0) 
	{
		fprintf(stderr,"\nThe file [%s] is not a dds file",filename);
		fclose(fp);
		return NULL;
	}
	/* get the surface desc */
	fread(&ddsd, sizeof(ddsd), 1, fp);
	if((ddsd.dwFlags & DDSD_PIXELFORMAT) != DDSD_PIXELFORMAT) 
	{
		fprintf(stderr,"\nDDS header pixelformat field contains no valid data,\nunable to load texture.");
		return 0;
	}
	if(((ddsd.dwFlags & DDSD_WIDTH) != DDSD_WIDTH) || ((ddsd.dwFlags & DDSD_HEIGHT) != DDSD_HEIGHT)) 
	{
		fprintf(stderr,"\nDDS header width/height fields contains no valid data,\nunable to load texture.");
		return 0;
	}
	if((ddsd.dwFlags & DDSD_MIPMAPCOUNT) != DDSD_MIPMAPCOUNT) 
	{
		fprintf(stderr,"\nDDS header mipmapcount field contains no valid data,\nassuming 0 mipmaps.");
		//return 0;
	}
	if(((ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) != DDPF_FOURCC) && ((ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB) != DDPF_RGB)) 
	{
		fprintf(stderr,"\nDDS header pixelformat field contains no valid FOURCC and RGB data,\nunable to load texture.");
		return 0;
	}
	if(((ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) == DDPF_FOURCC) && (ddsd.dwMipMapCount<=1) && ((ddsd.ddpfPixelFormat.dwFourCC==FOURCC_DXT1)||(ddsd.ddpfPixelFormat.dwFourCC==FOURCC_DXT3)||(ddsd.ddpfPixelFormat.dwFourCC==FOURCC_DXT5)) ) 
	{
		fprintf(stderr,"\nDDS header contains DXTx FOURCC code and no mipmaps,\nprogram does not support loading DXTx textures without mipmaps,\nunable to load texture.");
		return 0;
	}
	genericImage = (gliGenericImage*) malloc(sizeof(gliGenericImage));
	memset(genericImage,0,sizeof(gliGenericImage));
	factor=0;
	uncompressed_components=0;
	format_found=0;
	genericImage->width       = ddsd.dwWidth;
	genericImage->height      = ddsd.dwHeight;
	if((ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) == DDPF_FOURCC) // if FOURCC code is valid
	{
		switch(ddsd.ddpfPixelFormat.dwFourCC)
		{
			case FOURCC_DXT1:
				if(ddsd.dwAlphaBitDepth!=0)
				{
					genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					genericImage->components = 4;
				}
				else 
				{
					genericImage->format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
					genericImage->components = 3;
				}
				factor = 2;
				format_found=1;
			break;
			case FOURCC_DXT3:
				genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				genericImage->components = 4;
				factor = 4;
				format_found=1;
			break;
			case FOURCC_DXT5:
				genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				genericImage->components = 4;
				factor = 4;
				format_found=1;
			break;
			default:
			break;
		}
	}
	if(format_found==0)
	{
		switch (ddsd.ddpfPixelFormat.dwRGBBitCount)
		{
		case 24:
			genericImage->format = GL_RGB;
			genericImage->components = 3;
			format_found=2;
			break;
		case 32:
			genericImage->format = GL_RGBA;
			genericImage->components = 4;
			format_found=2;
		default:
			break;
		}
	}
	if(format_found==1)  // found compressed format
	{
	  *bufsize = ddsd.dwMipMapCount > 1 ? ddsd.dwLinearSize * factor : ddsd.dwLinearSize;
	}
	if(format_found==2)
	{
	  *bufsize = ddsd.dwMipMapCount > 1 ? ddsd.dwWidth*ddsd.dwHeight*genericImage->components+(ddsd.dwWidth*ddsd.dwHeight*genericImage->components)/2 : ddsd.dwWidth*ddsd.dwHeight*genericImage->components;
	}
	if(format_found==0)
	{
		fprintf(stderr,"\nUnsupported DDS format,\nthe program only supports DXTx and RGB/RGBA formats containing 8 bits per component");
		free(genericImage);
		return 0;
	}
	genericImage->pixels = (unsigned char*)malloc(*bufsize);
	fread(genericImage->pixels, 1, *bufsize, fp);
	fclose(fp);
	*numMipmaps = ddsd.dwMipMapCount;
	genericImage->numMipmaps=ddsd.dwMipMapCount;
	return genericImage;
}


int OceanSurface::LoadTexture(char * filename, GLuint * texID)
{
	gliGenericImage *ggi = NULL;
	int bufsize,numMipmaps,blocksize,i;
	long offset;
	GLint size,width,height;
	unsigned char c1,c2,c3,c4;
	long n;

	char trial_filename[1024];
	search_file(trial_filename, sizeof(trial_filename)/sizeof(trial_filename[0]), filename);

	fprintf(stderr,"\nLoading %s: ", trial_filename);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	ggi = ReadDDSFile(trial_filename, &bufsize, &numMipmaps);
	if(ggi==NULL)
	{
		fprintf(stderr,"The file [%s] hasn't been loaded successfully.",trial_filename);
		return 0;
	}
	height = ggi->height;
	width = ggi->width;
	switch (ggi->format)
	{
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			glGenTextures(1,texID);
			glBindTexture(GL_TEXTURE_2D, *texID);
			blocksize=8;
			offset = 0;
			height = ggi->height;
			width = ggi->width;
			for (i = 0; i < numMipmaps && (width || height); ++i)
			{
				if (width == 0)
					width = 1;
				if (height == 0)
					height = 1;
				size = ((width+3)/4)*((height+3)/4)*blocksize;
				glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 0, size, ggi->pixels+offset);
				offset += size;
				width >>= 1;
				height >>= 1;
			}
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			glGenTextures(1,texID);
			glBindTexture(GL_TEXTURE_2D, *texID);
			blocksize=8;
			offset = 0;
			height = ggi->height;
			width = ggi->width;
			for (i = 0; i < numMipmaps && (width || height); ++i)
			{
				if (width == 0)
					width = 1;
				if (height == 0)
					height = 1;
				size = ((width+3)/4)*((height+3)/4)*blocksize;
				glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 0, size, ggi->pixels + offset);
				offset += size;
				width >>= 1;
				height >>= 1;
			}
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			glGenTextures(1,texID);
			glBindTexture(GL_TEXTURE_2D, *texID);
			blocksize=16;
			offset = 0;
			height = ggi->height;
			width = ggi->width;
			for (i = 0; i < numMipmaps && (width || height); ++i)
			{
				if (width == 0)
					width = 1;
				if (height == 0)
					height = 1;
				size = ((width+3)/4)*((height+3)/4)*blocksize;
				glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 0, size, ggi->pixels + offset);
				offset += size;
				width >>= 1;
				height >>= 1;
			}
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			glGenTextures(1,texID);
			glBindTexture(GL_TEXTURE_2D, *texID);
			blocksize=16;
			offset = 0;
			height = ggi->height;
			width = ggi->width;
			for (i = 0; i < numMipmaps && (width || height); ++i)
				{
				if (width == 0)
					width = 1;
				if (height == 0)
					height = 1;
				size = ((width+3)/4)*((height+3)/4)*blocksize;
				glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 
					0, size, ggi->pixels + offset);
				offset += size;
				width >>= 1;
				height >>= 1;
			}
			break;
		case GL_RGB:
			glGenTextures(1,texID);
			glBindTexture(GL_TEXTURE_2D, *texID);
			offset = 0;
			height = ggi->height;
			width = ggi->width;
			if(numMipmaps<=1)
			{
				for(n=0;n<width*height*3;n+=3)
				{
					c1=*(ggi->pixels+n); // switching R and B
					c3=*(ggi->pixels+n+2);
					*(ggi->pixels+n)=c3;
					*(ggi->pixels+n+2)=c1;
				}
				glTexImage2D(GL_TEXTURE_2D, 0, ggi->format, width, height,0, ggi->format, GL_UNSIGNED_BYTE, ggi->pixels);
				glGenerateMipmap(GL_TEXTURE_2D);
			}else
			for (i = 0; i < numMipmaps && (width || height); ++i)
			{
				if (width == 0)
					width = 1;
				if (height == 0)
					height = 1;
				size = width*height*3;
				for(n=0;n<size;n+=3)
				{
					c1=*(ggi->pixels+offset+n); // switching R and B
					c3=*(ggi->pixels+offset+n+2);
					*(ggi->pixels+offset+n)=c3;
					*(ggi->pixels+offset+n+2)=c1;
				}
				glTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height,0, ggi->format, GL_UNSIGNED_BYTE, ggi->pixels + offset);
				offset += size;
				width >>= 1;
				height >>= 1;
			}
			break;
		case GL_RGBA:
			glGenTextures(1,texID);
			glBindTexture(GL_TEXTURE_2D, *texID);
			offset = 0;
			height = ggi->height;
			width = ggi->width;
			if(numMipmaps<=1)
			{
				for(n=0;n<width*height*4;n+=4)
				{
					c1=*(ggi->pixels+n); // switching BGRA to RGBA
					c2=*(ggi->pixels+n+1);
					c3=*(ggi->pixels+n+2);
					c4=*(ggi->pixels+n+3);
					*(ggi->pixels+n)=c3;
					*(ggi->pixels+n+1)=c2;
					*(ggi->pixels+n+2)=c1;
					*(ggi->pixels+n+3)=c4;
				}
				glTexImage2D(GL_TEXTURE_2D, 0, ggi->format, width, height,0, ggi->format, GL_UNSIGNED_BYTE, ggi->pixels);
				glGenerateMipmap(GL_TEXTURE_2D);
			}else
			for (i = 0; i < numMipmaps && (width || height); ++i)
			{
				if (width == 0)
					width = 1;
				if (height == 0)
					height = 1;
				size = width*height*4;
				for(n=0;n<size;n+=4)
				{
					c1=*(ggi->pixels+offset+n); // switching BGRA to RGBA
					c2=*(ggi->pixels+offset+n+1);
					c3=*(ggi->pixels+offset+n+2);
					c4=*(ggi->pixels+offset+n+3);
					*(ggi->pixels+offset+n)=c3;
					*(ggi->pixels+offset+n+1)=c2;
					*(ggi->pixels+offset+n+2)=c1;
					*(ggi->pixels+offset+n+3)=c4;
				}
				glTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height,0, ggi->format, GL_UNSIGNED_BYTE, ggi->pixels + offset);
				offset += size;
				width >>= 1;
				height >>= 1;
			}
			break;
	}
	free(ggi->pixels);
	free(ggi);
	fprintf(stderr," Loaded\n");
	return 1;
}

#define GL_GET_PROC_ADR_GL_1_1(proc_type, proc_name) proc_name = (proc_type) GetProcAddress(hOGL, #proc_name); if (proc_name == NULL) {fprintf(stderr,"InitGLFunctions(): Failed to load OpenGL function using GetProcAddress(): %s\n", #proc_name); gl_func_init_failed = true;}
#define GL_GET_PROC_ADR(proc_type, proc_name) proc_name = (proc_type) wglGetProcAddress(#proc_name); if (proc_name == NULL) {fprintf(stderr,"InitGLFunctions(): Failed to load OpenGL function using wglGetProcAddress(): %s\n",#proc_name); gl_func_init_failed = true;}

HRESULT OceanSurface::InitGLFunctions()
{
	HMODULE hOGL;
	hOGL = LoadLibrary(L"opengl32.dll");
	bool gl_func_init_failed = false;
	if(hOGL == NULL) 
	{
		fprintf(stderr,"InitGLFunctions(): Failed to load opengl32.dll using LoadLibrary()\n");
		return E_FAIL;
	}
	
	GL_GET_PROC_ADR(PFNGLACTIVETEXTUREPROC, glActiveTexture);
	GL_GET_PROC_ADR(PFNGLATTACHSHADERPROC, glAttachShader);
	GL_GET_PROC_ADR(PFNGLBINDBUFFERPROC, glBindBuffer);
	GL_GET_PROC_ADR(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
	GL_GET_PROC_ADR(PFNGLBINDTEXTUREPROC, glBindTexture);
	GL_GET_PROC_ADR(PFNGLBUFFERDATAPROC, glBufferData);
	GL_GET_PROC_ADR(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
	GL_GET_PROC_ADR_GL_1_1(PFNGLCLEARCOLORPROC, glClearColor);
	GL_GET_PROC_ADR(PFNGLCLEARDEPTHFPROC, glClearDepthf);
	GL_GET_PROC_ADR_GL_1_1(PFNGLCLEARPROC, glClear);
	GL_GET_PROC_ADR_GL_1_1(PFNGLCOLORMASKPROC, glColorMask);
	GL_GET_PROC_ADR(PFNGLCOMPILESHADERPROC, glCompileShader);
	GL_GET_PROC_ADR(PFNGLCOMPRESSEDTEXIMAGE2DPROC, glCompressedTexImage2D);
	GL_GET_PROC_ADR(PFNGLCREATEPROGRAMPROC, glCreateProgram);
	GL_GET_PROC_ADR(PFNGLCREATESHADERPROC, glCreateShader);
	GL_GET_PROC_ADR(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
	GL_GET_PROC_ADR(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
	GL_GET_PROC_ADR(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
	GL_GET_PROC_ADR(PFNGLDELETESHADERPROC, glDeleteShader);
	GL_GET_PROC_ADR(PFNGLDELETETEXTURESPROC, glDeleteTextures);
	GL_GET_PROC_ADR_GL_1_1(PFNGLDISABLEPROC, glDisable);
	GL_GET_PROC_ADR(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
	GL_GET_PROC_ADR(PFNGLDRAWELEMENTSPROC, glDrawElements);
	GL_GET_PROC_ADR(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
	GL_GET_PROC_ADR(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
	GL_GET_PROC_ADR(PFNGLGENBUFFERSPROC, glGenBuffers);
	GL_GET_PROC_ADR(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap);
	GL_GET_PROC_ADR(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
	GL_GET_PROC_ADR(PFNGLGENTEXTURESPROC, glGenTextures);
	GL_GET_PROC_ADR(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
	GL_GET_PROC_ADR_GL_1_1(PFNGLGETERRORPROC, glGetError);
	GL_GET_PROC_ADR_GL_1_1(PFNGLGETINTEGERVPROC, glGetIntegerv);
	GL_GET_PROC_ADR(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
	GL_GET_PROC_ADR(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
	GL_GET_PROC_ADR(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
	GL_GET_PROC_ADR(PFNGLGETSHADERIVPROC, glGetShaderiv);
	GL_GET_PROC_ADR(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
	GL_GET_PROC_ADR(PFNGLLINKPROGRAMPROC, glLinkProgram);
	GL_GET_PROC_ADR(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange);
	GL_GET_PROC_ADR(PFNGLPATCHPARAMETERIPROC, glPatchParameteri);
	GL_GET_PROC_ADR(PFNGLSHADERSOURCEPROC, glShaderSource);
	GL_GET_PROC_ADR_GL_1_1(PFNGLTEXIMAGE2DPROC, glTexImage2D);
	GL_GET_PROC_ADR_GL_1_1(PFNGLTEXPARAMETERFPROC, glTexParameterf);
	GL_GET_PROC_ADR_GL_1_1(PFNGLTEXPARAMETERIPROC, glTexParameteri);
	GL_GET_PROC_ADR(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D);
	GL_GET_PROC_ADR(PFNGLUNIFORM1FPROC, glUniform1f);
	GL_GET_PROC_ADR(PFNGLUNIFORM1IPROC, glUniform1i);
	GL_GET_PROC_ADR(PFNGLUNIFORM3FVPROC, glUniform3fv);
	GL_GET_PROC_ADR(PFNGLUNIFORM4FVPROC, glUniform4fv);
	GL_GET_PROC_ADR(PFNGLUNIFORMMATRIX3X4FVPROC, glUniformMatrix3x4fv);
	GL_GET_PROC_ADR(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
	GL_GET_PROC_ADR(PFNGLUNMAPBUFFERPROC, glUnmapBuffer);
	GL_GET_PROC_ADR(PFNGLUSEPROGRAMPROC, glUseProgram);
	GL_GET_PROC_ADR(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
	GL_GET_PROC_ADR_GL_1_1(PFNGLVIEWPORTPROC, glViewport);
	GL_GET_PROC_ADR(PFNGLGENQUERIESPROC, glGenQueries);
	GL_GET_PROC_ADR(PFNGLDELETEQUERIESPROC, glDeleteQueries);
	GL_GET_PROC_ADR(PFNGLQUERYCOUNTERPROC, glQueryCounter);
	GL_GET_PROC_ADR(PFNGLGETQUERYOBJECTUI64VPROC, glGetQueryObjectui64v);
	GL_GET_PROC_ADR(PFNGLGETACTIVEATTRIBPROC, glGetActiveAttrib);
	GL_GET_PROC_ADR(PFNGLFRAMEBUFFERTEXTURELAYERPROC, glFramebufferTextureLayer);
	GL_GET_PROC_ADR(PFNGLTEXIMAGE3DPROC, glTexImage3D);
	GL_GET_PROC_ADR(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer);
	GL_GET_PROC_ADR_GL_1_1(PFNGLREADBUFFERPROC, glReadBuffer);
	GL_GET_PROC_ADR(PFNGLDRAWBUFFERSPROC, glDrawBuffers);


	FreeLibrary (hOGL);
	if(gl_func_init_failed == true) return E_FAIL;
	return S_OK;
}