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

#include <GFSDK_WaveWorks.h>
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>

// types for GL functoins not defined in WaveWorks
typedef void (APIENTRYP PFNGLCLEARDEPTHFPROC) (GLfloat d);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

// GL constants
#define GL_FRAMEBUFFER                    0x8D40
#define GL_TEXTURE0                       0x84C0
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_TEXTURE_MAX_ANISOTROPY_EXT	  0x84FE
#define GL_ACTIVE_ATTRIBUTES              0x8B89

// DDS file reading related structures
typedef struct 
{
	GLsizei  width;
	GLsizei  height;
	GLint    components;
	GLenum   format;
	GLint	   numMipmaps;
	GLubyte *pixels;  
} gliGenericImage;

#ifndef _DDSURFACEDESC2
	// following structs & defines are copied from ddraw.h

	#ifndef DUMMYUNIONNAMEN
	#if defined(__cplusplus) || !defined(NONAMELESSUNION)
	#define DUMMYUNIONNAMEN(n)
	#else
	#define DUMMYUNIONNAMEN(n)      u##n
	#endif
	#endif

	#ifndef MAKEFOURCC
		#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
					((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
					((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
	#endif //defined(MAKEFOURCC)

	#define DDSD_PIXELFORMAT        0x00001000l
	#define DDSD_CAPS               0x00000001l     
	#define DDSD_HEIGHT             0x00000002l
	#define DDSD_WIDTH              0x00000004l
	#define DDSD_PITCH              0x00000008l
	#define DDSD_MIPMAPCOUNT        0x00020000l
	#define DDPF_FOURCC             0x00000004l
	#define DDPF_RGB                0x00000040l
	#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
	#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
	#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
	#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
	#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))

	typedef struct _DDCOLORKEY
	{
		DWORD       dwColorSpaceLowValue;   // low boundary of color space that is to
											// be treated as Color Key, inclusive
		DWORD       dwColorSpaceHighValue;  // high boundary of color space that is
											// to be treated as Color Key, inclusive
	} DDCOLORKEY;

	typedef struct _DDPIXELFORMAT
	{
		DWORD       dwSize;                 // size of structure
		DWORD       dwFlags;                // pixel format flags
		DWORD       dwFourCC;               // (FOURCC code)
		union
		{
			DWORD   dwRGBBitCount;          // how many bits per pixel
			DWORD   dwYUVBitCount;          // how many bits per pixel
			DWORD   dwZBufferBitDepth;      // how many total bits/pixel in z buffer (including any stencil bits)
			DWORD   dwAlphaBitDepth;        // how many bits for alpha channels
			DWORD   dwLuminanceBitCount;    // how many bits per pixel
			DWORD   dwBumpBitCount;         // how many bits per "buxel", total
			DWORD   dwPrivateFormatBitCount;// Bits per pixel of private driver formats. Only valid in texture
											// format list and if DDPF_D3DFORMAT is set
		} DUMMYUNIONNAMEN(1);
		union
		{
			DWORD   dwRBitMask;             // mask for red bit
			DWORD   dwYBitMask;             // mask for Y bits
			DWORD   dwStencilBitDepth;      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
			DWORD   dwLuminanceBitMask;     // mask for luminance bits
			DWORD   dwBumpDuBitMask;        // mask for bump map U delta bits
			DWORD   dwOperations;           // DDPF_D3DFORMAT Operations
		} DUMMYUNIONNAMEN(2);
		union
		{
			DWORD   dwGBitMask;             // mask for green bits
			DWORD   dwUBitMask;             // mask for U bits
			DWORD   dwZBitMask;             // mask for Z bits
			DWORD   dwBumpDvBitMask;        // mask for bump map V delta bits
			struct
			{
				WORD    wFlipMSTypes;       // Multisample methods supported via flip for this D3DFORMAT
				WORD    wBltMSTypes;        // Multisample methods supported via blt for this D3DFORMAT
			} MultiSampleCaps;

		} DUMMYUNIONNAMEN(3);
		union
		{
			DWORD   dwBBitMask;             // mask for blue bits
			DWORD   dwVBitMask;             // mask for V bits
			DWORD   dwStencilBitMask;       // mask for stencil bits
			DWORD   dwBumpLuminanceBitMask; // mask for luminance in bump map
		} DUMMYUNIONNAMEN(4);
		union
		{
			DWORD   dwRGBAlphaBitMask;      // mask for alpha channel
			DWORD   dwYUVAlphaBitMask;      // mask for alpha channel
			DWORD   dwLuminanceAlphaBitMask;// mask for alpha channel
			DWORD   dwRGBZBitMask;          // mask for Z channel
			DWORD   dwYUVZBitMask;          // mask for Z channel
		} DUMMYUNIONNAMEN(5);
	} DDPIXELFORMAT;

	typedef struct _DDSCAPS2
	{
		DWORD       dwCaps;         // capabilities of surface wanted
		DWORD       dwCaps2;
		DWORD       dwCaps3;
		union
		{
			DWORD       dwCaps4;
			DWORD       dwVolumeDepth;
		} DUMMYUNIONNAMEN(1);
	} DDSCAPS2;

	typedef struct _DDSURFACEDESC2	
	{
		DWORD               dwSize;                 // size of the DDSURFACEDESC structure
		DWORD               dwFlags;                // determines what fields are valid
		DWORD               dwHeight;               // height of surface to be created
		DWORD               dwWidth;                // width of input surface
		union
		{
			LONG            lPitch;                 // distance to start of next line (return value only)
			DWORD           dwLinearSize;           // Formless late-allocated optimized surface size
		} DUMMYUNIONNAMEN(1);
		union
		{
			DWORD           dwBackBufferCount;      // number of back buffers requested
			DWORD           dwDepth;                // the depth if this is a volume texture 
		} DUMMYUNIONNAMEN(5);
		union
		{
			DWORD           dwMipMapCount;          // number of mip-map levels requestde
													// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
			DWORD           dwRefreshRate;          // refresh rate (used when display mode is described)
			DWORD           dwSrcVBHandle;          // The source used in VB::Optimize
		} DUMMYUNIONNAMEN(2);
		DWORD               dwAlphaBitDepth;        // depth of alpha buffer requested
		DWORD               dwReserved;             // reserved
		DWORD               lpSurface;              // pointer to the associated surface memory
		union
		{
			DDCOLORKEY      ddckCKDestOverlay;      // color key for destination overlay use
			DWORD           dwEmptyFaceColor;       // Physical color for empty cubemap faces
		} DUMMYUNIONNAMEN(3);
		DDCOLORKEY          ddckCKDestBlt;          // color key for destination blt use
		DDCOLORKEY          ddckCKSrcOverlay;       // color key for source overlay use
		DDCOLORKEY          ddckCKSrcBlt;           // color key for source blt use
		union
		{
			DDPIXELFORMAT   ddpfPixelFormat;        // pixel format description of the surface
			DWORD           dwFVF;                  // vertex format description of vertex buffers
		} DUMMYUNIONNAMEN(4);
		DDSCAPS2            ddsCaps;                // direct draw surface capabilities
		DWORD               dwTextureStage;         // stage in multitexture cascade
	} DDSURFACEDESC2;
#endif


class OceanSurface
{
	public:
	OceanSurface(bool use_texture_arrays);
	~OceanSurface();
	
	// Quadtree handle and shader mappings
	GFSDK_WaveWorks_QuadtreeHandle hOceanQuadTree;
	UINT* pQuadTreeShaderInputMappings;
	UINT* pSimulationShaderInputMappings;

	// Quadtree initialization
	gfsdk_waveworks_result InitQuadTree(const GFSDK_WaveWorks_Quadtree_Params& params);
	

	// Initializing OpenGL functions addresses
	HRESULT InitGLFunctions(void);

	// Creating OpenGL programs
	GLuint LoadProgram(char *);
	GLuint CompileShader(char *text, GLenum type);

	// Texture loading related methods
	int LoadTexture(char * filename, GLuint * texID);
	void CreateTextures(void);

	// Rendering related methods
	void SetupNormalView(void);
	void Render(GFSDK_WaveWorks_SimulationHandle hSim, GFSDK_WaveWorks_Simulation_Settings settings, bool Wireframe);

	// Constants
	UINT ScreenWidth;
	UINT ScreenHeight;
	UINT MultiSampleCount;
	UINT MultiSampleQuality;

	// Programs
	GLuint					WaterProgram;

	// Textures
	GLuint					FoamIntensityTextureID;
	GLuint					FoamIntensityTextureBindLocation;
	GLuint					FoamDiffuseTextureID;
	GLuint					FoamDiffuseTextureBindLocation;

	// GL resources allocated for WaveWorks during ocean surface rendering
	GFSDK_WaveWorks_Simulation_GL_Pool glPool;

	// Camera reated variables
	float					CameraPosition[3];
	float					LookAtPosition[3];

	float					NormalViewMatrix[4][4];
	float					NormalProjMatrix[4][4];
	float					NormalViewProjMatrix[4][4];

	// Counters
	double					total_time;
	float					delta_time;

	// Input
	int						MouseX,MouseY;
	float					MouseDX,MouseDY;
	float					Alpha;
	float					Beta;

	// GL functions
	PFNGLACTIVETEXTUREPROC glActiveTexture;
	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLBINDBUFFERPROC glBindBuffer;
	PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
	PFNGLBINDTEXTUREPROC glBindTexture;
	PFNGLBUFFERDATAPROC glBufferData;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
	PFNGLCLEARCOLORPROC glClearColor;
	PFNGLCLEARDEPTHFPROC glClearDepthf;
	PFNGLCLEARPROC glClear;
	PFNGLCOLORMASKPROC glColorMask;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLDELETEBUFFERSPROC glDeleteBuffers;
	PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLDELETETEXTURESPROC glDeleteTextures;
	PFNGLDISABLEPROC glDisable;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	PFNGLDRAWELEMENTSPROC glDrawElements;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	PFNGLGENBUFFERSPROC glGenBuffers;
	PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
	PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	PFNGLGENTEXTURESPROC glGenTextures;
	PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
	PFNGLGETERRORPROC glGetError;
	PFNGLGETINTEGERVPROC glGetIntegerv;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
	PFNGLPATCHPARAMETERIPROC glPatchParameteri;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLTEXIMAGE2DPROC glTexImage2D;
	PFNGLTEXPARAMETERFPROC glTexParameterf;
	PFNGLTEXPARAMETERIPROC glTexParameteri;
	PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
	PFNGLUNIFORM1FPROC glUniform1f;
	PFNGLUNIFORM1IPROC glUniform1i;
	PFNGLUNIFORM3FVPROC glUniform3fv;
	PFNGLUNIFORM4FVPROC glUniform4fv;
	PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	PFNGLUNMAPBUFFERPROC glUnmapBuffer;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLVIEWPORTPROC glViewport;
	PFNGLGENQUERIESPROC glGenQueries;
	PFNGLDELETEQUERIESPROC glDeleteQueries;
	PFNGLQUERYCOUNTERPROC glQueryCounter;
	PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;
	PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
	PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
	PFNGLTEXIMAGE3DPROC glTexImage3D;
	PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
	PFNGLREADBUFFERPROC glReadBuffer;
	PFNGLDRAWBUFFERSPROC glDrawBuffers;
};

void checkError(const char *);

