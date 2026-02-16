#pragma once
#include "Core/RHI/RHI.h"

namespace PK
{
    // Wrapper types & utilities
    struct TextureAsset;
    struct ShaderAsset;
    struct Material;
    struct Font;
    struct Mesh;
    struct MeshStatic;
    struct SubMeshStatic;
    struct MeshStaticAsset;
    struct MeshStaticDescriptor;
    class MeshStaticCollection;
    struct ConstantBuffer;
    struct CommandBufferExt;
    struct ShaderBindingTable;
    struct ShaderProperty;
    struct ShaderPropertyLayout;
    struct ShaderPropertyWriter;
    struct ShaderPropertyBlock;
    struct Window;
    enum class TextAlign;

    typedef Ref<TextureAsset> TextureAssetRef;
    typedef Ref<ShaderAsset> ShaderAssetRef;
    typedef Ref<Material> MaterialRef;
    typedef Ref<Font> FontRef;
    typedef Ref<Mesh> MeshRef;
    typedef Ref<MeshStaticAsset> MeshStaticAssetRef;
    typedef Ref<ConstantBuffer> ConstantBufferRef;
    typedef Unique<Window> WindowScope;
}
