#pragma once
#include "Core/RHI/RHI.h"

namespace PK
{
    // Wrapper types & utilities
    struct ShaderAsset;
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

    typedef Ref<ConstantBuffer> ConstantBufferRef;
    typedef Ref<MeshStaticAsset> MeshStaticAssetRef;
    typedef Ref<TextureAsset> TextureAssetRef;
}