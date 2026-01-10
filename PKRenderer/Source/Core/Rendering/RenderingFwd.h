#pragma once
#include "Core/RHI/RHI.h"

namespace PK
{
    // Wrapper types & utilities
    struct ShaderAsset;
    struct ConstantBuffer;
    struct StorageBuffer;
    struct CommandBufferExt;
    struct Material;
    struct Mesh;
    struct MeshStatic;
    struct SubMeshStatic;
    struct MeshStaticAsset;
    struct MeshStaticDescriptor;
    class MeshStaticCollection;
    struct ShaderBindingTable;
    struct ShaderPropertyBlock;
    struct TextureAsset;
    struct Font;
    struct Window;
    enum class TextAlign;

    typedef Ref<ConstantBuffer> ConstantBufferRef;
    typedef Ref<StorageBuffer> StorageBufferRef;
    typedef Ref<MeshStaticAsset> MeshStaticAssetRef;
    typedef Ref<TextureAsset> TextureAssetRef;
    typedef Ref<Font> FontRef;
    typedef Unique<Window> WindowScope;
}
