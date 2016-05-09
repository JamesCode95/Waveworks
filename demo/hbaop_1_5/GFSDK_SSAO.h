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
// Copyright (c) 2008-2014 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

/*====================================================================================================
-------------------------------------------------------------------------------------------
                                           HBAO+                                           
-------------------------------------------------------------------------------------------

HBAO+ is a SSAO algorithm designed to achieve high efficiency on DX11 GPUs.
The algorithm is based on HBAO [Bavoil and Sainz 2008], with the following differences:

(1.) To minimize cache trashing, HBAO+ does not use any randomization texture.
Instead, the algorithm uses an Interleaved Rendering approach, generating the AO
in multiple passes with a unique jitter value per pass [Bavoil and Jansen 2013].

(2.) To avoid over-occlusion artifacts, HBAO+ uses a simpler AO approximation than HBAO,
similar to "Scalable Ambient Obscurance" [McGuire et al. 2012] [Bukowski et al. 2012].

(3.) To minimize flickering, the HBAO+ is always rendered in full resolution,
from full-resolution depths.

[Bavoil et al. 2008] "Image-Space Horizon-Based Ambient Occlusion"
http://www.nvidia.com/object/siggraph-2008-HBAO.html

[McGuire et al. 2012] "Scalable Ambient Obscurance"
http://graphics.cs.williams.edu/papers/SAOHPG12/

[Bukowski et al. 2012] "Scalable High-Quality Motion Blur and Ambient Occlusion"
http://graphics.cs.williams.edu/papers/VVSIGGRAPH12/

[Bavoil and Jansen 2013] "Particle Shadows & Cache-Efficient Post-Processing"
https://developer.nvidia.com/gdc-2013

-------------------------------------------------------------------------------------------
                                       PERFORMANCE                                        
-------------------------------------------------------------------------------------------

On GeForce GTX 680 in 1920x1200 1xAA, using HARDWARE_DEPTHS as input and BLUR_RADIUS_8,
the RenderAO call takes up to 2.7 ms of GPU time per frame and 46 MB of video memory.

-------------------------------------------------------------------------------------------
                                    INTEGRATION EXAMPLE                                    
-------------------------------------------------------------------------------------------

(1.) INITIALIZE THE LIBRARY

    GFSDK_SSAO_CustomHeap CustomHeap;
    CustomHeap.new_ = ::operator new;
    CustomHeap.delete_ = ::operator delete;

    GFSDK_SSAO_Status status;
    GFSDK_SSAO_Context* pAOContext;
    status = GFSDK_SSAO_CreateContext(pD3D11Device, &pAOContext, &CustomHeap);
    assert(status == GFSDK_SSAO_OK); // HBAO+ requires feature level 11_0 or above

(2.) SET INPUT DEPTHS

    GFSDK_SSAO_InputData Input;
    Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
    Input.DepthData.pFullResDepthTextureSRV = pDepthStencilTextureSRV;
    Input.DepthData.pViewport = &Viewport;
    Input.DepthData.pProjectionMatrix = pProjectionMatrix;
    Input.DepthData.ProjectionMatrixLayout = GFSDK_SSAO_ROW_MAJOR_ORDER;
    Input.DepthData.MetersToViewSpaceUnits = SceneScale;

(3.) SET AO PARAMETERS

    GFSDK_SSAO_Parameters Params;
    Params.Radius = 2.f;
    Params.Bias = 0.1f;
    Params.PowerExponent = 2.f;
    Params.Blur.Enable = TRUE;
    Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_8;
    Params.Blur.Sharpness = 4.f;
    Params.Output.BlendMode = GFSDK_SSAO_OVERWRITE_RGB;

(4.) RENDER AO

    status = pAOContext->RenderAO(pD3D11Context, &Input, &Params, pOutputColorRTV);
    assert(status == GFSDK_SSAO_OK);

-------------------------------------------------------------------------------------------
                                 DOCUMENTATION AND SUPPORT                                 
-------------------------------------------------------------------------------------------

    See HBAO+_PartnerInfo.pdf for more information on the input requirements and the parameters.
    For any feedback or questions about this library, please contact devsupport@nvidia.com.

====================================================================================================*/

#pragma once
#pragma pack(push,8) // Make sure we have consistent structure packings

#include <d3d11.h>

#if defined(__d3d11_h__)

/*====================================================================================================
   Entry-point declarations.
====================================================================================================*/

#if !_WINDLL
#define GFSDK_SSAO_DECL(RETURN_TYPE) extern RETURN_TYPE __cdecl
#else
#define GFSDK_SSAO_DECL(RETURN_TYPE) __declspec(dllexport) RETURN_TYPE __cdecl
#endif

/*====================================================================================================
   Build version.
====================================================================================================*/

#define GFSDK_SSAO_BRANCH "r1.5"
#define GFSDK_SSAO_BRANCH_VERSION 0
#define GFSDK_SSAO_HEADER_VERSION 17786695

/*====================================================================================================
   Enums.
====================================================================================================*/

enum GFSDK_SSAO_Status
{
    GFSDK_SSAO_OK,                                          // Success
    GFSDK_SSAO_VERSION_MISMATCH,                            // The header version number does not match the DLL version number
    GFSDK_SSAO_NULL_ARGUMENT,                               // One of the required argument pointers is NULL
    GFSDK_SSAO_INVALID_PROJECTION_MATRIX,                   // The projection matrix is not valid (off-centered?)
    GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX,                // The world-to-view matrix is not valid (transposing it may help)
    GFSDK_SSAO_INVALID_DEPTH_TEXTURE,                       // The depth texture is not valid (MSAA view-depth textures are not supported)
    GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION,           // The normal-texture resolution does not match the depth-texture resolution
    GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT,         // The normal-texture sample count does not match the depth-texture sample count
    GFSDK_SSAO_INVALID_VIEWPORT_DIMENSIONS,                 // The viewport rectangle expands outside of the input depth texture
    GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE,                // The viewport depth range is not a sub-range of [0.f,1.f]
    GFSDK_SSAO_INVALID_OUTPUT_MSAA_SAMPLE_COUNT,            // MSAASampleCount(OutputColorRT) > MSAASampleCount(InputDepthTexture)
    GFSDK_SSAO_UNSUPPORTED_VIEWPORT_DIMENSIONS,             // View depths are used as input together with a partial viewport (not supported)
    GFSDK_SSAO_UNSUPPORTED_D3D_FEATURE_LEVEL,               // The current D3D11 feature level is lower than 11_0
    GFSDK_SSAO_RESOURCE_CREATION_FAILED,                    // A D3D resource-creation call has failed (running out of memory?)
    GFSDK_SSAO_MEMORY_ALLOCATION_FAILED,                    // Failed to allocate memory on the heap
};

enum GFSDK_SSAO_GPUConfiguration
{
    GFSDK_SSAO_SINGLE_GPU,                                  // The application is rendering on a single GPU
    GFSDK_SSAO_MULTI_GPU,                                   // The application is rendering on multiple GPUs (e.g. SLI)
};

enum GFSDK_SSAO_DepthTextureType
{
    GFSDK_SSAO_HARDWARE_DEPTHS,                             // Non-linear depths in the range [0.f,1.f]
    GFSDK_SSAO_HARDWARE_DEPTHS_SUB_RANGE,                   // Non-linear depths in the range [Viewport.MinDepth,Viewport.MaxDepth]
    GFSDK_SSAO_VIEW_DEPTHS,                                 // Linear depths in the range [ZNear,ZFar]
};

enum GFSDK_SSAO_BlendMode
{
    GFSDK_SSAO_OVERWRITE_RGB,                               // Overwrite the destination RGB with the AO, preserving alpha
    GFSDK_SSAO_MULTIPLY_RGB,                                // Multiply the AO over the destination RGB, preserving alpha
    GFSDK_SSAO_CUSTOM_BLEND,                                // Composite the AO using a custom D3D11 blend state
};

enum GFSDK_SSAO_BlurRadius
{
    GFSDK_SSAO_BLUR_RADIUS_4,                               // Kernel radius = 4 pixels
    GFSDK_SSAO_BLUR_RADIUS_8,                               // Kernel radius = 8 pixels
};

enum GFSDK_SSAO_MSAAMode
{
    GFSDK_SSAO_PER_PIXEL_AO,                                // Render one AO value per output pixel (recommended)
    GFSDK_SSAO_PER_SAMPLE_AO,                               // Render one AO value per output MSAA sample (slower)
};

enum GFSDK_SSAO_MatrixLayout
{
    GFSDK_SSAO_ROW_MAJOR_ORDER,                             // The matrix is stored as Row[0],Row[1],Row[2],Row[3]
    GFSDK_SSAO_COLUMN_MAJOR_ORDER,                          // The matrix is stored as Col[0],Col[1],Col[2],Col[3]
};

enum GFSDK_SSAO_RenderMask
{
    GFSDK_SSAO_DRAW_Z                              = 1,     // Linearize the input depths
    GFSDK_SSAO_DRAW_AO                             = 2,     // Render AO based on pre-linearized depths
    GFSDK_SSAO_DRAW_DEBUG_N                        = 4,     // Render Color.rgba = -InternalViewNormal.z (for debugging)
    GFSDK_SSAO_RENDER_AO                           = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_AO,
    GFSDK_SSAO_RENDER_DEBUG_NORMAL_Z               = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_DEBUG_N,
};

/*====================================================================================================
   Input data.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// Input depth data.
//
// Requirements:
//    * View-space depths (linear) are required to be non-multisample.
//    * Hardware depths (non-linear) can be multisample or non-multisample.
//    * The projection matrix must have the following form, with |P23| == 1.f:
//       { P00, 0.f, 0.f, 0.f }
//       { 0.f, P11, 0.f, 0.f }
//       { 0.f, 0.f, P22, P23 }
//       { 0.f, 0.f, P32, 0.f }
//
// Remarks:
//    * MetersToViewSpaceUnits is used to convert the AO radius parameter from meters to view-space units,
//      as well as to convert the blur sharpness parameter from inverse meters to inverse view-space units.
//    * The Viewport defines a sub-area of the input & output full-resolution textures to be sourced and rendered to.
//      Only the depth values within the viewport sub-area contribute to the RenderAO output.
//    * The Viewport's MinDepth and MaxDepth values are ignored except if DepthTextureType is HARDWARE_DEPTHS_SUB_RANGE.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_InputDepthData
{
    GFSDK_SSAO_DepthTextureType DepthTextureType;           // HARDWARE_DEPTHS, HARDWARE_DEPTHS_SUB_RANGE or VIEW_DEPTHS
    ID3D11ShaderResourceView*   pFullResDepthTextureSRV;    // Full-resolution depth texture
    const D3D11_VIEWPORT*       pViewport;                  // Viewport from the depth generation pass
    const FLOAT*                pProjectionMatrix;          // 4x4 perspective matrix from the depth generation pass
    GFSDK_SSAO_MatrixLayout     ProjectionMatrixLayout;     // Memory layout of the projection matrix
    FLOAT                       MetersToViewSpaceUnits;     // DistanceInViewSpaceUnits = MetersToViewSpaceUnits * DistanceInMeters

    GFSDK_SSAO_InputDepthData()
        : DepthTextureType(GFSDK_SSAO_HARDWARE_DEPTHS)
        , pFullResDepthTextureSRV(NULL)
        , pViewport(NULL)
        , pProjectionMatrix(NULL)
        , ProjectionMatrixLayout(GFSDK_SSAO_ROW_MAJOR_ORDER)
        , MetersToViewSpaceUnits(1.f)
     {
     }
};

//---------------------------------------------------------------------------------------------------
// [Optional] Input normal data.
//
// Requirements:
//    * The normal texture is required to contain world-space normals in RGB.
//    * The normal texture must have the same resolution and MSAA sample count as the input depth texture.
//    * The view-space Z axis is assumed to be pointing away from the viewer (left-handed projection).
//    * The WorldToView matrix is assumed to not contain any non-uniform scaling.
//    * The WorldView matrix must have the following form:
//       { M00, M01, M02, 0.f }
//       { M10, M11, M12, 0.f }
//       { M20, M21, M22, 0.f }
//       { M30, M31, M32, M33 }
//
// Remarks:
//    * The actual view-space normal used for the AO rendering is:
//      N = normalize( mul( FetchedNormal.xyz * DecodeScale + DecodeBias, (float3x3)WorldToViewMatrix ) )
//    * Using bent normals as input may result in false-occlusion (overdarkening) artifacts.
//      Such artifacts may be alleviated by increasing the AO Bias parameter.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_InputNormalData
{
    ID3D11ShaderResourceView*   pFullResNormalTextureSRV;   // Full-resolution world-space normal texture
    const FLOAT*                pWorldToViewMatrix;         // 4x4 WorldToView matrix from the depth generation pass
    GFSDK_SSAO_MatrixLayout     WorldToViewMatrixLayout;    // Memory layout of the WorldToView matrix
    FLOAT                       DecodeScale;                // Optional pre-matrix scale
    FLOAT                       DecodeBias;                 // Optional pre-matrix bias

    GFSDK_SSAO_InputNormalData()
        : pFullResNormalTextureSRV(NULL)
        , pWorldToViewMatrix(NULL)
        , WorldToViewMatrixLayout(GFSDK_SSAO_ROW_MAJOR_ORDER)
        , DecodeScale(1.f)
        , DecodeBias(0.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_InputData
{
    GFSDK_SSAO_InputDepthData           DepthData;          // Required
    const GFSDK_SSAO_InputNormalData*   pNormalData;        // Optional (set to NULL to disable)

    GFSDK_SSAO_InputData()
        : pNormalData(NULL)
    {
    }
};

/*====================================================================================================
   Parameters.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// When enabled, the actual per-pixel blur sharpness value depends on the per-pixel view depth with:
//     LerpFactor = (PixelViewDepth - ForegroundViewDepth) / (BackgroundViewDepth - ForegroundViewDepth)
//     Sharpness = lerp(Sharpness*ForegroundSharpnessScale, Sharpness, saturate(LerpFactor))
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_BlurSharpnessProfile
{
    BOOL                        Enable;                     // To make the blur sharper in the foreground
    FLOAT                       ForegroundSharpnessScale;   // Sharpness scale factor for ViewDepths <= ForegroundViewDepth
    FLOAT                       ForegroundViewDepth;        // Maximum view depth of the foreground depth range
    FLOAT                       BackgroundViewDepth;        // Minimum view depth of the background depth range

    GFSDK_SSAO_BlurSharpnessProfile ()
        : Enable(FALSE)
        , ForegroundSharpnessScale(4.f)
        , ForegroundViewDepth(0.f)
        , BackgroundViewDepth(1.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_BlurParameters
{
    BOOL                            Enable;                 // To blur the AO with an edge-preserving blur
    GFSDK_SSAO_BlurRadius           Radius;                 // BLUR_RADIUS_4 or BLUR_RADIUS_8
    FLOAT                           Sharpness;              // The higher, the more the blur preserves edges // 0.0~16.0
    GFSDK_SSAO_BlurSharpnessProfile SharpnessProfile;       // Optional depth-dependent sharpness function

    GFSDK_SSAO_BlurParameters()
        : Enable(TRUE)
        , Radius(GFSDK_SSAO_BLUR_RADIUS_8)
        , Sharpness(4.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_DepthThreshold
{
    BOOL                            Enable;                 // To return white AO for ViewDepths > MaxViewDepth
    FLOAT                           MaxViewDepth;           // Custom view-depth threshold

    GFSDK_SSAO_DepthThreshold()
        : Enable(FALSE)
        , MaxViewDepth(0.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_CustomBlendState
{
    ID3D11BlendState*               pBlendState;            // Custom blend state to composite the AO with
    const FLOAT*                    pBlendFactor;           // Relevant only if pBlendState uses D3D11_BLEND_BLEND_FACTOR

    GFSDK_SSAO_CustomBlendState()
        : pBlendState(NULL)
        , pBlendFactor(NULL)
    {
    }
};

//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_OutputParameters
{
    GFSDK_SSAO_BlendMode            BlendMode;              // Blend mode used to composite the AO to the output render target
    GFSDK_SSAO_CustomBlendState     CustomBlendState;       // Relevant only if BlendMode is CUSTOM_BLEND
    GFSDK_SSAO_MSAAMode             MSAAMode;               // Relevant only if the input and output textures are multisample

    GFSDK_SSAO_OutputParameters()
        : BlendMode(GFSDK_SSAO_OVERWRITE_RGB)
        , MSAAMode(GFSDK_SSAO_PER_PIXEL_AO)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// Remarks:
//    * The final occlusion is a weighted sum of 2 occlusion contributions. The DetailAO and CoarseAO parameters are the weights.
//    * Setting the DetailAO parameter to 0.0 (default value) is fastest and avoids over-occlusion artifacts on alpha-tested geometry.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_Parameters
{
    FLOAT                           Radius;                 // The AO radius in meters
    FLOAT                           Bias;                   // To hide low-tessellation artifacts // 0.0~1.0
    FLOAT                           DetailAO;               // Scale factor for the detail AO, the greater the darker // 0.0~2.0
    FLOAT                           CoarseAO;               // Scale factor for the coarse AO, the greater the darker // 0.0~2.0
    FLOAT                           PowerExponent;          // The final AO output is pow(AO, powerExponent)
    GFSDK_SSAO_GPUConfiguration     GPUConfiguration;       // To avoid inter-frame dependencies with MULTI_GPU rendering
    GFSDK_SSAO_DepthThreshold       DepthThreshold;         // Optional Z threshold, to hide possible depth-precision artifacts
    GFSDK_SSAO_BlurParameters       Blur;                   // Optional AO blur, to blur the AO before compositing it
    GFSDK_SSAO_OutputParameters     Output;                 // To composite the AO with the output render target

    GFSDK_SSAO_Parameters()
        : Radius(1.f)
        , Bias(0.1f)
        , DetailAO(0.f)
        , CoarseAO(1.f)
        , PowerExponent(2.f)
        , GPUConfiguration(GFSDK_SSAO_SINGLE_GPU)
    {
    }
};

/*====================================================================================================
   Interface.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// Note: The RenderAO, PreCreateRTs and Release entry points should not be called simultaneously from different threads.
//---------------------------------------------------------------------------------------------------
class GFSDK_SSAO_Context
{
public:

//---------------------------------------------------------------------------------------------------
// Renders SSAO to pOutputColorRT.
//
// Remarks:
//    * Allocates internal D3D render targets on first use, and re-allocates them when the depth-texture resolution changes.
//    * All the relevant device-context states are saved and restored internally when entering and exiting the call.
//    * Setting RenderMask = GFSDK_SSAO_DRAW_DEBUG_N can be useful to visualize the normals used for the AO rendering.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid (off-centered?)
//     GFSDK_SSAO_INVALID_VIEWPORT_DIMENSIONS          - The viewport rectangle expands outside of the input depth texture
//     GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE         - The viewport depth range is not a sub-range of [0.f,1.f]
//     GFSDK_SSAO_INVALID_DEPTH_TEXTURE                - The depth texture is not valid (MSAA view-depth textures are not supported)
//     GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX         - The world-to-view matrix is not valid (transposing it may help)
//     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION    - The normal-texture resolution does not match the depth-texture resolution
//     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT  - The normal-texture sample count does not match the depth-texture sample count
//     GFSDK_SSAO_UNSUPPORTED_VIEWPORT_DIMENSIONS      - View depths are used as input together with a partial viewport (not supported)
//     GFSDK_SSAO_INVALID_OUTPUT_MSAA_SAMPLE_COUNT     - MSAASampleCount(OutputColorRT) > MSAASampleCount(InputDepthTexture)
//     GFSDK_SSAO_RESOURCE_CREATION_FAILED,            - A D3D resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
virtual GFSDK_SSAO_Status RenderAO(ID3D11DeviceContext* pDeviceContext,
                                   const GFSDK_SSAO_InputData* pInputData,
                                   const GFSDK_SSAO_Parameters* pParameters,
                                   ID3D11RenderTargetView* pOutputColorRT,
                                   GFSDK_SSAO_RenderMask RenderMask = GFSDK_SSAO_RENDER_AO) = 0;

//---------------------------------------------------------------------------------------------------
// [Optional] Pre-creates all internal render targets for RenderAO.
//
// Remarks:
//    * This call may be safely skipped since RenderAO creates its render targets on demand if they were not pre-created.
//    * This call releases and re-creates the internal render targets if the provided resolution changes.
//    * This call performs CreateTexture calls for all the relevant render targets.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_RESOURCE_CREATION_FAILED,            - A D3D resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
virtual GFSDK_SSAO_Status PreCreateRTs(const GFSDK_SSAO_Parameters* pParameters,
                                       UINT ViewportWidth,
                                       UINT ViewportHeight) = 0;

//---------------------------------------------------------------------------------------------------
// [Optional] Returns the amount of video memory allocated by the library, in bytes.
//---------------------------------------------------------------------------------------------------
virtual UINT GetAllocatedVideoMemoryBytes() = 0;

//---------------------------------------------------------------------------------------------------
// [Optional] Returns a pointer to a string literal containing the build version number.
//---------------------------------------------------------------------------------------------------
virtual const char* GetBuildString() = 0;

//---------------------------------------------------------------------------------------------------
// Releases all D3D objects created by the library (to be called right before releasing the D3D device).
//---------------------------------------------------------------------------------------------------
virtual void Release() = 0;

}; //class GFSDK_SSAO_Context

//---------------------------------------------------------------------------------------------------
// [Optional] Custom heap
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_CustomHeap
{
    void* (*new_)(size_t);
    void (*delete_)(void*);
};

//---------------------------------------------------------------------------------------------------
// Creates a GFSDK_SSAO_Context associated with the D3D11 device.
//
// Remarks:
//    * Allocates D3D11 resources internally.
//    * Allocates memory using the default "::operator new", or "pCustomHeap->new_" if provided.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_VERSION_MISMATCH                     - HeaderVersion != GFSDK_SSAO_HEADER_VERSION
//     GFSDK_SSAO_MEMORY_ALLOCATION_FAILED             - Failed to allocate memory on the heap
//     GFSDK_SSAO_UNSUPPORTED_D3D_FEATURE_LEVEL        - The D3D11 feature level of pD3DDevice is lower than 11_0
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
GFSDK_SSAO_DECL(GFSDK_SSAO_Status) GFSDK_SSAO_CreateContext(ID3D11Device* pD3DDevice,
                                                            GFSDK_SSAO_Context** ppContext,
                                                            const GFSDK_SSAO_CustomHeap* pCustomHeap = NULL,
                                                            UINT HeaderVersion = GFSDK_SSAO_HEADER_VERSION);

#endif //defined(__d3d11_h__)

#pragma pack(pop)
