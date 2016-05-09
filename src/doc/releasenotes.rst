Release Notes 
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
