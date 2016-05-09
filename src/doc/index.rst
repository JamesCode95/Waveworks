.. Replace existing content with product specific content. Layout for this page should be consistent for all products. 
   Use the root `toctree` directive to include documents

|PRODUCTNAME| 
======================================

.. Replace the content. Layout should not change

NVIDIA WaveWorks enables developers to deliver a cinematic-quality ocean simulation for interactive applications. The simulation runs in the frequency domain, using a spectral wave dispersion model. An inverse FFT step then transforms to the spatial domain, ready for rendering. The NVIDIA WaveWorks simulation is initialized and controlled by a simple C API, and the results are accessed for rendering through a HLSL shader API. Parameterization is done via intuitive real-world variables, such as wind speed and direction. These parameters can be used to tune the look of the sea surface for a wide variety of conditions – from gentle ripples to a heavy storm-tossed ocean based on the Beaufort scale. 

In addition, we also provide an energy-based surface foam simulation, which is locked to and driven by the underlying spectral simulation. The foam simulation results are also exposed through HLSL shader API, and permit full customization of the foam look, according to physical properties like surface energy and mixed-in turbulent energy. Version 1.3 also adds optional support for greatly simplified parameterization choices, based on the Beaufort scale.

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


Learn more about |PRODUCTNAME|
------------------------------
* Visit the `product home page`_ on `NVIDIA Developer`_

* View Documentation :ref:`search`

.. Other links to highlight:
.. Link to archived docs
.. Any other archived (version-specific) docs can be linked here as well.

**Browse Documentation**

.. toctree::
   :maxdepth: 1
   
   product
.. Reference only product TOT pages here. 
..   productOld
..   productOlder




