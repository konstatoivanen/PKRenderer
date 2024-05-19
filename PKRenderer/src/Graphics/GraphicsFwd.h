#pragma once
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics
{
    typedef RHI::RHIDriver Driver;
    typedef RHI::RHIAccelerationStructure AccelerationStructure;
    typedef RHI::RHIBuffer Buffer;
    typedef RHI::RHICommandBuffer CommandBuffer;
    typedef RHI::RHIQueueSet QueueSet;
    typedef RHI::RHITexture Texture;
    typedef RHI::RHIWindow Window;
    typedef RHI::RHITextureBindArray TextureBindArray;
    typedef RHI::RHIBufferBindArray BufferBindArray;

    typedef RHI::RHIAccelerationStructureRef AccelerationStructureRef;
    typedef RHI::RHITextureBindArrayRef TextureBindArrayRef;
    typedef RHI::RHIBufferBindArrayRef BufferBindArrayRef;
    typedef RHI::RHIBufferRef BufferRef;
    typedef RHI::RHITextureRef TextureRef;
    typedef RHI::RHIWindowScope WindowScope;
    typedef RHI::RHIDriverScope DriverScope;

    // Wrapper types & utilities
    struct Shader;
    struct ConstantBuffer;
    struct CommandBufferExt;
    struct Material;
    struct Mesh;
    struct MeshStatic;
    struct SubMeshStatic;
    struct MeshStaticAsset;
    struct MeshStaticAllocationData;
    class MeshStaticCollection;
    struct ShaderBindingTable;
    struct ShaderPropertyBlock;
    struct TextureAsset;

    typedef Utilities::Ref<ConstantBuffer> ConstantBufferRef;
    typedef Utilities::Ref<MeshStaticAsset> MeshStaticAssetRef;
    typedef Utilities::Ref<TextureAsset> TextureAssetRef;
}