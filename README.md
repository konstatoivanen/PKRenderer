# PKRenderer
Is a renderer developed for the implementation & testing of various rendering techniques using modern graphics APIs. 
The project is built for Windows using C++ 17, Visual Studio & MSVC and supports Vulkan as a rendering backend (DX12 support is planned).

## Preview
![Preview](T_Preview_01.gif?raw=true "GI Preview")

## Features
- Vulkan 1.2 Rendering Backend.
- Dynamic scene material batching & instanced rendering.
- Clustered forward rendering.
- Volumetric fog & lighting.
- Realtime global illumination (Voxel cone tracing).
- Temporal reprojection for volumetrics & screen space GI.
- Point, spot & directional lights.
- Variance shadow mapping.
- Cascaded shadow maps.
- Light cookies.
- Physically based shading.
- HDR bloom.
- Tone mapping & color grading.
- Film grain.
- Auto exposure.
- Depth of field with auto focus.
- Octahedron hdr environment maps.
- Multi compile shader variants.
- Asset hot reloading.

## Planned Features
- Hardware accelerated raytraced GI (deprecate VXGI).
- Virtualized shared scene vertex buffer (improve performance by using multidraw indirect).
- Mesh Skinning.
- Serialized scene representation.
- DX12 Rendering backend.
- Improved bloom effect performance.
  - Current one is a quick direct port from GLSLTestbed which used bindless textures to avoid frame buffer switches. The same technique is unsupported on Vulkan.
- Antialiasing.

## Libraries & Other Dependencies
- Windows 10 (Support for other platforms has not been tested/developed/verified).
- C++ 17 support required.
- Vulkan 1.2 support required.
- [PKAssetTools](https://github.com/konstatoivanen/PKAssetTools)
- [KTX](https://github.com/KhronosGroup/KTX-Software)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
- [GLFW](https://www.glfw.org/)
- [GLM](https://github.com/g-truc/glm)
- [mikktspace](http://www.mikktspace.com/)
- [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
