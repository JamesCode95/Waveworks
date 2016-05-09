.. Replace existing content with product specific content. Layout for this page should be consistent for all products. 

|PRODUCTNAME| |VERSION|
======================================

.. Replace the content. Layout should not change

Overview
##############

NVIDIA WaveWorks enables developers to deliver a cinematic-quality ocean simulation for interactive applications. The simulation runs in the frequency domain, using a spectral wave dispersion model. An inverse FFT step then transforms to the spatial domain, ready for rendering. The NVIDIA WaveWorks simulation is initialized and controlled by a simple C API, and the results are accessed for rendering through a HLSL shader API. Parameterization is done via intuitive real-world variables, such as wind speed and direction. These parameters can be used to tune the look of the sea surface for a wide variety of conditions – from gentle ripples to a heavy storm-tossed ocean based on the Beaufort scale. 

In addition, we also provide an energy-based surface foam simulation, which is locked to and driven by the underlying spectral simulation. The foam simulation results are also exposed through HLSL shader API, and permit full customization of the foam look, according to physical properties like surface energy and mixed-in turbulent energy. Version 1.3 also adds optional support for greatly simplified parameterization choices based on the Beaufort scale.

Version 1.4 adds support for running in ‘no-graphics’ mode, where the application consumes simulation results via displacement queries only. This mode is aimed initially at the MMO server use-case, and is currently supported for Windows and Linux for simulations running on both CPU and GPU. Importantly, the simulation will always produce the same result for a given time value, which means it can be synchronized between the multiple nodes of a networked application.

Features

* Controlled via a simple C API 
* Simulation results accessed via HLSL API. Lighting/shading remains under full application control 
* Flexible save/restore for D3D state across C API calls   
* Quad-tree tile-based LODing Host readback (e.g. for simulation of water-borne objects) 
* DX11 tessellation; geo-morphing for DX9/10 
* Foam simulation Beaufort presets 
* GPU acceleration for evolving spectra 
* A "no graphics" path, for clients who need only readback results (e.g. MMO servers)
* Linux port available 
* Next-gen console ports available
* Win/GL port available
* Mac/GL port available

.. image:: \_static\WaveWorks_ship.png


Getting Started
##############

The sample app is a good place to start if you want to see how to integrate the library. This app is located in the 'sample' directory. The library must be globally initialized using ``GFSDK_WaveWorks_InitXXXX()`` before attempting to create objects and run simulations. However not all entrypoints are subject to this rule - the following entrypoints *can* safely be called without first initialising the library (because they are get-only informational functions):

* ``GFSDK_WaveWorks_GetBuildString()``
* ``GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()``
* ``GFSDK_WaveWorks_Simulation_GetShaderInputCountXXXX()``
* ``GFSDK_WaveWorks_Simulation_GetShaderInputDescXXXX()``
* ``GFSDK_WaveWorks_Quadtree_GetShaderInputCountXXXX()``
* ``GFSDK_WaveWorks_Quadtree_GetShaderInputDescXXXX()``
* ``GFSDK_WaveWorks_GLAttribIsShaderInput()``

Note that throughout this documentation, 'XXXX' is used to represent the graphics API permutations offered by the library (so D3D9/D3D10/D3D11/NoGraphics/Gnm/GL2, etc.)

Core simulation
---------------
An understanding of the underlying FFT-based technique is helpful when setting up a simulation (see: `http://graphics.ucsd.edu/courses/rendering/2005/jdewall/tessendorf.pdf <http://graphics.ucsd.edu/courses/rendering/2005/jdewall/tessendorf.pdf>`_ ).The library actually runs a cascade of such FFT simulations, with each successive cascade member covering a greater footprint in world space than the previous member. This cascade of simulations is used to achieve smooth LODing without repetition artifacts when rendering the water surface. The cascaded nature of the simulation should be invisible to clients of the library - the library operates as a 'black box' in this repsect, allowing only for the overall properties of the simulation to be specified, and for the displacements/gradients to be accessed during rendering.The client app initializes a simulation by filling out a ``GFSDK_WaveWorks_Simulation_Settings`` and a ``GFSDK_WaveWorks_Simulation_Params``, and passing them to ``GFSDK_WaveWorks_Simulation_CreateXXXX()``. The properties for a simulation can also be updated later, by calling ``GFSDK_WaveWorks_Simulation_UpdateProperties()``, but note that this can cause simulation resources to be reallocated (depending on which properties are changed; e.g., detail level). A simulation has the following parameters:

*  ``wave_amplitude`` - global scale factor for simulated wave amplitude.
*  ``wind_dir`` - the direction of the wind inducing the waves.
*  ``wind_speed`` - the speed of the wind inducing the waves. If GFSDK_WaveWorks_Simulation_Settings.UseBeaufortScale is set, this is interpreted as a Beaufort scale value. Otherwise, it is interpreted as metres per second
*  ``wind_dependency`` - the degree to which waves appear to move in the wind direction (vs. standing waves), in the [0,1] range
*  ``choppy_scale`` - in addition to height displacements, the simulation also applies lateral displacements. This controls the non-linearity and therefore 'choppiness' in the resulting wave shapes. Should normally be set in the [0,1] range.
*  ``small_wave_fraction`` - the simulation spectrum is low-pass filtered to eliminate wavelengths that could end up under-sampled, this controls how much of the frequency range is considered 'high frequency' (i.e. small wave).
*  ``time_scale`` - the global time multiplier.
*  ``foam_generation_threshold`` - the turbulent energy representing foam and bubbles spread in water starts generating on the tips of the waves if Jacobian of wave curvature gets higher than this threshold. The range is [0,1], the typical values are [0.2,0.4] range.
*  ``foam_generation_amount`` - the amount of turbulent energy injected in areas defined by foam_generation_threshold parameter on each simulation step. The range is [0,1], the typical values are [0,0.1] range.
*  ``foam_dissipation_speed`` - the speed of spatial dissipation of turbulent energy. The range is [0,1], the typical values are in [0.5,1] range.
*  ``foam_falloff_speed`` - in addition to spatial dissipation, the turbulent energy dissolves over time. This parameter sets the speed of dissolving over time. The range is [0,1], the typical values are in [0.9,0.99] range.

A simulation has the following settings:

*  ``detail_level`` - the detail level of the simulation: this drives the resolution of the FFT, and also determines whether the simulation workload is done on the GPU or CPU (but see note below on hardware support).
*  ``fft_period`` - the repeat interval of the simulation, in metres. The simulation should generate unique results within any fft_period x fft_period area.
*  ``use_Beaufort_scale`` - how to interpret ``GFSDK_WaveWorks_Simulation_Params::wind_speed``. If true, interpret as a Beaufort scale quantity, and use dependent presets for all other params.
*  ``readback_displacements`` - true if ``GFSDK_WaveWorks_Simulation_GetDisplacements()`` should apply the simulated displacements.
*  ``num_readback_FIFO_entries`` - if readback is enabled, displacement data can be kept alive in a FIFO for historical lookups. e.g. in order to implement predict/correct for a networked application
*  ``aniso_level`` - this should be set to desired anisotropic filtering degree for sampling of gradient maps. This value is clamped to [1,16] range internally, and it should be be clamped further to the range supported by the GPU.
*  ``CPU_simulation_threading_model`` - the threading model to use when the CPU simulation path is active. Can be set to none (meaning: simulation is performed on the calling thread, synchronously), automatic, or even an explicitly specified thread count
*  ``num_GPUs`` - this should be set to the number of SLI AFR groups detected by the app via NVAPI (e.g. set to 1 for the single GPU case).
*  ``use_texture_arrays`` - true if texture arrays should be used in GL (requires fewer texture units)
*  ``enable_CUDA_timers`` - controls whether timer events will be used to gather stats on the CUDA simulation path. This can impact negatively on GPU/CPU parallelism, so it is recommended to enable this only when necessary

A note on hardware support: not all hardware is capable of supporting all possible settings for ``detail_level`` - settings that are not supported on the current hardware will cause ``GFSDK_WaveWorks_Simulation_CreateXXXX()`` to fail. This can be tested in advance of creation using ``GFSDK_WaveWorks_Simulation_DetailLevelIsSupported_XXXX()``. Once a simulation has been initialized, the application should:

* call ``GFSDK_WaveWorks_Simulation_SetTime()`` once per simulation step to set the current time.
* call ``GFSDK_WaveWorks_Simulation_KickXXXX()`` once per simulation step to pump the simulation pipeline. 
* call ``GFSDK_WaveWorks_Simulation_SetRenderStateXXXX()`` once per frame to bind the simulation outputs as shader inputs ready for rendering. 

If the application uses a dedicated thread for rendering, these per-frame calls should be made on that thread.

Note that the simulation is pipelined, therefore it may be necessary to 'prime' the pipeline on the first frame after intialization by pushing multiple simulation steps via ``GFSDK_WaveWorks_Simulation_KickXXXX()``. ``GFSDK_WaveWorks_Simulation_GetStagingCursor()`` will return 'gfsdk_waveworks_result_OK' when enough steps have been pushed to prime the pipeline.

Geometry generators
---------------

The lib is designed to support different methods for generating geometry. A geometry-generator is expected to minimally take care of frustum culling, distance LODing, labeling of non-water regions (i.e., inland areas) and initiation of drawing. The client app can either select a 'stock' geometry-generator from the lib (including any corresponding shader fragments), or it can implement its own geometry generator. A geometry-generator interfaces with the lib at the shader-fragment level. Specifically:

* it should define a vertex input struct, ``GFSDK_WAVEWORKS_VERTEX_INPUT``.
* it should define ``GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition()``.
* it should issue draw calls as necessary, with a stream of ``GFSDK_WAVEWORKS_VERTEX_INPUT``'s bound to the vertex shader.

**Quad-tree generator**

This geometry-generator uses a hierarchical quad-tree of square patches. All patches have the same number of triangles (apart from edge fixups), so the quad-tree generator uses smaller patches nearer the camera, in order to achieve greater overall mesh density where it is most needed. The D3D11 path uses hardware tessellation to smoothly vary the triangle rate of mesh, the D3D9 and D3D10 paths use geomorphing. The client app initializes a quad-tree generator by filling out a ``GFSDK_WaveWorks_Quadtree_Params`` and passing it to ``GFSDK_WaveWorks_Quadtree_CreateXXXX()``. The parameters can also be updated later on by calling ``GFSDK_WaveWorks_Quadtree_UpdateParams()``, although again, this is best avoided for performance reasons. A quad-tree generator has the following parameters:

*  ``mesh_dim``  - the number of triangles along the side of a single patch.
*  ``min_patch_length``  - the size of the smallest permissible leaf patch, in world space.
*  ``patch_origin``  - the coordinates of the min corner of patch (0,0) at some LOD (used only with ``AllocPatch/FreePatch``).
*  ``auto_root_lod``  - the LOD of the root patch (only when ``AllocPatch/FreePatch`` are *not* used).
*  ``upper_grid_coverage`` - the maximum number of pixels a patch can cover (used to choose patch LODs).
*  ``sea_level`` - the vertical offset required to place the surface at sea level.
*  ``use_tessellation`` - whether to use tessellation for DX11/GL4.
*  ``tessellation_lod`` - for DX11, the adaptive tessellation density.
*  ``geomorphing_degree`` - for DX9/10, the degree of geomorphing to apply, in the [0,1] range. High levels of geomorphing require greater triangle density in the underlying mesh.

The quad-tree generator can be used in two modes: automatic and explicit.

In automatic mode, ``GFSDK_WaveWorks_Quadtree_AllocPatch`` and ``GFSDK_WaveWorks_Quadtree_FreePatch`` are never called. Therefore, the water surface is assumed to be infinite in extent, and traversal of the quad-tree begins at the LOD level specified by *auto_root_lod* .

In explicit mode, the client app makes calls to ``GFSDK_WaveWorks_Quadtree_AllocPatch`` and ``GFSDK_WaveWorks_Quadtree_FreePatch`` to load/unload patches and marks them as present or not-present (using the 'enabled' parameter). Traversal of the quad-tree begins at the highest-LOD allocated patches.

To traverse the quad-tree and draw its visible patches, call ``GFSDK_WaveWorks_Quadtree_DrawXXXX()``.

The quad-tree generator performs frustum culling against undisplaced tile bounds which can lead to artifacts when simulation displacements are added during shading. For this reason, a quad-tree culling margin can be specified using ``GFSDK_WaveWorks_Quadtree_SetFrustumCullMargin()``. An appropriate culling margin value can be obtained from a simulation using ``GFSDK_WaveWorks_Simulation_GetConservativeMaxDisplacementEstimate()``, but clients should add a further margin for any client-generated displacements applied during shading (e.g. boat wakes, explosion craters).

Shader integration
---------------

For Direct3D 9 and Direct3D 10 apps, the shader-level integration works as follows:

1. The application calls ``GFSDK_WaveWorks_GetDisplacedVertex()`` in its vertex shader. This returns a ``GFSDK_WAVEWORKS_VERTEX_OUTPUT``, which contains the world position and displacement generated by the simulation for the displaced vertex, and also an 'interp' member, which the app should pass to its pixel shader.
2. The application calls ``GFSDK_WaveWorks_GetSurfaceAttributes()`` in its pixel shader, passing in the 'interp' data generated in the vertex shader. This returns a ``GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES``, which contains the water surface normal generated by the simulation.

For Direct3D 11 apps, the shader-level integration is slightly different due to the use of tessellation:

1. The application calls ``GFSDK_WaveWorks_GetUndisplacedVertexWorldPosition()`` in its vertex shader. This returns a float4 world position, which should be passed on to the hull shader stage.
2. In the hull shader, the application calls ``GFSDK_WaveWorks_GetEdgeTessellationFactor()`` to calculate the tessellation factor for a particular edge, passing in the world positions of the ends of the edge.
3. The application calls ``GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation()`` in its domain shader. This returns a GFSDK_WAVEWORKS_VERTEX_OUTPUT, which contains the world position and displacement generated by the simulation for the displaced vertex, and also an 'interp' member which the app should pass to its pixel shader.

**GFSDK_WAVEWORKS_VERTEX_OUTPUT structure**

``GFSDK_WAVEWORKS_VERTEX_OUTPUT`` is returned by ``GFSDK_WaveWorks_GetDisplacedVertex()``, which is to be called in vertex shader in case of Direct3D 9 or Direct3D 10 integration, or ``GFSDK_WaveWorks_GetDisplacedVertexAfterTessellation()`` is called in domain shader in case of Direct3D 11 integration. It contains the following members:

#)  ``GFSDK_WAVEWORKS_INTERPOLATED_VERTEX_OUTPUT interp`` - this structure holds internal parameters that need to be passed to pixel shader and ``GFSDK_WaveWorks_GetSurfaceAttributes()`` function.
#)  ``float3 pos_world`` - worldspace position of displaced water vertex. Note that the x and y axes lie on the water plane, and the z axis is oriented towards the sky.
#)  ``float3 pos_world_undisplaced`` - the original position of water vertex before the displacement is applied. This parameter can be used if one needs to generate texture coords based on non-displaced water surface.
#)  ``float3 world_displacement`` - the actual displacement that was applied to the water vertex. This parameter can be used for implementing complex water surface shading.

**GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES structure**

GFSDK_WAVEWORKS_SURFACE_ATTRIBUTES structure is returned by ``GFSDK_WaveWorks_GetSurfaceAttributes()`` called in pixel shader. It contains the following members:

#)  ``float3 normal`` - the per-pixel water surface normal. It can be used to calculate Fresnel, reflection, refraction, etc.
#)  ``float3 eye_dir`` - normalized water surface pixel to camera position vector in worldspace coordinates. It can be used to calculate specular reflection, Fresnel, etc.
#)  ``float foam_surface_folding`` - this value provides the resulting amount of "squeezing" or "stretching" of the water surface, the range of values are [-1,1]. It is negative in areas where the water surface is "stretched;" for instance, in valleys between the waves, and positive on tips of the waves. It is useful for rendering water surface foam: the foam is expected to be denser in "squeezed" areas and thinner in "stretched" areas.
#)  ``float foam_turbulent_energy`` - this value provides the result of turbulent energy simulation, the range is [0,+inf], and the actual value highly depends on foam simulation parameters. This value is used to render surface foam and bubbles spread in water. It is zero in areas where turbulent energy is absent, and it is positive in areas where turbulent energy is present. The higher the value, the more turbulent energy exists in the area, and the denser foam can be applied to the water surface.
#)  ``float foam_wave_hats`` - this value marks the areas where turbulent energy is generated: the very tips of the waves that are about to break. The range is [0,+inf], and the actual value depends on foam simulation parameters. This value is used to render foamy wave tips: an additional foam texture can be modulated by this value.

**Register assignments**

The shader fragments use various constants and resources which need to be assigned to registers. No two applications handle their register assignments in the same way, so the library allow applications to manage assignments by defining pre-processor macros. The sample app shows how to define the macros so that registers are assigned via name-based lookup (for use with the ``D3DXEffect`` framework) - see ``ocean_surface.fx``. Alternatively, it is possible to define the macros so that registers are assigned to pre-determined contiguous ranges.

The application communicates register assignments to the library via the ``pShaderInputRegisterMappings`` parameter (see ``GFSDK_WaveWorks_Simulation_SetRenderStateXXXX()`` and ``GFSDK_WaveWorks_Quadtree_DrawXXXX()``). This should point to an array of UINTs of the size specified by ``GFSDK_WaveWorks_XXX_GetShaderInputCountXXXX()`` - each entry in the array represents a register mapping. A description of *what* is being mapped by an entry can be obtained by calling ``GFSDK_WaveWorks_XXX_GetShaderInputDescXXXX()`` - this returns a ``GFSDK_WaveWorks_ShaderInput_Desc``, which the application can use to determine which register applies to which entry. The sample apps use the *Type* and *Name*  information in ``GFSDK_WaveWorks_ShaderInput_Desc`` to fetch register assignments from ``D3DXEffect``. Alternatively, an application might use *RegisterOffset*  in cases where registers are assigned to pre-determined contiguous ranges.

OpenGL compatibility
--------------------

WaveWorks targets GL2, meaning it will only ever call GL entrypoints that are part of the GL2 core spec.

However, it is perfectly possible to use WaveWorks with later-version GL contexts, provided you fully understand the implications. In particular, watch out for:

#) VAOs - WaveWorks will not create or bind/unbind VAOs. If an app leaves a VAO bound prior to calling a GL-specific WaveWorks entrypoint, it is likely that the state of the VAO will be disrupted by WaveWorks. The recommended usage pattern here is for the app to create a VAO specifically for WaveWorks, and bind it prior to calling any WaveWorks entrypoint with 'GL' in the name. The VAO will act as a sandpit and prevent vertex-related state-changes leaking out of WaveWorks and affecting the rest of the app
#) Samplers - WaveWorks will not create or bind/unbind samplers. If an app leaves a sampler bound prior to calling a GL-specific WaveWorks entrypoint, it is likely that the sampler state will override the texture object state set by WaveWorks, leading to undefined results. The recommended usage pattern here is to unbind all samplers prior to calling any WaveWorks entrypoint with 'GL' in the name.

For GL rendering, it is necessary to reserve a handful of texture units for WaveWorks' exclusive use. These reserved texture units are specified to WaveWorks by filling out the 
GFSDK_WaveWorks_Simulation_GL_Pool data structure. The number of units required can be queried by calling GFSDK_WaveWorks_Simulation_GetTextureUnitCountGL2(), and the answer will
depend on whether the path that uses GL texture arrays has been specified. This option causes WaveWorks to use a combined texture array for shader input, which has the benefit of
reducing the number of texture units required at the expense of some additional internal copying of simulation data.

Device save/restore
-------------------

The library makes extensive changes to graphics device state, and this can cause problems with applications that have their own device state management layer, or which make assumptions about device state being preserved at certain times. For this reason, the library provides an optional facility to selectively save and restore device state across library calls, or (and this is important for efficiency) groups of calls. To use device state save/restore:

1. On initialization, call ``GFSDK_WaveWorks_Savestate_CreateXXX()`` to create a save-state object - the creation flags are used to determine what state the object will manage.
2. Pass the save-state object handle to functions that accept an hSavestate handle.
3. Call ``GFSDK_WaveWorks_Savestate_Restore()`` to restore the device state that was overwritten by the last batch of library calls.

Note that save/restore is offered only for graphics APIs where the device state can be queried efficiently - for example, it is not offered for OpenGL or GNM.

For OpenGL, clients of the library may implement save/restore by hooking their own wrapper functions into the table of GL entrypoint bindings (GFSDK_WAVEWORKS_GLFunctions) which is passed to the library on initialization. Also, be aware that the majority of OpenGL state-disruption issues can be solved with the following application changes:

1. if the application uses VAOs, create a dedicated VAO just for WaveWorks and bind it prior to calls to WaveWorks GL functions (see 'OpenGL compatibility')
2. restore glViewport() as necessary after calls to WaveWorks entrypoints with 'GL' in the name
3. restore the state of GL_DEPTH_TEST as necessary after calls to WaveWorks with 'GL' in the name
4. if the application uses samplers, ensure all samplers are unbound prior to calls to WaveWorks GL functions (see 'OpenGL compatibility')

Host readback
-------------
Some applications will need access to the displacements generated by the simulation (for example, so that water-borne objects can be made to bob accurately). Applications can use ``GFSDK_WaveWorks_Simulation_GetDisplacements()`` for this - the application provides an array of world x-y coordinates, for which displacements are to be retrieved, and the function fills out a corresponding array with the displacement data. Note that this call will only provide non-zero data if the *readback_displacements* flag is set.

**Readback FIFO**

It is possible to archive a limited history of readback results in a FIFO maintained by the WaveWorks simulation. The number of entries available for this is determined by the ``num_readback_FIFO_entries`` setting.
Readback results are pushed efficiently into the FIFO by calling ``GFSDK_WaveWorks_Simulation_ArchiveDisplacements()`` (but note that this could evict older entries if the FIFO is full!).
FIFO results can then be accessed using ``GFSDK_WaveWorks_Simulation_GetArchivedDisplacements()``. This is identical to the ``GFSDK_WaveWorks_Simulation_GetDisplacements()``, save for the additional ``coord`` argument
which is used to specify which FIFO entry (or entries - interpolation is allowed) to read from.

Calculation of first (velocity) or second (acceleration) derivatives is a possibe application of readback FIFO.

**Ray-casting**

Applications may need to perform ray-cast tests against the simulated ocean surface e.g. to detect when the path of a bullet intersects a wave.

The ``GFSDK_WaveWorks_Simulation_GetDisplacements()`` entrypoint cannot be used *directly* to perform ray-casting queries, since the inputs to the entrypoint are 2D parameterized world-space coordinates, not true 3D rays.

However, it is possible to implement ray-casting by making *indirect* use of ``GFSDK_WaveWorks_Simulation_GetDisplacements()``. The D3D11 sample app includes illustrative ray-casting code along these lines - see OceanSurface::intersectRayWithOcean().

Synchronization
-------------
Conceptually, the WaveWorks pipeline consists of two main sections:

* ``Staging`` - this is the top part of the pipeline which does all of the CPU-side work to prepare for rendering, *including* scheduling any GPU simulation work and any subsequent graphics interop to make results available for rendering.
* ``Readback`` - this is the bottom part of the pipeline which occurs after simulation work is complete, and which (if necessary) transfers results back to the CPU for use with physics or other application logic.

WaveWorks can be driven using a number of different synchronization patterns.

#. Fully synchronized - simulation work is submitted via ``GFSDK_WaveWorks_Simulation_KickXXXX()``, the caller then uses ``GFSDK_WaveWorks_Simulation_GetStagingCursor()`` and ``GFSDK_WaveWorks_Simulation_AdvanceStagingCursorXXXX()`` to pump the pipeline until the results of the kick are staged for rendering.
#. Fully asynchronous - simulation work is submitted via ``GFSDK_WaveWorks_Simulation_KickXXXX()`` (with multiple calls on the first frame to fill the pipeline) and staged for rendering as and when results become available.
#. Opportunistic - simulation work is submitted via ``GFSDK_WaveWorks_Simulation_KickXXXX()``, the caller then performs other useful work whilst occasionally polling for results with a non-blocking call to ``GFSDK_WaveWorks_Simulation_AdvanceStagingCursorXXXX()``.

Maximum performance is achieved with a fully asynchronous pattern, and in practice it is actually very rare for an application to *require* anything
other than a fully asynchronous usage pattern. Any application where the time delta is broadly predictable one or two updates in advance
can usually be pipelined for fully asynchronous operation, and only applications with unpredictable or uncorrelated time deltas will *require*
a fully synchronous usage pattern.

Statistics
-------------

**Simulation stats**

These can be retrieved via ``GFSDK_WaveWorks_Simulation_GetStats()``. The following statistics are available:

*  ``CPU_main_thread_wait_time`` - CPU time spent by main app thread waiting for CPU FFT simulation results.
*  ``CPU_threads_start_to_finish_time`` - CPU wallclock time spent on CPU FFT simulation: time between 1st thread starts work and last thread finishes simulation work.
*  ``CPU_threads_total_time`` - CPU time spent on CPU FFT simulation: sum time spent in threads that perform simulation work.
*  ``GPU_simulation_time`` - GPU time spent on GPU simulation.
*  ``GPU_FFT_simulation_time`` - GPU simulation time spent on FFT.
*  ``GPU_gfx_time`` - GPU time spent on non-simulation; e.g., updating gradient maps.
*  ``GPU_update_time`` - Total GPU time spent on UpdateTickXXXX() workloads.

"**Quad-tree stats**"

These can be retrieved via ``GFSDK_WaveWorks_Quadtree_GetStats()``. The following statistics are available:

*  ``num_patches_drawn`` - useful for checking correct operation of frustum culling, LODing, and patch alloc/free.
*  ``CPU_quadtree_update_time`` - the CPU time spent frustum culling, LODing, and patch alloc/free.

.. Un-comment out if section is used
.. Additional Links
.. ################   

.. Possible content here includes any of the following:
.. * User guide
.. * Videos
.. * Forums
.. * Etc.

Browse Documentation
#####################
.. toctree::
   :maxdepth: 1

   releasenotes
   changelog
.. Reference your chapters here. Chapters will not be listed if not defined here. 
..   chapter1
..   chatper2

.. Example of Getting Start Guide link
.. _Getting Started Guide: gettingstarted.html
