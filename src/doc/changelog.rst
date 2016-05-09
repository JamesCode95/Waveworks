Change Log
=======================================

|PRODUCTNAMEDOCRELEASEBOLD|

WaveWorks is a library for simulating terrain water surfaces, such as lakes and oceans, using the GPU.
The library includes shader fragments which can be used to reference the results of the simulation.
The library also supports a no-graphics path which can be used for pure-simulation applications.

**1.6**

- tracking simulation updates along pipeline with kick ID's
- support for multiple pipeline synchronization strategies
- readback FIFO (can be used to calculate velocities and/or accelerations)
- MacOS port
- support for synchronous in-thread simulation on CPU path
- texture array option for GL
- control over enabling of CUDA timers
- explicit allocation of GL texture units

**1.5**

- PS4 port
- XboxOne port
- Win/GL port
- D3D11 sample app now ships with VS2010 sln/vcxproj, instead of VS2008
- all API entrypoints which used to take a void* or IUnknown* have been specialized for reasons of type-safety and clarity (e.g. GFSDK_WaveWorks_Simulation_UpdateTick -> GFSDK_WaveWorks_Simulation_UpdateTickD3D9, GFSDK_WaveWorks_Simulation_UpdateTickD3D10 etc...)
- simplified index-based register hookup for constant buffers, by removing the offset param from macros (implication is that each shader-level module - Quadtree, Attributes - requires a maximum of one constant buffer slot per shader stage)
- example ray-casting implementation added to D3D11 sample app

**1.4**

- Added the 'no graphics' path, for clients who need only readback results (e.g. MMO servers).
- Ported to Linux, with initial support for CPU vs no-graphics and GPU vs no-graphics. 
- Rename ``GFSDK_WaveWorks_Simulation_CanSetRenderState()`` to ``GFSDK_WaveWorks_Simulation_HasResults()``. The reason for this is that there is no render-state when working in no-graphics mode, but clients will still need to be able to test whether the simulation pipeline is primed. 

**1.3**

- Rename to WaveWorks.
- GameWorks standardization.
- Foam.
- Beaufort presets.
- CUDA acceleration for evolving spectra.
- CUDA now only available with D3D9Ex when running with D3D9.
- Numerous checks added for API usage consistency.
- Added memory allocation callback hooks.
- Misc fixes and improvements to CPU simulation path.
- Added dependee DLLs (e.g. cudart, cufft) to distro, for convenience.
- Simulation update now accepts and honours double-precision time (fixes rounding errors on long-duration runs).

**1.2**

- Implemented CPU fallback path for simulation.
- CPU path uses SSE and is parallelized across a user-configurable number of threads.
- Added entrypoint to test whether simulation pipeline is full (primarily for CPU path).
- Bucketed assignment of frequencies to cascade levels.
- Added geomorphing to D3D9/D3D10 to smooth transitions between quad-tree mesh LODs.
- Made readbacks viewpoint-independent.
- Improved performance of CUDA readbacks.
- Added counters to help with perf triage.
- SI units throughout.
- CUDA Toolkit 4.2 required.

Redistribution considerations:

- Requires ``CUDARTXX_42_9.dll`` and ``CUFFTXX_42_9.dll`` from the CUDA 4.2 Toolkit. 
- Requires ``D3DX9_43.DLL`` from the June 2010 DXSDK. 

**1.1**

- Implemented CPU fallback path for simulation.
- CPU path uses SSE and is parallelized across a user-configurable number of threads.
- Added entrypoint to test whether simulation pipeline is full (primarily for CPU path).
- Bucketed assignment of frequencies to cascade levels.
- Added geomorphing to D3D9/D3D10 to smooth transitions between quad-tree mesh LODs.
- Made readbacks viewpoint-independent.
- Improved performance of CUDA readbacks.
- Added counters to help with perf triage.
- SI units throughout.
- CUDA Toolkit 4.2 required.

Redistribution considerations:

- Requires ``CUDARTXX_42_9.dll`` and ``CUFFTXX_42_9.dll`` from the CUDA 4.2 Toolkit. 
- Requires ``D3DX9_43.DLL`` from the June 2010 DXSDK. 

**1.0**

- Added DX11 support.
- DX11 quad-tree rendering uses tessellation to generate triangles.
- Added x64 support.
- Added DX11 demo.
- CUDA Toolkit 3.2 required.

Redistribution considerations:

- Requires ``cudart*.dll`` and ``cufft*.dll`` from the CUDA Toolkit. 
