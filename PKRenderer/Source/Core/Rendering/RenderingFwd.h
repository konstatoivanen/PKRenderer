#pragma once
#include "Core/RHI/RHI.h"

namespace PK
{
    // Wrapper types & utilities
    struct TextureAsset;
    struct ShaderAsset;
    struct Material;
    struct Font;
    struct FontStyle;
    struct SubMesh;
    struct MeshDescriptor;
    struct MeshletsDescriptor;
    struct MeshStaticDescriptor;
    struct IMesh;
    struct IMeshlets;
    struct IRayTracingGeometry;
    struct MeshStaticAllocator;
    struct MeshStatic;
    struct Mesh;
    struct ConstantBuffer;
    struct CommandBufferExt;
    struct ShaderBindingTable;
    struct ShaderProperty;
    struct ShaderPropertyLayout;
    struct ShaderPropertyWriter;
    struct ShaderPropertyBlock;
    struct Window;

    typedef Ref<TextureAsset> TextureAssetRef;
    typedef Ref<ShaderAsset> ShaderAssetRef;
    typedef Ref<Material> MaterialRef;
    typedef Ref<Font> FontRef;
    typedef Ref<Mesh> MeshRef;
    typedef Ref<MeshStatic> MeshStaticRef;
    typedef Ref<ConstantBuffer> ConstantBufferRef;
    typedef Unique<Window> WindowScope;

}
