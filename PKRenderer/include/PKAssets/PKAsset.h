#pragma once

namespace PK::Assets
{
    typedef unsigned int uint_t;
    typedef unsigned int relativePtr;

    constexpr static const unsigned int PK_ASSET_NAME_MAX_LENGTH = 128;
    constexpr static const unsigned int PK_ASSET_MAX_VERTEX_ATTRIBUTES = 8;
    constexpr static const unsigned int PK_ASSET_MAX_DESCRIPTOR_SETS = 4;
    constexpr static const unsigned int PK_ASSET_MAX_DESCRIPTORS_PER_SET = 16;
    constexpr static const unsigned int PK_ASSET_MAX_SHADER_KEYWORDS = 256;

    constexpr static const char* PK_ASSET_EXTENSION_SHADER = ".pkshader";
    constexpr static const char* PK_ASSET_EXTENSION_MESH = ".pkmesh";
    constexpr static const char* PK_ASSET_EXTENSION_ANIM = ".pkanim";

    template<typename T>
    struct RelativePtr
    {
        uint_t offset = 0;

        T* Get(void* base)
        {
            auto cptr = reinterpret_cast<char*>(base) + offset;
            return reinterpret_cast<T*>(cptr);
        }

        void Set(void* base, T* value)
        {
            offset = (uint_t)(reinterpret_cast<char*>(value) - reinterpret_cast<char*>(base));
        }
    };

    enum class PKAssetType : unsigned char
    {
        Shader,
        Mesh,
        Animation
    };

    enum class PKElementType : unsigned short
    {
        Invalid = 0,

        Float,
        Float2,
        Float3,
        Float4,

        Double,
        Double2,
        Double3,
        Double4,

        Half,
        Half2,
        Half3,
        Half4,

        Int,
        Int2,
        Int3,
        Int4,

        Uint,
        Uint2,
        Uint3,
        Uint4,

        Short,
        Short2,
        Short3,
        Short4,

        Ushort,
        Ushort2,
        Ushort3,
        Ushort4,

        Long,
        Long2,
        Long3,
        Long4,

        Ulong,
        Ulong2,
        Ulong3,
        Ulong4,

        Float2x2,
        Float3x3,
        Float4x4,

        Double2x2,
        Double3x3,
        Double4x4,

        Half2x2,
        Half3x3,
        Half4x4,
    };

    enum class PKShaderStage : unsigned char
    {
        Vertex,
        TesselationControl,
        TesselationEvaluation,
        Geometry,
        Fragment,
        Compute,
        MaxCount
    };

    enum class PKDescriptorType : unsigned char
    {
        Invalid,
        Sampler,
        SamplerTexture,
        Texture,
        Image,
        UniformBuffer,
        StorageBuffer,
        DynamicUniformBuffer,
        DynamicStorageBuffer,
        InputAttachment,
    };

    enum class PKComparison : unsigned char
    {
        Off,
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    enum class PKCullMode : unsigned char
    {
        Off,
        Front,
        Back
    };

    enum class PKBlendFactor : unsigned char
    {
        None,
        One,
        Zero,
        SrcColor,
        SrcAlpha,
        DstColor,
        DstAlpha,
        OneMinusSrcColor,
        OneMinusSrcAlpha,
        OneMinusDstColor,
        OneMinusDstAlpha,
        ConstColor,
        OneMinusConstColor,
        ConstAlpha,
        OneMinusConstAlpha,
    };

    struct PKAssetHeader
    {
        char name[PK_ASSET_NAME_MAX_LENGTH];
        PKAssetType type;
    };

    namespace Shader
    {
        struct alignas(4) PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKElementType type;
            unsigned short location;
        };

        struct alignas(4) PKDescriptor
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKDescriptorType type;
            unsigned char binding;
            unsigned short count;
        };

        struct alignas(4) PKShaderKeyword
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            uint_t offsets;
        };

        struct alignas(4) PKShaderFixedStateAttributes
        {
            PKComparison ztest;
            PKCullMode cull;
            PKBlendFactor blendSrc;
            PKBlendFactor blendDst;
            unsigned short colorMask;
            unsigned short zwrite;
            float zoffsets[3];
        };

        struct PKDescriptorSet
        {
            uint_t set;
            uint_t stageflags;
            uint_t descriptorCount;
            RelativePtr<PKDescriptor> descriptors;
        };

        struct PKShaderVariant
        {
            uint_t descriptorSetCount;
            RelativePtr<PKDescriptorSet> descriptorSets;
            PKVertexAttribute vertexAttributes[PK_ASSET_MAX_VERTEX_ATTRIBUTES];

            uint_t sprivSizes[(int)PKShaderStage::MaxCount];
            RelativePtr<void> sprivBuffers[(int)PKShaderStage::MaxCount];
        };

        struct PKShader
        {
            uint_t keywordCount = 0;
            RelativePtr<PKShaderKeyword> keywords;
            uint_t variantcount = 0;
            RelativePtr<PKShaderVariant> variants;
            PKShaderFixedStateAttributes attributes;
        };
    }

    namespace Mesh
    {
        struct PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKElementType type;
            unsigned short size = 0;
            unsigned short offset = 0;
            bool normalized = false;
        };

        struct PKMesh
        {
            PKVertexAttribute vertexAttributes[PK_ASSET_MAX_VERTEX_ATTRIBUTES];
            PKElementType indexType;
            RelativePtr<void> indexBuffer;
            uint_t indexBufferSize;
            RelativePtr<void> vertexBuffer;
            uint_t vertexBufferSize;
        };
    }

    struct PKAsset
    {
        PKAssetHeader* header = nullptr;
        void* rawData = nullptr;
    };
}