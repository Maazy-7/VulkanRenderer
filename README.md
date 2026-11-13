# Hyrbid Vulkan Renderer
A real time hybrid renderer built in Vulkan 1.4. The renderer uses deferred rasterization for the primary shading pass, while hardware accelerated ray tracing is used for reflections and shadows. The renderer leverages Vulkan's ray tracing extensions to make use of the gpu's ray tracing hardware for efficient BVH traversal and ray-triangle/box intersections. It uses stb_image for loading textures, tinyobjloader for loading 3d models, glm for mathematics and GLFW for windowing and input. Physically based rendering is implemented using the Cook Torrance BRDF and custom pbr materials.

# Renderer Features
- **Modern Vulkan 1.4 usage**
  - Dynamic rendering
  - Descriptor Indexing
  - Synchronization2
- **Real time 3D rendering pipeline**
  - Deferred rasterization
  - Hardware accelerated ray tracing
  - Efficient asset loading
- **Lighting**
  - Physically based rendering 
  - Cook Torrance BRDF
  - Metallic and roughness materials
  - Normal maps
- **Rasterized Shadows**
  - Directional light shadow maps
  - Point light shadow cubemaps
- **Ray Tracing**
    - Top level and Bottom level acceleration structure (BLAS/TLAS)
    - Ray Tracing pipeline and shader binding table (SBT)
    - Ray traced reflections
    - Ray traced shadows
    - Ray generation (`.rgen`), miss (`.rmiss`), and closest hit (`.rchit`) shaders
- **Post Processing**
  - Blending raster and rt results
  - Compute shader blending pass
  - Gamma correction and tone mapping


