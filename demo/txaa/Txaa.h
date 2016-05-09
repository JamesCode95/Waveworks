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
// Copyright (C) 2012, NVIDIA Corporation. All rights reserved.
 
/*===========================================================================
                                  TXAA.H
=============================================================================
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

                       ________   __   __    __
                      (__    __)\/  ) /  \  /  \
                         |  | \    / /    \/    \
                         |  | /    \/  /\  \ /\  \
                         \__/(__/\__)_/  \__)  \__)

                    NVIDIA TXAA v2.Es by TIMOTHY LOTTES

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

-----------------------------------------------------------------------------
               SPECIAL VERSION OF TXAA FOR "E"ASY INTEGRATION
-----------------------------------------------------------------------------
 - Computes motion vector field from inside TXAA using depth.
 - DX11 only with feature level 10 shaders.
 
-----------------------------------------------------------------------------
                            ENGINEERING CONTACT
-----------------------------------------------------------------------------
Timothy Lottes (NVIDIA Devtech)
tlottes@nvidia.com

-----------------------------------------------------------------------------
                                 FXAA AND TXAA
-----------------------------------------------------------------------------
Don't use FXAA on top of TXAA as FXAA will introduce artifacts!

-----------------------------------------------------------------------------
                                   MODES
-----------------------------------------------------------------------------
TXAA 1x TEMPORAL SUPER-SAMPLING
 - Core TXAA modes
 - Softer film style resolve + reduction of temporal aliasing
 - Modes,
    - TXAA_MODE_2xMSAAx11 -> using 2xMSAA
    - TXAA_MODE_4xMSAAx1T -> using 4xMSAA

DEBUG PASS-THROUGH MODES
 - Test without TXAA but using the TXAA API
 - Modes,
    - TXAA_MODE_DEBUG_2xMSAA -> 2xMSAA
    - TXAA_MODE_DEBUG_4xMSAA -> 4xMSAA

DEBUG VIEW MOTION VECTOR INPUT
 - Coloring,
    - Red   -> objects moving left
    - Green -> objects moving up
    - Blue  -> areas which get temporal feedback
 - Mode: TXAA_MODE_DEBUG_VIEW_MV

DEBUG VIEW DIFFERENCE BETWEEN NON-BLENDED TEMPORAL SUPER-SAMPLING FRAMES
 - Shows difference of back-projected frame and current frame
    - Frames without TXAA filtering
 - Used to check if input view projection matrixes are correct
    - Should see black screen on a static image.
       - Or a convergance to a black screen on a static image.
    - Should see an outline on edges in motion.
       - Outline should not grow in size under motion.
    - Should see bright areas representing occlusion regions,
       - Ie the content is only visible in one frame.
    - Should see bright areas when motion exceeds the input limits.
       - Note the input limits are on a curve.
    - Red shows regions outside the motion limit for temporal feedback.
 - Modes,
    - TXAA_MODE_DEBUG_2xMSAAx1T_DIFF -> 2xMSAA
    - TXAA_MODE_DEBUG_4xMSAAx1T_DIFF -> 4xMSAA



-----------------------------------------------------------------------------
                                 PIPELINE
-----------------------------------------------------------------------------
TXAA Replaces the MSAA resolve step with this custom resolve pipeline,

 AxB sized MSAA RGBA color --------->  T  --> AxB sized RGBA resolved color
 AxB sized resolved depth         -->  X                           |
 AxB sized RGBA feedback ----------->  A                           |
  ^                                    A                           |
  |                                                                |
  +----------------------------------------------------------------+
  
TXAA needs two AxB sized resolve surfaces (because of the feedback). 

NOTE, if the AxB sized RGBA resolved color surface is going to be modified
after TXAA, then it needs to be copied, and the unmodified copy 
needs to be feed back into the next TXAA frame.

NOTE, resolved depth is best resolved for TXAA with the minimum depth 
of all samples in the pixel. 
If required one can write min depth for TXAA and max depth for particles
in the same resolve pass (if the game needs max depth for something else).


-----------------------------------------------------------------------------
                                SLI SCALING
-----------------------------------------------------------------------------
If the app hits SLI scaling problems because of TXAA,
there is an NVAPI interface which can improve performance.


-----------------------------------------------------------------------------
                          ALPHA CHANNEL IS NOT USED
-----------------------------------------------------------------------------
Currently TXAA does not resolve the Alpha channel.
Please contact NVIDIA if you need this changed.


-----------------------------------------------------------------------------
                                 USE LINEAR
-----------------------------------------------------------------------------
TXAA requires linear RGB color input.
 - Either use DXGI_FORMAT_*_SRGB for 8-bit/channel colors (LDR).
    - Make sure sRGB->linear (TEX) and linear->sRGB (ROP) conversion is on.
 - Or use DXGI_FORMAT_R16G16B16A16_FLOAT for HDR.

Tone-mapping the linear HDR data to low-dynamic-range is done after TXAA. 
For example, after post-processing at the time of color-grading.
The hack to tone-map prior to resolve, to workaround the artifacts 
of the traditional MSAA box filter resolve, is not needed with TXAA.
Tone-map prior to resolve will result on wrong colors on thin features.


-----------------------------------------------------------------------------
                               DRIVER SUPPORT
-----------------------------------------------------------------------------
TXAA requires driver support to work.


-----------------------------------------------------------------------------
                               GPU API STATE
-----------------------------------------------------------------------------
This library does not save and restore GPU state.
All modified GPU state is documented below by the function prototypes.


-----------------------------------------------------------------------------
                          MATRIX CONVENTION FOR INPUT
-----------------------------------------------------------------------------
Given a matrix,

  mxx mxy mxz mxw
  myx myy myz myw
  mzx mzy mzz mzw
  mwx mwy mwz mww

A matrix vector multiply is defined as,

  x' = x*mxx + y*mxy + z*mxz + w*mxw
  y' = x*myx + y*myy + z*myz + w*myw
  z' = x*mzx + y*mzy + z*mzz + w*mzw
  w' = x*mwx + y*mwy + z*mwz + w*mww


-----------------------------------------------------------------------------
                           TEMPORAL FEEDBACK CONTROL
-----------------------------------------------------------------------------
TXAA has some controls to turn off the temporal component
in areas where camera motion is known to be very wrong.
Areas for example,
 - The GUN in a FPS.
 - A vehicle the player is riding in a FPS.
This can greatly increase quality.

Pixel motion computed from depth which is larger than "motionLimitPixels" 
on the curve below will not contribute to temporal feedback.
  
 0 <---------------------DEPTH-----------------------> 1
                           .____________________________
                          /^       motionLimitPixels2
                         / |
 _______________________/  |
   motionLimitPixels1   ^  |
                        | depth2
                        |
                       depth1
                       
The cutoff in motion in pixels is a piece wise curve.
The near cutoff is "motionLimitPixels1".
The far cutoff is "motionLimitPixels2".
With "depth1" and "depth2" choosing the feather region.

Place "depth1" after the GUN in Z or after the inside of a vehicle in Z.
Place "depth2" some point after depth2 to feather the effect.

Set "motionLimitPixels1" to 0.125 to remove temporal feedback under motion.
This must not be ZERO!!!

Set "motionLimitPixels2" to around 16.0 (suggested) to 32.0 pixels.
This will enable temporal feedback when it needed,
and avoid any possible artifacts when temporal feedback won't be noticed.
This can help in in-vehicle cases where the world position inside a 
vehicle is rapidly changing, but says stationary with respect to the view.

                       
-----------------------------------------------------------------------------
                             INTEGRATION EXAMPLE
-----------------------------------------------------------------------------
(0.) OPEN CONTEXT AND CHECK FOR TXAA FEATURE SUPPORT IN DRIVER/GPU,

  #include "Txaa.h"
  TxaaCtx txaaCtx;
  ID3D11Device* dev;
  ID3D11DeviceContext* dxCtx;
  if(TxaaOpenDX(&txaaCtx, dev, dxCtx) == TXAA_RETURN_FAIL) // No TXAA.


(1.) REPLACE THE MSAA RESOLVE STEP WITH THE TXAA RESOLVE

  // DX11
  TxaaCtx* ctx; 
  ID3D11DeviceContext* dxCtx;         // DX11 device context.
  ID3D11RenderTargetView* dstRtv;     // Destination texture.
  ID3D11ShaderResourceView* msaaSrv;  // Source MSAA texture shader resource view.
  ID3D11ShaderResourceView* depthSrv; // Resolved depth (min depth of samples in pixel).
  ID3D11ShaderResourceView* dstSrv;   // SRV to feedback resolve results from prior frame.
  TxaaU4 mode = TXAA_MODE_4xMSAAx1T;  // TXAA resolve mode.
  // Source texture (msaaSrv) dimensions in pixels.  
  TxaaF4 width;
  TxaaF4 height;
  TxaaF4 depth1;             // first depth position 0-1 in Z buffer
  TxaaF4 depth2;             // second depth position 0-1 in Z buffer 
  TxaaF4 motionLimitPixels1; // first depth position motion limit in pixels
  TxaaF4 motionLimitPixels2; // second depth position motion limit in pixels
  TxaaF4* __TXAA_RESTRICT__ const mvpCurrent; // matrix for world to view projection (current frame)
  TxaaF4* __TXAA_RESTRICT__ const mvpPrior;   // matrix for world to view projection (prior frame)
  TxaaU4 frameCounter; // Increment every frame, notice the ping pong of dst.
  TxaaResolveDX(ctx, dxCtx, dstRtv[frameCounter&1],  
    msaaSrv, depthSrv, dstSrv[(frameCounter+1)&1], mode, width, height, 
    depth1, depth2, motionLimitPixels1, motionLimitPixels2, mvpCurrent, mvpPrior);


-----------------------------------------------------------------------------
                           INTEGRATION SUGGESTIONS
-----------------------------------------------------------------------------
(1.) Get the debug pass-through modes working,
     for instance TXAA_MODE_DEBUG_2xMSAA.
 
(2.) Get the temporal options working on still frames.
     First get the TXAA_MODE_<2|4>xMSAAx1T modes working first.

(2.a.) Verify motion vector field is correct
       using TXAA_MODE_DEBUG_VIEW_MV.
       Output should be,
         Red   -> objects moving left
         Green -> objects moving up
       If this is wrong, try the transpose of mvpCurrent and mvpPrior.
       Set {depth1=0.0, depth2=1.0, motionLimitPixels1=64.0, motionLimitPixels2=64.0}.

(2.b.) Verify motion vector field is correct 
       using TXAA_MODE_DEBUG_2xMSAAx1T_DIFF.
       Should see,
         No    Motion -> edges
         Small Motion -> edges maintaining brightness and thickness
      If seeing edges increase in size with a simple camera rotation,
      then mvpCurrent and/or mvpPrior is wrong.
       
(2.c.) Once motion field is verified,
       Then try the TXAA_MODE_2xMSAAx1T mode again.
       Everything should be working at this point.
         
(3.) Fine tune the motion limits,
       depth1 = where the GUN ends in Z buffer or
                where the vehicle the player is in ends in Z buffer
       depth2 = a feather region farther in Z from depth1
       motionLimitPixels1 = 0.125
                            This turns off temporal feedback
                            where camera motion is likely wrong (ie gun).
       motionLimitPixels2 = 16.0 to 32.0 pixels
                            This limits feedback outside the region
                            when motion gets really large
                            to increase quality.
         
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
===========================================================================*/
#ifndef __TXAA_H__
#define __TXAA_H__

/*===========================================================================
                                  INCLUDES 
===========================================================================*/
#include <windows.h>
#include <d3d11.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*===========================================================================

                               CC PORTING MSVC

===========================================================================*/
typedef unsigned char TxaaU1; 
typedef unsigned short TxaaU2;
typedef signed int TxaaS4; typedef unsigned int TxaaU4; 
typedef float TxaaF4; typedef double TxaaF8;
typedef signed long long TxaaS8; typedef unsigned long long TxaaU8;
/*-------------------------------------------------------------------------*/
#define __TXAA_ALIGN__(bytes) __declspec(align(bytes))
#define __TXAA_EXPECT__(exp, tf) (exp)
#define __TXAA_INLINE__ __forceinline
#define __TXAA_NOINLINE__
#define __TXAA_RESTRICT__ __restrict
/*-------------------------------------------------------------------------*/
#define __TXAA_CDECL__ __cdecl
#define __TXAA_IMPORT__ __declspec(dllimport)
#define __TXAA_EXPORT__ __declspec(dllexport)
#define __TXAA_STDCALL__ __stdcall

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*===========================================================================


                        TXAA CAMERA MOTION HELPER CODE


-----------------------------------------------------------------------------
 - This is the logic used inside the TXAA library.
 - It is visible here for reference.
===========================================================================*/

/*===========================================================================
                       INVERSION WITH FORMAT CONVERSION
===========================================================================*/
// Float matrix input and double output for TxaaCameraMotionConstants().
static __TXAA_INLINE__ void TxaaInverse4x4(
TxaaF8* __TXAA_RESTRICT__ const d, // destination
const TxaaF4* __TXAA_RESTRICT__ const s // source
) {
  TxaaF8 inv[16]; TxaaF8 det; TxaaU4 i;
/*-------------------------------------------------------------------------*/
  const TxaaF8 s0  = (TxaaF8)(s[ 0]); const TxaaF8 s1  = (TxaaF8)(s[ 1]); const TxaaF8 s2  = (TxaaF8)(s[ 2]); const TxaaF8 s3  = (TxaaF8)(s[ 3]);
  const TxaaF8 s4  = (TxaaF8)(s[ 4]); const TxaaF8 s5  = (TxaaF8)(s[ 5]); const TxaaF8 s6  = (TxaaF8)(s[ 6]); const TxaaF8 s7  = (TxaaF8)(s[ 7]);
  const TxaaF8 s8  = (TxaaF8)(s[ 8]); const TxaaF8 s9  = (TxaaF8)(s[ 9]); const TxaaF8 s10 = (TxaaF8)(s[10]); const TxaaF8 s11 = (TxaaF8)(s[11]);
  const TxaaF8 s12 = (TxaaF8)(s[12]); const TxaaF8 s13 = (TxaaF8)(s[13]); const TxaaF8 s14 = (TxaaF8)(s[14]); const TxaaF8 s15 = (TxaaF8)(s[15]);
/*-------------------------------------------------------------------------*/
  inv[0]  =  s5 * s10 * s15 - s5 * s11 * s14 - s9 * s6 * s15 + s9 * s7 * s14 + s13 * s6 * s11 - s13 * s7 * s10;
  inv[1]  = -s1 * s10 * s15 + s1 * s11 * s14 + s9 * s2 * s15 - s9 * s3 * s14 - s13 * s2 * s11 + s13 * s3 * s10;
  inv[2]  =  s1 * s6  * s15 - s1 * s7  * s14 - s5 * s2 * s15 + s5 * s3 * s14 + s13 * s2 * s7  - s13 * s3 * s6;
  inv[3]  = -s1 * s6  * s11 + s1 * s7  * s10 + s5 * s2 * s11 - s5 * s3 * s10 - s9  * s2 * s7  + s9  * s3 * s6;
  inv[4]  = -s4 * s10 * s15 + s4 * s11 * s14 + s8 * s6 * s15 - s8 * s7 * s14 - s12 * s6 * s11 + s12 * s7 * s10;
  inv[5]  =  s0 * s10 * s15 - s0 * s11 * s14 - s8 * s2 * s15 + s8 * s3 * s14 + s12 * s2 * s11 - s12 * s3 * s10;
  inv[6]  = -s0 * s6  * s15 + s0 * s7  * s14 + s4 * s2 * s15 - s4 * s3 * s14 - s12 * s2 * s7  + s12 * s3 * s6;
  inv[7]  =  s0 * s6  * s11 - s0 * s7  * s10 - s4 * s2 * s11 + s4 * s3 * s10 + s8  * s2 * s7  - s8  * s3 * s6;
  inv[8]  =  s4 * s9  * s15 - s4 * s11 * s13 - s8 * s5 * s15 + s8 * s7 * s13 + s12 * s5 * s11 - s12 * s7 * s9;
  inv[9]  = -s0 * s9  * s15 + s0 * s11 * s13 + s8 * s1 * s15 - s8 * s3 * s13 - s12 * s1 * s11 + s12 * s3 * s9;
  inv[10] =  s0 * s5  * s15 - s0 * s7  * s13 - s4 * s1 * s15 + s4 * s3 * s13 + s12 * s1 * s7  - s12 * s3 * s5;
  inv[11] = -s0 * s5  * s11 + s0 * s7  * s9  + s4 * s1 * s11 - s4 * s3 * s9  - s8  * s1 * s7  + s8  * s3 * s5;
  inv[12] = -s4 * s9  * s14 + s4 * s10 * s13 + s8 * s5 * s14 - s8 * s6 * s13 - s12 * s5 * s10 + s12 * s6 * s9;
  inv[13] =  s0 * s9  * s14 - s0 * s10 * s13 - s8 * s1 * s14 + s8 * s2 * s13 + s12 * s1 * s10 - s12 * s2 * s9;
  inv[14] = -s0 * s5  * s14 + s0 * s6  * s13 + s4 * s1 * s14 - s4 * s2 * s13 - s12 * s1 * s6  + s12 * s2 * s5;
  inv[15] =  s0 * s5  * s10 - s0 * s6  * s9  - s4 * s1 * s10 + s4 * s2 * s9  + s8  * s1 * s6  - s8  * s2 * s5;
/*-------------------------------------------------------------------------*/
  det = (s0 * inv[0] + s1 * inv[4] + s2 * inv[8] + s3 * inv[12]);
  if(det != 0.0) det = 1.0/det;
  for(i=0; i<16; i++) d[i] = inv[i] * det; }

/*===========================================================================
                       CAMERA MOTION CONSTANT GENERATION
-----------------------------------------------------------------------------
HLSL CODE FOR SHADER
--------------------
float2 CameraMotionVector(
float3 xyd, // {x,y} = position on screen range 0 to 1, d = fetched depth
float4 const0,  // constants generated by CameraMotionConstants() function
float4 const1,
float4 const2,
float4 const3,
float4 const4)
{ 
  float2 mv;
  float scaleM = 1.0/(dot(xyd, const0.xyz) + const0.w);
  mv.x = ((xyd.x * ((const1.x * xyd.y) + (const1.y * xyd.z) + const1.z)) + (const1.w * xyd.y) + (const2.x * xyd.x * xyd.x) + (const2.y * xyd.z) + const2.z) * scaleM;
  mv.y = ((xyd.y * ((const3.x * xyd.x) + (const3.y * xyd.z) + const3.z)) + (const3.w * xyd.x) + (const4.x * xyd.y * xyd.y) + (const4.y * xyd.z) + const4.z) * scaleM;
  return mv; }
===========================================================================*/
static __TXAA_INLINE__ void TxaaCameraMotionConstants(
TxaaF4* __TXAA_RESTRICT__ const cb, // constant buffer output
const TxaaF8* __TXAA_RESTRICT__ const c, // current view projection matrix inverse
const TxaaF4* __TXAA_RESTRICT__ const p  // prior view projection matrix
) {
/*-------------------------------------------------------------------------*/
  const TxaaF8 cxx = c[ 0]; const TxaaF8 cxy = c[ 1]; const TxaaF8 cxz = c[ 2]; const TxaaF8 cxw = c[ 3];
  const TxaaF8 cyx = c[ 4]; const TxaaF8 cyy = c[ 5]; const TxaaF8 cyz = c[ 6]; const TxaaF8 cyw = c[ 7];
  const TxaaF8 czx = c[ 8]; const TxaaF8 czy = c[ 9]; const TxaaF8 czz = c[10]; const TxaaF8 czw = c[11];
  const TxaaF8 cwx = c[12]; const TxaaF8 cwy = c[13]; const TxaaF8 cwz = c[14]; const TxaaF8 cww = c[15];
/*-------------------------------------------------------------------------*/
  const TxaaF8 pxx = (TxaaF8)(p[ 0]); const TxaaF8 pxy = (TxaaF8)(p[ 1]); const TxaaF8 pxz = (TxaaF8)(p[ 2]); const TxaaF8 pxw = (TxaaF8)(p[ 3]);
  const TxaaF8 pyx = (TxaaF8)(p[ 4]); const TxaaF8 pyy = (TxaaF8)(p[ 5]); const TxaaF8 pyz = (TxaaF8)(p[ 6]); const TxaaF8 pyw = (TxaaF8)(p[ 7]);
  const TxaaF8 pwx = (TxaaF8)(p[12]); const TxaaF8 pwy = (TxaaF8)(p[13]); const TxaaF8 pwz = (TxaaF8)(p[14]); const TxaaF8 pww = (TxaaF8)(p[15]);
/*-------------------------------------------------------------------------*/
  // c0
  cb[0] = (TxaaF4)(4.0*(cwx*pww + cxx*pwx + cyx*pwy + czx*pwz));
  cb[1] = (TxaaF4)((-4.0)*(cwy*pww + cxy*pwx + cyy*pwy + czy*pwz));
  cb[2] = (TxaaF4)(2.0*(cwz*pww + cxz*pwx + cyz*pwy + czz*pwz));
  cb[3] = (TxaaF4)(2.0*(cww*pww - cwx*pww + cwy*pww + (cxw - cxx + cxy)*pwx + (cyw - cyx + cyy)*pwy + (czw - czx + czy)*pwz));
/*-------------------------------------------------------------------------*/
  // c1
  cb[4] = (TxaaF4)(( 4.0)*(cwy*pww + cxy*pwx + cyy*pwy + czy*pwz));
  cb[5] = (TxaaF4)((-2.0)*(cwz*pww + cxz*pwx + cyz*pwy + czz*pwz));
  cb[6] = (TxaaF4)((-2.0)*(cww*pww + cwy*pww + cxw*pwx - 2.0*cxx*pwx + cxy*pwx + cyw*pwy - 2.0*cyx*pwy + cyy*pwy + czw*pwz - 2.0*czx*pwz + czy*pwz - cwx*(2.0*pww + pxw) - cxx*pxx - cyx*pxy - czx*pxz));  
  cb[7] = (TxaaF4)(-2.0*(cyy*pwy + czy*pwz + cwy*(pww + pxw) + cxy*(pwx + pxx) + cyy*pxy + czy*pxz));  
/*-------------------------------------------------------------------------*/
  // c2
  cb[ 8] = (TxaaF4)((-4.0)*(cwx*pww + cxx*pwx + cyx*pwy + czx*pwz));  
  cb[ 9] = (TxaaF4)(cyz*pwy + czz*pwz + cwz*(pww + pxw) + cxz*(pwx + pxx) + cyz*pxy + czz*pxz);  
  cb[10] = (TxaaF4)(cwy*pww + cwy*pxw + cww*(pww + pxw) - cwx*(pww + pxw) + (cxw - cxx + cxy)*(pwx + pxx) + (cyw - cyx + cyy)*(pwy + pxy) + (czw - czx + czy)*(pwz + pxz));  
  cb[11] = (TxaaF4)(0);  
/*-------------------------------------------------------------------------*/
  // c3
  cb[12] = (TxaaF4)((-4.0)*(cwx*pww + cxx*pwx + cyx*pwy + czx*pwz));  
  cb[13] = (TxaaF4)((-2.0)*(cwz*pww + cxz*pwx + cyz*pwy + czz*pwz));  
  cb[14] = (TxaaF4)(2.0*((-cww)*pww + cwx*pww - 2.0*cwy*pww - cxw*pwx + cxx*pwx - 2.0*cxy*pwx - cyw*pwy + cyx*pwy - 2.0*cyy*pwy - czw*pwz + czx*pwz - 2.0*czy*pwz + cwy*pyw + cxy*pyx + cyy*pyy + czy*pyz));  
  cb[15] = (TxaaF4)(2.0*(cyx*pwy + czx*pwz + cwx*(pww - pyw) + cxx*(pwx - pyx) - cyx*pyy - czx*pyz));  
/*-------------------------------------------------------------------------*/
  // c4
  cb[16] = (TxaaF4)(4.0*(cwy*pww + cxy*pwx + cyy*pwy + czy*pwz));  
  cb[17] = (TxaaF4)(cyz*pwy + czz*pwz + cwz*(pww - pyw) + cxz*(pwx - pyx) - cyz*pyy - czz*pyz);
  cb[18] = (TxaaF4)(cwy*pww + cww*(pww - pyw) - cwy*pyw + cwx*((-pww) + pyw) + (cxw - cxx + cxy)*(pwx - pyx) + (cyw - cyx + cyy)*(pwy - pyy) + (czw - czx + czy)*(pwz - pyz));  
  cb[19] = (TxaaF4)(0); }

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*===========================================================================


                                    API


===========================================================================*/

/*===========================================================================
                                RETURN CODES
===========================================================================*/
#define TXAA_RETURN_OK   0
#define TXAA_RETURN_FAIL 1

/*===========================================================================
                                 TXAA MODES
===========================================================================*/
#define TXAA_MODE_2xMSAAx1T 0
#define TXAA_MODE_4xMSAAx1T 1
/*-------------------------------------------------------------------------*/
#define TXAA_MODE_DEBUG_VIEW_MV 2
/*-------------------------------------------------------------------------*/
#define TXAA_MODE_DEBUG_2xMSAA 3
#define TXAA_MODE_DEBUG_4xMSAA 4
/*-------------------------------------------------------------------------*/
#define TXAA_MODE_DEBUG_2xMSAAx1T_DIFF 5
#define TXAA_MODE_DEBUG_4xMSAAx1T_DIFF 6
/*-------------------------------------------------------------------------*/
#define TXAA_MODE_TOTAL 7

/*===========================================================================

                                 FUNCTIONS

===========================================================================*/
#ifndef __TXAA_CPP__
extern "C" {

/*===========================================================================
                                TXAA CONTEXT
===========================================================================*/
  typedef struct { __TXAA_ALIGN__(64) TxaaU4 pad[256]; } TxaaCtxDX; 

/*===========================================================================
                                      OPEN
-----------------------------------------------------------------------------
 - Check if GPU supports TXAA and open context.
 - Note this might be a high latency operation as it does a GPU->CPU read.
    - Might want to spawn a thread to do this async from normal load.
 - Returns,
     TXAA_RETURN_OK        -> Success and valid context.
                              Make sure to call TxaaCloseDX()
                              when finished with TXAA context.
     TXAA_RETURN_FAIL      -> Fail attempting to open TXAA.
                              No need to call TxaaCloseDX().
-----------------------------------------------------------------------------
 - Same side effects as TxaaResolveDX().
===========================================================================*/
  TxaaU4 TxaaOpenDX(
  TxaaCtxDX* __TXAA_RESTRICT__ const ctx,
  ID3D11Device* __TXAA_RESTRICT__ const dev,
  ID3D11DeviceContext* __TXAA_RESTRICT__ const dxCtx);

/*===========================================================================
                                   CLOSE
-----------------------------------------------------------------------------
 - Close a TXAA instance.
 - Has no GPU state side effects (only frees resources).
===========================================================================*/
  void TxaaCloseDX(
  TxaaCtxDX* __TXAA_RESTRICT__ const ctx);

/*===========================================================================
                                  RESOLVE
-----------------------------------------------------------------------------
 - Use in-place of standard MSAA resolve.
-----------------------------------------------------------------------------
 - Source MSAA or non-MSAA texture has the following size in pixels,
     {width, height}
 - Depth texture should be resolve MSAA depth buffer.
 - Destination surfaces need to be the following size in pixels,
     {width, height}
-----------------------------------------------------------------------------
 - DX11 side effects,
    - OMSetRenderTargets()
    - PSSetShaderResources()
    - RSSetViewports()
    - VSSetShader()
    - PSSetShader()
    - PSSetSamplers()
    - VSSetConstantBuffers()
    - PSSetConstantBuffers()
    - RSSetState()
    - OMSetDepthStencilState()
    - OMSetBlendState()
    - IASetInputLayout()
    - IASetPrimitiveTopology()
===========================================================================*/
  void TxaaResolveDX(
  TxaaCtxDX* __TXAA_RESTRICT__ const ctx, 
  ID3D11DeviceContext* __TXAA_RESTRICT__ const dxCtx,         // DX11 device context.
  ID3D11RenderTargetView* __TXAA_RESTRICT__ const dstRtv,     // Destination texture.
  ID3D11ShaderResourceView* __TXAA_RESTRICT__ const msaaSrv,  // Source MSAA texture shader resource view.
  ID3D11ShaderResourceView* __TXAA_RESTRICT__ const depthSrv, // Resolved depth (min depth of samples in pixel).
  ID3D11ShaderResourceView* __TXAA_RESTRICT__ const feedSrv,  // SRV to destination from prior frame.
  const TxaaU4 mode,      // TXAA_MODE_* select TXAA resolve mode.
  const TxaaF4 width,     // // Source/destination texture dimensions in pixels.
  const TxaaF4 height,
  const TxaaF4 depth1,             // first depth position 0-1 in Z buffer
  const TxaaF4 depth2,             // second depth position 0-1 in Z buffer 
  const TxaaF4 motionLimitPixels1, // first depth position motion limit in pixels
  const TxaaF4 motionLimitPixels2, // second depth position motion limit in pixels
  const TxaaF4* __TXAA_RESTRICT__ const mvpCurrent, // matrix for world to view projection (current frame)
  const TxaaF4* __TXAA_RESTRICT__ const mvpPrior);  // matrix for world to view projection (prior frame)
}
#endif /* !__TXAA_CPP__ */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif /* __TXAA_H__ */