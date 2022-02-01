#pragma once
#include <cstdint>

namespace PK::Assets
{
    typedef uint32_t relativePtr;

    constexpr static const uint64_t PK_ASSET_MAGIC_NUMBER = 16056123332373007180ull;
    constexpr static const uint32_t PK_ASSET_NAME_MAX_LENGTH = 64;
    constexpr static const uint32_t PK_ASSET_MAX_VERTEX_ATTRIBUTES = 8;
    constexpr static const uint32_t PK_ASSET_MAX_DESCRIPTOR_SETS = 4;
    constexpr static const uint32_t PK_ASSET_MAX_DESCRIPTORS_PER_SET = 16;
    constexpr static const uint32_t PK_ASSET_MAX_SHADER_KEYWORDS = 256;
    constexpr static const uint32_t PK_ASSET_MAX_UNBOUNDED_SIZE = 2048;

    constexpr static const char* PK_ASSET_EXTENSION_SHADER = ".pkshader";
    constexpr static const char* PK_ASSET_EXTENSION_MESH = ".pkmesh";
    constexpr static const char* PK_ASSET_EXTENSION_ANIM = ".pkanim";

    template<typename T>
    struct RelativePtr
    {
        uint32_t offset = 0;

        T* Get(void* base)
        {
            auto cptr = reinterpret_cast<char*>(base) + offset;
            return reinterpret_cast<T*>(cptr);
        }

        void Set(void* base, T* value)
        {
            offset = (uint32_t)(reinterpret_cast<char*>(value) - reinterpret_cast<char*>(base));
        }
    };

    enum class PKAssetType : unsigned char
    {
        Invalid,
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

        Texture2DHandle,
        Texture3DHandle,
        TextureCubeHandle,
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
        ConstantBuffer,
        StorageBuffer,
        DynamicConstantBuffer,
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

    PKElementType GetElementType(const char* string);
    uint32_t GetElementSize(PKElementType type);
    uint32_t GetElementAlignment(PKElementType type);
    uint32_t GetElementComponents(PKElementType type);

    struct PKEncNode
    {
        RelativePtr<PKEncNode> left;
        RelativePtr<PKEncNode> right;
        char value;
        bool isLeaf;
    };

    struct PKAssetHeader
    {
        uint64_t magicNumber = PK_ASSET_MAGIC_NUMBER;
        char name[PK_ASSET_NAME_MAX_LENGTH]{};
        PKAssetType type = PKAssetType::Invalid;
        bool isCompressed = false;
    };

    namespace Shader
    {
        constexpr const static char* PK_SHADER_ATTRIB_ZWRITE = "#ZWrite ";
        constexpr const static char* PK_SHADER_ATTRIB_ZTEST = "#ZTest ";
        constexpr const static char* PK_SHADER_ATTRIB_BLENDCOLOR = "#BlendColor ";
        constexpr const static char* PK_SHADER_ATTRIB_BLENDALPHA = "#BlendAlpha ";
        constexpr const static char* PK_SHADER_ATTRIB_COLORMASK = "#ColorMask ";
        constexpr const static char* PK_SHADER_ATTRIB_CULL = "#Cull ";
        constexpr const static char* PK_SHADER_ATTRIB_OFFSET = "#Offset ";
        constexpr const static char* PK_SHADER_ATTRIB_MULTI_COMPILE = "#multi_compile ";
        constexpr const static char* PK_SHADER_ATTRIB_MATERIAL_PROP = "#MaterialProperty ";
        constexpr const static char* PK_SHADER_ATTRIB_INSTANCING_PROP = "#EnableInstancing";
        
        constexpr const static char* PK_SHADER_INSTANCING_TRANSFORMS = "pk_Instancing_Transforms";
        constexpr const static char* PK_SHADER_INSTANCING_INDICES = "pk_Instancing_Indices";
        constexpr const static char* PK_SHADER_INSTANCING_PROPERTIES = "pk_Instancing_Properties";
        constexpr const static char* PK_SHADER_INSTANCING_TEXTURES2D = "pk_Instancing_Textures2D";
        constexpr const static char* PK_SHADER_INSTANCING_TEXTURES3D = "pk_Instancing_Textures3D";
        constexpr const static char* PK_SHADER_INSTANCING_TEXTURESCUBE = "pk_Instancing_TexturesCube";
        
        enum class Type : unsigned char
        {
            Graphics,
            Compute
        };

        struct alignas(4) PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKElementType type;
            uint16_t location;
        };

        struct alignas(4) PKMaterialProperty
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKElementType type;
        };

        struct alignas(4) PKConstantVariable
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            uint16_t size;
            uint16_t offset;
            uint16_t stageFlags;
        };

        struct alignas(4) PKDescriptor
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            PKDescriptorType type;
            uint16_t count;
        };

        struct alignas(4) PKShaderKeyword
        {
            char name[PK_ASSET_NAME_MAX_LENGTH];
            uint32_t offsets;
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
            uint16_t colorMask = 0xFF;
            uint16_t zwrite = 0x0;
            float zoffsets[3] = { 0, 0, 0 };
        };

        struct PKDescriptorSet
        {
            uint32_t stageflags;
            uint32_t descriptorCount;
            RelativePtr<PKDescriptor> descriptors;
        };

        struct PKShaderVariant
        {
            uint32_t descriptorSetCount;
            uint32_t constantVariableCount;
            RelativePtr<PKDescriptorSet> descriptorSets;
            RelativePtr<PKConstantVariable> constantVariables;
            PKVertexAttribute vertexAttributes[PK_ASSET_MAX_VERTEX_ATTRIBUTES];
            uint32_t sprivSizes[(int)PKShaderStage::MaxCount];
            RelativePtr<void> sprivBuffers[(int)PKShaderStage::MaxCount];
        };

        struct PKShader
        {
            Type type = Type::Graphics;
            uint32_t materialPropertyCount = 0;
            uint32_t keywordCount = 0;
            uint32_t variantcount = 0;
            PKShaderFixedStateAttributes attributes;
            RelativePtr<PKMaterialProperty> materialProperties;
            RelativePtr<PKShaderKeyword> keywords;
            RelativePtr<PKShaderVariant> variants;
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
            uint16_t size = 0;
            uint16_t offset = 0;
        };

        struct PKSubmesh
        {
            uint32_t firstIndex;
            uint32_t indexCount;
            float bbmin[3]{};
            float bbmax[3]{};
        };

        struct PKMesh
        {
            PKElementType indexType;
            uint32_t indexCount;
            uint32_t vertexCount;
            uint32_t vertexStride;
            uint32_t submeshCount;
            uint32_t vertexAttributeCount;
            RelativePtr<PKVertexAttribute> vertexAttributes;
            RelativePtr<PKSubmesh> submeshes;
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
