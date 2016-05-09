#include "Common.fxh"
#ifdef GFSDK_WAVEWORKS_GL
#define DECLARE_ATTR_CONSTANT(Type,Label,Regoff) uniform Type Label
#define DECLARE_ATTR_SAMPLER(Label,TextureLabel,Regoff) \
	uniform sampler2D TextureLabel
#else
#define DECLARE_ATTR_CONSTANT(Type,Label,Regoff) Type Label : register(c##Regoff)
#define DECLARE_ATTR_SAMPLER(Label,TextureLabel,Regoff) \
	Texture2D Label : register(t##Regoff);	\
	SamplerState TextureLabel : register(s##Regoff)
#endif
BEGIN_CBUFFER(nv_waveworks_impl_0_0,0)
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_1,        0); 
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_2, 1);
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_3,2);
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_4, 3);
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_5,4);
END_CBUFFER
DECLARE_ATTR_SAMPLER(nv_waveworks_impl_0_6,nv_waveworks_impl_0_7,0);
#ifdef GFSDK_WAVEWORKS_GL
varying float2 nv_waveworks_impl_0_8;
#endif
#ifndef GFSDK_WAVEWORKS_OMIT_VS
#ifdef GFSDK_WAVEWORKS_GL
attribute float4 nv_waveworks_impl_0_9;
attribute float2 nv_waveworks_impl_0_10;
#define nv_waveworks_impl_0_11 gl_Position
void main()
#else
void vs(
	float4 nv_waveworks_impl_0_9 SEMANTIC(POSITION),
	float2 nv_waveworks_impl_0_10 SEMANTIC(TEXCOORD0),
	out float2 nv_waveworks_impl_0_8 SEMANTIC(TEXCOORD0),
	out float4 nv_waveworks_impl_0_11 SEMANTIC(SV_Position)
)
#endif
{
    nv_waveworks_impl_0_11 = nv_waveworks_impl_0_9;
    nv_waveworks_impl_0_8 = nv_waveworks_impl_0_10;
}
#endif 
#ifndef GFSDK_WAVEWORKS_OMIT_PS
#ifdef GFSDK_WAVEWORKS_GL
#define nv_waveworks_impl_0_12 gl_FragColor
void main()
#else
void ps(
	float2 nv_waveworks_impl_0_8 SEMANTIC(TEXCOORD0),
	out float4 nv_waveworks_impl_0_12 SEMANTIC(SV_Target)
)
#endif
{
	float3 nv_waveworks_impl_0_13	= SampleTex2D(nv_waveworks_impl_0_6, nv_waveworks_impl_0_7, nv_waveworks_impl_0_8.xy + nv_waveworks_impl_0_2.xy).rgb;
	float3 nv_waveworks_impl_0_14	= SampleTex2D(nv_waveworks_impl_0_6, nv_waveworks_impl_0_7, nv_waveworks_impl_0_8.xy + nv_waveworks_impl_0_3.xy).rgb;
	float3 nv_waveworks_impl_0_15	= SampleTex2D(nv_waveworks_impl_0_6, nv_waveworks_impl_0_7, nv_waveworks_impl_0_8.xy + nv_waveworks_impl_0_4.xy).rgb;
	float3 nv_waveworks_impl_0_16	= SampleTex2D(nv_waveworks_impl_0_6, nv_waveworks_impl_0_7, nv_waveworks_impl_0_8.xy + nv_waveworks_impl_0_5.xy).rgb;
	float2 nv_waveworks_impl_0_17 = float2(-(nv_waveworks_impl_0_14.z - nv_waveworks_impl_0_13.z) / max(0.01,1.0 + nv_waveworks_impl_0_1.y*(nv_waveworks_impl_0_14.x - nv_waveworks_impl_0_13.x)), -(nv_waveworks_impl_0_16.z - nv_waveworks_impl_0_15.z) / max(0.01,1.0+nv_waveworks_impl_0_1.y*(nv_waveworks_impl_0_16.y - nv_waveworks_impl_0_15.y)));
	float2 nv_waveworks_impl_0_18 = (nv_waveworks_impl_0_14.xy - nv_waveworks_impl_0_13.xy) * nv_waveworks_impl_0_1.x;
	float2 nv_waveworks_impl_0_19 = (nv_waveworks_impl_0_16.xy - nv_waveworks_impl_0_15.xy) * nv_waveworks_impl_0_1.x;
	float nv_waveworks_impl_0_20 = (1.0f + nv_waveworks_impl_0_18.x) * (1.0f + nv_waveworks_impl_0_19.y) - nv_waveworks_impl_0_18.y * nv_waveworks_impl_0_19.x;
	nv_waveworks_impl_0_12 = float4(nv_waveworks_impl_0_17, nv_waveworks_impl_0_20, 0);
}
#endif 
