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

#ifndef _NVWAVEWORKS_SIMULATION_IMPL_H
#define _NVWAVEWORKS_SIMULATION_IMPL_H

#if WAVEWORKS_ENABLE_GNM
namespace sce
{
	namespace Gnmx
	{
		class VsShader;
		class PsShader;
		class CsShader;

		namespace Toolkit
		{
			class IAllocator;
		}

	}
}
namespace GFSDK_WaveWorks_GNM_Util
{
	class RenderTargetClearer;
}
#include <gnm\buffer.h>
#include <gnm\sampler.h>
#include <gnm\regs.h>
#include <gnm\texture.h>
#include <gnm\rendertarget.h>
#endif

class NVWaveWorks_Mesh;
class NVWaveWorks_FFT_Simulation;
class NVWaveWorks_FFT_Simulation_Manager;
class NVWaveWorks_GFX_Timer_Impl;
struct GFSDK_WaveWorks_CPU_Scheduler_Interface;
struct ID3D11DeviceContext;

struct GFSDK_WaveWorks_Simulation
{
public:
    GFSDK_WaveWorks_Simulation();
    ~GFSDK_WaveWorks_Simulation();

    HRESULT initD3D11(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler, ID3D11Device* pD3DDevice);
	HRESULT initGnm(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, GFSDK_WaveWorks_CPU_Scheduler_Interface* pOptionalScheduler);
	HRESULT initGL2(const GFSDK_WaveWorks_Detailed_Simulation_Params& params, void* pGLContext);
	HRESULT initNoGraphics(const GFSDK_WaveWorks_Detailed_Simulation_Params& params);
    HRESULT reinit(const GFSDK_WaveWorks_Detailed_Simulation_Params& params);

    void setSimulationTime(double dAppTime);
	float getConservativeMaxDisplacementEstimate();
	void updateRMS(const GFSDK_WaveWorks_Detailed_Simulation_Params& params);

    HRESULT kick(gfsdk_U64* pKickID, Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);
	HRESULT getStats(GFSDK_WaveWorks_Simulation_Stats& stats);

	bool getStagingCursor(gfsdk_U64* pKickID);
	HRESULT advanceStagingCursor(Graphics_Context* pGC, bool block, bool& wouldBlock, GFSDK_WaveWorks_Savestate* pSavestateImpl);
	HRESULT waitStagingCursor();
	bool getReadbackCursor(gfsdk_U64* pKickID);
	HRESULT advanceReadbackCursor(bool block, bool& wouldBlock);

	HRESULT archiveDisplacements();

    HRESULT setRenderState(	Graphics_Context* pGC,
							const gfsdk_float4x4& matView,
							const UINT* pShaderInputRegisterMappings ,
							GFSDK_WaveWorks_Savestate* pSavestateImpl,
							const GFSDK_WaveWorks_Simulation_GL_Pool* pGlPool
							);

    HRESULT getDisplacements(	const gfsdk_float2* inSamplePoints,
                                gfsdk_float4* outDisplacements,
                                UINT numSamples
                                );

	HRESULT getArchivedDisplacements(	float coord,
										const gfsdk_float2* inSamplePoints,
										gfsdk_float4* outDisplacements,
										UINT numSamples
										);

    static HRESULT getShaderInputCountD3D11();
    static HRESULT getShaderInputDescD3D11(UINT inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);
    static HRESULT getShaderInputCountGnm();
    static HRESULT getShaderInputDescGnm(UINT inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);
    static HRESULT getShaderInputCountGL2();
	static HRESULT getTextureUnitCountGL2(gfsdk_bool useTextureArrays);
    static HRESULT getShaderInputDescGL2(UINT inputIndex, GFSDK_WaveWorks_ShaderInput_Desc* pDesc);

private:

    GFSDK_WaveWorks_Detailed_Simulation_Params m_params;

    HRESULT updateGradientMaps(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);
	HRESULT updateGradientMapsD3D11(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);
	HRESULT updateGradientMapsGnm(Graphics_Context* pGC, GFSDK_WaveWorks_Savestate* pSavestateImpl);
	HRESULT updateGradientMapsGL2(Graphics_Context* pGC);

    HRESULT setRenderStateD3D11(	ID3D11DeviceContext* pDC,
									const gfsdk_float4x4& matView,
									const UINT* pShaderInputRegisterMappings,
									GFSDK_WaveWorks_Savestate* pSavestateImpl
									);
	HRESULT setRenderStateGnm(		sce::Gnmx::LightweightGfxContext* gfxContext,
									const gfsdk_float4x4& matView,
									const UINT* pShaderInputRegisterMappings,
									GFSDK_WaveWorks_Savestate* pSavestateImpl
									);
    HRESULT setRenderStateGL2(		const gfsdk_float4x4& matView,
									const UINT* pShaderInputRegisterMappings,
									const GFSDK_WaveWorks_Simulation_GL_Pool& glPool
									);


    // ---------------------------------- GPU simulation data --------------------------------

    struct CascadeState
    {
		NVWaveWorks_Mesh* m_pQuadMesh;
        NVWaveWorks_FFT_Simulation* m_pFFTSimulation;

        // The kickID that originated the last update to this displacement map, allowing us to track when
		// the map is out of date and needs another update...
        gfsdk_U64 m_gradient_map_version;

		// Set when the gradient map is newly created and therefore in need of an intitial clear
		bool m_gradient_map_needs_clear[MaxNumGPUs];

#if WAVEWORKS_ENABLE_D3D11
        struct D3D11Objects
        {
            ID3D11ShaderResourceView* m_pd3d11GradientMap[MaxNumGPUs];			// (ABGR16F) - round-robin, to avoid SLI-inteframe dependencies
            ID3D11RenderTargetView*   m_pd3d11GradientRenderTarget[MaxNumGPUs];	// (ditto)
            ID3D11ShaderResourceView* m_pd3d11FoamEnergyMap;		// (R16F)
            ID3D11RenderTargetView*   m_pd3d11FoamEnergyRenderTarget;// (ditto)
        };
#endif

#if WAVEWORKS_ENABLE_GNM
		struct GnmObjects
		{
			sce::Gnm::Texture         m_gnmGradientMap[MaxNumGPUs];			// (ABGR16F) - round-robin, to avoid SLI-inteframe dependencies
			sce::Gnm::RenderTarget    m_gnmGradientRenderTarget[MaxNumGPUs];	// (ditto)
			sce::Gnm::Texture         m_gnmFoamEnergyMap;		// (R16F)
			sce::Gnm::RenderTarget    m_gnmFoamEnergyRenderTarget;// (ditto)
		};
#endif
#if WAVEWORKS_ENABLE_GL
        struct GL2Objects
        {
            GLuint m_GL2GradientMap[MaxNumGPUs];			// (ABGR16F) - round-robin, to avoid SLI-inteframe dependencies
            GLuint m_GL2GradientFBO[MaxNumGPUs];			// (ditto)
            GLuint m_GL2FoamEnergyMap;					// (R16F)
            GLuint m_GL2FoamEnergyFBO;					// (ditto)
        };
#endif
		union
        {
#if WAVEWORKS_ENABLE_D3D11
			D3D11Objects _11;
#endif
#if WAVEWORKS_ENABLE_GNM
			GnmObjects _gnm;
#endif
#if WAVEWORKS_ENABLE_GL
			GL2Objects _GL2;
#endif
		} m_d3d;

    };

    CascadeState cascade_states[GFSDK_WaveWorks_Detailed_Simulation_Params::MaxNumCascades];

	// To preserve SLI scaling, we operate some resources that have inter-frame dependencies on a round-robin basis...
    int m_num_GPU_slots;		// the number of GPU slots allocated for per-GPU resources (e.g. gradient maps)
	int m_active_GPU_slot;		// the index of the active GPU within m_num_GPU_slots
	void consumeGPUSlot();

	float m_total_rms;

    double m_dSimTime;
	double m_dSimTimeFIFO[MaxNumGPUs+1];
	int m_numValidEntriesInSimTimeFIFO;
	double m_dFoamSimDeltaTime;

	// Some kinds of simulation require a manager to hook simulation-level events
	NVWaveWorks_FFT_Simulation_Manager* m_pSimulationManager;

	// Scheduler to use for CPU work (optional)
	GFSDK_WaveWorks_CPU_Scheduler_Interface* m_pOptionalScheduler;

	// GFX timing services
	NVWaveWorks_GFX_Timer_Impl* m_pGFXTimer;

    // ---------------------------------- Rendering ------------------------------------

    HRESULT initShaders();
    HRESULT initGradMapSamplers();
	HRESULT initTextureArrays();
	HRESULT initQuadMesh(int mode);

    // D3D API handling
    nv_water_d3d_api m_d3dAPI;

#if WAVEWORKS_ENABLE_D3D11
    struct D3D11Objects
    {
        ID3D11Device* m_pd3d11Device;

        // Shaders for grad calc
        ID3D11VertexShader* m_pd3d11GradCalcVS;
        ID3D11PixelShader* m_pd3d11GradCalcPS;
        ID3D11Buffer* m_pd3d11GradCalcPixelShaderCB;
        ID3D11SamplerState* m_pd3d11PointSampler;
        ID3D11DepthStencilState* m_pd3d11NoDepthStencil;
        ID3D11RasterizerState* m_pd3d11AlwaysSolidRasterizer;
        ID3D11BlendState* m_pd3d11CalcGradBlendState;
		ID3D11BlendState* m_pd3d11AccumulateFoamBlendState;
		ID3D11BlendState* m_pd3d11WriteAccumulatedFoamBlendState;

        // State for main rendering
        ID3D11SamplerState* m_pd3d11LinearNoMipSampler;
        ID3D11SamplerState* m_pd3d11GradMapSampler;
        ID3D11Buffer*		m_pd3d11PixelShaderCB;
        ID3D11Buffer*		m_pd3d11VertexDomainShaderCB;

        // Shaders for foam generation
        ID3D11VertexShader* m_pd3d11FoamGenVS;
        ID3D11PixelShader* m_pd3d11FoamGenPS;
        ID3D11Buffer* m_pd3d11FoamGenPixelShaderCB;
    };
#endif

#if WAVEWORKS_ENABLE_GNM
	struct GnmObjects
	{
		// Shaders for grad calc
		sce::Gnmx::VsShader* m_pGnmGradCalcVS;
		sce::Gnmx::InputResourceOffsets* m_pGnmGradCalcVSResourceOffsets;
		void* m_pGnmGradCalcFS;
		sce::Gnmx::PsShader* m_pGnmGradCalcPS;
		sce::Gnmx::InputResourceOffsets* m_pGnmGradCalcPSResourceOffsets;
		sce::Gnm::Sampler m_pGnmPointSampler;
		sce::Gnm::DepthStencilControl m_pGnmNoDepthStencil;
		sce::Gnm::PrimitiveSetup m_pGnmAlwaysSolidRasterizer;
		sce::Gnm::BlendControl m_pGnmCalcGradBlendState;
		sce::Gnm::BlendControl m_pGnmAccumulateFoamBlendState;
		sce::Gnm::BlendControl m_pGnmWriteAccumulatedFoamBlendState;

		// State for main rendering
		sce::Gnm::Sampler m_pGnmLinearNoMipSampler;
		sce::Gnm::Sampler m_pGnmGradMapSampler;
		sce::Gnm::Buffer m_pGnmPixelShaderCB;
		sce::Gnm::Buffer m_pGnmVertexDomainShaderCB;

		// Shaders for foam generation
		sce::Gnmx::VsShader* m_pGnmFoamGenVS;
		sce::Gnmx::InputResourceOffsets* m_pGnmFoamGenVSResourceOffsets;
		void* m_pGnmFoamGenFS;
		sce::Gnmx::PsShader* m_pGnmFoamGenPS;
		sce::Gnmx::InputResourceOffsets* m_pGnmFoamGenPSResourceOffsets;

		sce::Gnmx::CsShader* m_pGnmMipMapGenCS;
		sce::Gnmx::InputResourceOffsets* m_pGnmMipMapGenCSResourceOffsets;
		GFSDK_WaveWorks_GNM_Util::RenderTargetClearer* m_pGnmRenderTargetClearer;
	};
#endif
#if WAVEWORKS_ENABLE_GL
    struct GL2Objects
    {
        void* m_pGLContext;

        // Shaders for grad calc
		GLuint m_GradCalcProgram;
		// Uniform binding points for grad calc shader
		GLuint m_GradCalcUniformLocation_Scales;
		GLuint m_GradCalcUniformLocation_OneLeft;
		GLuint m_GradCalcUniformLocation_OneRight;
		GLuint m_GradCalcUniformLocation_OneBack;
		GLuint m_GradCalcUniformLocation_OneFront;
		GLuint m_GradCalcTextureBindLocation_DisplacementMap;
		GLuint m_GradCalcTextureUnit_DisplacementMap;
		// Vertex attribute locations
		GLuint m_GradCalcAttributeLocation_Pos;
		GLuint m_GradCalcAttributeLocation_TexCoord;

        // Shaders for foam generation
		GLuint m_FoamGenProgram;
		// Uniform binding points for foam generation shader
		GLuint m_FoamGenUniformLocation_DissipationFactors;
		GLuint m_FoamGenUniformLocation_SourceComponents;
		GLuint m_FoamGenUniformLocation_UVOffsets;
		GLuint m_FoamGenTextureBindLocation_EnergyMap;
		GLuint m_FoamGenTextureUnit_EnergyMap;
		// Vertex attribute locations
		GLuint m_FoamGenAttributeLocation_Pos;
		GLuint m_FoamGenAttributeLocation_TexCoord;

		// Texture arrays & FBO needed to blit to those
		GLuint m_DisplacementsTextureArray;
		GLuint m_GradientsTextureArray;
		GLuint m_TextureArraysBlittingReadFBO;
		GLuint m_TextureArraysBlittingDrawFBO;
    };
#endif
	union
    {
#if WAVEWORKS_ENABLE_D3D11
		D3D11Objects _11;
#endif
#if WAVEWORKS_ENABLE_GNM
		GnmObjects _gnm;
#endif
#if WAVEWORKS_ENABLE_GL
		GL2Objects _GL2;
#endif
    } m_d3d;

    HRESULT allocateAll();
    void releaseAll();

    void releaseRenderingResources(int mode);
    HRESULT allocateRenderingResources(int mode);

    void releaseSimulation(int mode);
    HRESULT allocateSimulation(int mode);

	void releaseSimulationManager();
	HRESULT allocateSimulationManager();

	void releaseGFXTimer();
	HRESULT allocateGFXTimer();

	// Timer query ring-buffer
	struct TimerSlot
	{
		int m_DisjointQueryIndex;
		int m_StartQueryIndex;
		int m_StopQueryIndex;
		int m_StartGFXQueryIndex;
		int m_StopGFXQueryIndex;
		float m_elapsed_time;			// in milli-seconds, as per house style
		float m_elapsed_gfx_time;		// in milli-seconds, as per house style
	};

	struct TimerPool
	{
		enum { NumTimerSlots = 4 };			// 2 in-flight, one usable, one active
		int m_active_timer_slot;			// i.e. not in-flight
		int m_end_inflight_timer_slots;		// the first in-flight slot is always the one after active
		TimerSlot m_timer_slots[NumTimerSlots];

		void reset();
	};

	TimerPool m_gpu_kick_timers;
	TimerPool m_gpu_wait_timers;

	bool m_has_consumed_wait_timer_slot_since_last_kick;

	HRESULT consumeAvailableTimerSlot(Graphics_Context* pGC, NVWaveWorks_GFX_Timer_Impl* pGFXTimer, TimerPool& pool, TimerSlot** ppSlot);
	HRESULT queryTimers(Graphics_Context* pGC, NVWaveWorks_GFX_Timer_Impl* pGFXTimer, TimerPool& pool);
	HRESULT queryAllGfxTimers(Graphics_Context* pGC, NVWaveWorks_GFX_Timer_Impl* pGFXTimer);

	GLuint compileGLShader(const char *text, GLenum type);
	GLuint loadGLProgram(const char* vstext, const char* tetext, const char* tctext,  const char* gstext, const char* fstext);
};

#endif	// _NVWAVEWORKS_SIMULATION_IMPL_H
