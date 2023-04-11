# PKRenderer
Is a renderer developed for the implementation & testing of various rendering techniques using modern graphics APIs. 
The project is built for Windows using C++ 17, Visual Studio & MSVC and supports Vulkan as a rendering backend (DX12 support is planned).

**Note that if you want to build this but don't want to build PKAssetTools you can place the prebuilt binaries from that repository into your Build folder instead.**

## Preview
![Preview](T_Preview_01.gif?raw=true "GI Preview")

## Features
- Vulkan 1.2 Rendering Backend.
- Dynamic scene material batching & instanced rendering.
- Clustered forward rendering.
- Virtual geometry.
- Volumetric fog & lighting.
- Realtime global illumination (Raytraced SHL1 specular & diffuse, world space voxel radiance cache).
- Temporal reprojection for volumetrics & screen space GI.
- Point, spot & directional lights.
- Temporal Antialiasing.
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
- Mesh Skinning.
- Serialized scene representation.
- DX12 Rendering backend.

## Required Vulkan Features & Extensions
- VK_EXT_debug_utils
- VK_LAYER_KHRONOS_validation
- VK_KHR_surface
- VK_KHR_win32_surface
- alphaToOne
- shaderImageGatherExtended
- sparseBinding
- sparseResidencyBuffer
- samplerAnisotropy
- multiViewport
- shaderSampledImageArrayDynamicIndexing
- shaderUniformBufferArrayDynamicIndexing
- shaderFloat64
- shaderInt16
- shaderInt64
- imageCubeArray
- fragmentStoresAndAtomics
- multiDrawIndirect
- storageBuffer16BitAccess
- uniformAndStorageBuffer16BitAccess
- storagePushConstant16
- shaderUniformBufferArrayNonUniformIndexing
- shaderSampledImageArrayNonUniformIndexing
- runtimeDescriptorArray
- descriptorBindingVariableDescriptorCount
- descriptorBindingPartiallyBound
- scalarBlockLayout
- shaderFloat16
- shaderInt8
- shaderOutputViewportIndex
- shaderOutputLayer

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
