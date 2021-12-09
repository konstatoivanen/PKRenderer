#pragma once

namespace PK::Assets
{
    typedef unsigned int uint_t;
    typedef unsigned int relativePtr;

    constexpr static const unsigned int PK_ASSET_NAME_MAX_LENGTH = 64;
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

    enum class PKBlendOp 
    {
        None,
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

    struct PKEncNode
    {
        RelativePtr<PKEncNode> left;
        RelativePtr<PKEncNode> right;
        char value;
        bool isLeaf;
    };

    struct PKAssetHeader
    {
        char name[PK_ASSET_NAME_MAX_LENGTH];
        PKAssetType type;
        bool isCompressed;
    };

    namespace Shader
    {
        enum class Type : unsigned char
        {
            Graphics,
            Compute
        };

        struct alignas(4) PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKElementType type;
            unsigned short location;
        };

        struct alignas(4) PKConstantVariable
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            unsigned short size;
            unsigned short offset;
            unsigned short stageFlags;
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
            PKComparison ztest = PKComparison::Off;
            PKCullMode cull = PKCullMode::Off;
            PKBlendFactor blendSrcFactorColor = PKBlendFactor::None;
            PKBlendFactor blendDstFactorColor = PKBlendFactor::None;
            PKBlendFactor blendSrcFactorAlpha = PKBlendFactor::None;
            PKBlendFactor blendDstFactorAlpha = PKBlendFactor::None;
            PKBlendOp blendOpColor = PKBlendOp::None;
            PKBlendOp blendOpAlpha = PKBlendOp::None;
            unsigned short colorMask = 0xFF;
            unsigned short zwrite = 0x0;
            float zoffsets[3] = { 0, 0, 0 };
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
            uint_t constantVariableCount;
            RelativePtr<PKDescriptorSet> descriptorSets;
            RelativePtr<PKConstantVariable> constantVariables;
            PKVertexAttribute vertexAttributes[PK_ASSET_MAX_VERTEX_ATTRIBUTES];

            uint_t sprivSizes[(int)PKShaderStage::MaxCount];
            RelativePtr<void> sprivBuffers[(int)PKShaderStage::MaxCount];
        };

        struct PKShader
        {
            Type type = Type::Graphics;
            uint_t keywordCount = 0;
            RelativePtr<PKShaderKeyword> keywords;
            uint_t variantcount = 0;
            RelativePtr<PKShaderVariant> variants;
            PKShaderFixedStateAttributes attributes;
        };
    }

    namespace Mesh
    {
        constexpr const static char* PK_VS_POSITION = "in_POSITION";
        constexpr const static char* PK_VS_NORMAL = "in_NORMAL";
        constexpr const static char* PK_VS_TANGENT = "in_TANGENT";
        constexpr const static char* PK_VS_COLOR = "in_COLOR";
        constexpr const static char* PK_VS_TEXCOORD0 = "in_TEXCOORD0";
        constexpr const static char* PK_VS_TEXCOORD1 = "in_TEXCOORD1";
        constexpr const static char* PK_VS_TEXCOORD2 = "in_TEXCOORD2";
        constexpr const static char* PK_VS_TEXCOORD3 = "in_TEXCOORD3";

        struct PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKElementType type;
            unsigned short size = 0;
            unsigned short offset = 0;
        };

        struct PKIndexRange
        {
            uint_t offset;
            uint_t count;
        };

        struct PKMesh
        {
            float bbmin[3]{};
            float bbmax[3]{};
            PKElementType indexType;
            uint_t indexCount;
            uint_t vertexCount;
            uint_t vertexStride;
            uint_t submeshCount;
            uint_t vertexAttributeCount;
            RelativePtr<PKVertexAttribute> vertexAttributes;
            RelativePtr<PKIndexRange> submeshes;
            RelativePtr<void> vertexBuffer;
            RelativePtr<void> indexBuffer;
        };
    }

    struct PKAsset
    {
        PKAssetHeader* header = nullptr;
        void* rawData = nullptr;
    };
}
