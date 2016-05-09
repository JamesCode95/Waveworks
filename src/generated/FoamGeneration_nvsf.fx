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
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_1,0); 
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_2  ,1); 
DECLARE_ATTR_CONSTANT(float4,nv_waveworks_impl_0_3         ,2); 
END_CBUFFER
DECLARE_ATTR_SAMPLER(nv_waveworks_impl_0_4,nv_waveworks_impl_0_5,0);
#ifdef GFSDK_WAVEWORKS_GL
varying float2 nv_waveworks_impl_0_6;
#endif
#ifndef GFSDK_WAVEWORKS_OMIT_VS
#ifdef GFSDK_WAVEWORKS_GL
attribute float4 nv_waveworks_impl_0_7;
attribute float2 nv_waveworks_impl_0_8;
#define nv_waveworks_impl_0_9 gl_Position
void main()
#else
void vs(
	float4 nv_waveworks_impl_0_7 SEMANTIC(POSITION),
	float2 nv_waveworks_impl_0_8 SEMANTIC(TEXCOORD0),
	out float2 nv_waveworks_impl_0_6 SEMANTIC(TEXCOORD0),
	out float4 nv_waveworks_impl_0_9 SEMANTIC(SV_Position)
)
#endif
{
    nv_waveworks_impl_0_9 = nv_waveworks_impl_0_7;
    nv_waveworks_impl_0_6 = nv_waveworks_impl_0_8;
}
#endif 
#ifndef GFSDK_WAVEWORKS_OMIT_PS
#ifdef GFSDK_WAVEWORKS_GL
#define nv_waveworks_impl_0_10 gl_FragColor
void main()
#else
void ps(
	float2 nv_waveworks_impl_0_6 SEMANTIC(TEXCOORD0),
	out float4 nv_waveworks_impl_0_10 SEMANTIC(SV_Target)
)
#endif
{
	float2 nv_waveworks_impl_0_11 = nv_waveworks_impl_0_3.xy*nv_waveworks_impl_0_1.x;
	float nv_waveworks_impl_0_12	= dot(nv_waveworks_impl_0_2, SampleTex2D(nv_waveworks_impl_0_4, nv_waveworks_impl_0_5, nv_waveworks_impl_0_6.xy + nv_waveworks_impl_0_11));
	float nv_waveworks_impl_0_13	= dot(nv_waveworks_impl_0_2, SampleTex2D(nv_waveworks_impl_0_4, nv_waveworks_impl_0_5, nv_waveworks_impl_0_6.xy - nv_waveworks_impl_0_11));
	float nv_waveworks_impl_0_14	= dot(nv_waveworks_impl_0_2, SampleTex2D(nv_waveworks_impl_0_4, nv_waveworks_impl_0_5, nv_waveworks_impl_0_6.xy + nv_waveworks_impl_0_11*2.0));
	float nv_waveworks_impl_0_15	= dot(nv_waveworks_impl_0_2, SampleTex2D(nv_waveworks_impl_0_4, nv_waveworks_impl_0_5, nv_waveworks_impl_0_6.xy - nv_waveworks_impl_0_11*2.0));
	float nv_waveworks_impl_0_16 = max(0,SampleTex2D(nv_waveworks_impl_0_4, nv_waveworks_impl_0_5, nv_waveworks_impl_0_6.xy).z);
	float nv_waveworks_impl_0_17 = nv_waveworks_impl_0_1.y*((nv_waveworks_impl_0_12 + nv_waveworks_impl_0_13 + nv_waveworks_impl_0_14 + nv_waveworks_impl_0_15)*0.25 + max(0,(1.0-nv_waveworks_impl_0_16-nv_waveworks_impl_0_1.w))*nv_waveworks_impl_0_1.z);
	nv_waveworks_impl_0_17 = min(1.0,nv_waveworks_impl_0_17);
	nv_waveworks_impl_0_10 = float4(nv_waveworks_impl_0_17,nv_waveworks_impl_0_17,nv_waveworks_impl_0_17,nv_waveworks_impl_0_17);
}
#endif 