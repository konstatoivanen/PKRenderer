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

    enum class PKAssetType : uint8_t
    {
        Invalid,
        Shader,
        Mesh,
        Animation
    };

    enum class PKElementType : uint16_t
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
        Float3x4,

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

    enum class PKShaderStage : uint8_t
    {
        Vertex,
        TesselationControl,
        TesselationEvaluation,
        Geometry,
        Fragment,
        Compute,
        MeshTask,
        MeshAssembly,
        RayGeneration,
        RayMiss,
        RayClosestHit,
        RayAnyHit,
        RayIntersection,
        MaxCount
    };

    // maps directly to the above values, exists for convenience.
    enum class PKShaderStageFlags : uint32_t
    {
        None = 0u,
        Vertex = 1u << (uint8_t)PKShaderStage::Vertex,
        TesselationControl = 1u << (uint8_t)PKShaderStage::TesselationControl,
        TesselationEvaluation = 1u << (uint8_t)PKShaderStage::TesselationEvaluation,
        Geometry = 1u << (uint8_t)PKShaderStage::Geometry,
        Fragment = 1u << (uint8_t)PKShaderStage::Fragment,
        Compute = 1u << (uint8_t)PKShaderStage::Compute,
        MeshTask = 1u << (uint8_t)PKShaderStage::MeshTask,
        MeshAssembly = 1u << (uint8_t)PKShaderStage::MeshAssembly,
        RayGeneration = 1u << (uint8_t)PKShaderStage::RayGeneration,
        RayMiss = 1u << (uint8_t)PKShaderStage::RayMiss,
        RayClosestHit = 1u << (uint8_t)PKShaderStage::RayClosestHit,
        RayAnyHit = 1u << (uint8_t)PKShaderStage::RayAnyHit,
        RayIntersection = 1u << (uint8_t)PKShaderStage::RayIntersection,

        StagesGraphics = Vertex | TesselationControl | TesselationEvaluation | Geometry | Fragment | MeshTask | MeshAssembly,
        StagesVertex = Vertex | TesselationControl | TesselationEvaluation | Geometry,
        StagesMesh = MeshTask | MeshAssembly,
        StagesCompute = Compute,
        StagesRayTrace = RayGeneration | RayMiss | RayClosestHit | RayAnyHit | RayIntersection,
        RayTraceGroupGeneration = RayGeneration,
        RayTraceGroupMiss = RayMiss,
        RayTraceGroupHit = RayClosestHit | RayAnyHit | RayIntersection,
        RayTraceGroupCallable = 0u // Not supported atm
    };

    enum class PKDescriptorType : uint8_t
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
        AccelerationStructure,
    };

    enum class PKComparison : uint8_t
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

    enum class PKCullMode : uint8_t
    {
        Off,
        Front,
        Back
    };

    enum class PKBlendFactor : uint8_t
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

    enum class PKBlendOp : uint8_t
    {
        None,
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

    enum class PKRasterMode : uint8_t
    {
        Default,
        OverEstimate,
        UnderEstimate,
    };

    PKElementType GetElementType(const char* string);
    uint32_t GetElementSize(PKElementType type);
    uint32_t GetElementAlignment(PKElementType type);
    uint32_t GetElementComponents(PKElementType type);

    struct PKAssetMeta
    {
        char* optionNames = nullptr;
        uint32_t* optionValues = nullptr;
        uint32_t optionCount = 0u;
    };

    struct PKEncNode
    {
        uint32_t left : 11;
        uint32_t right : 11;
        uint32_t isLeaf : 2;
        uint32_t value : 8;
    };

    struct alignas(8) PKAssetHeader
    {
        uint64_t magicNumber = PK_ASSET_MAGIC_NUMBER;   // 8 bytes
        char name[PK_ASSET_NAME_MAX_LENGTH]{};          // 72 bytes
        PKAssetType type = PKAssetType::Invalid;        // 73 bytes
        bool isCompressed = false;                      // 74 bytes

        uint16_t compressedOffset = 0u;                 // 76 bytes
        uint32_t uncompressedSize = 0u;                 // 80 bytes
        uint64_t compressedBitCount = 0u;               // 88 bytes
    };

    namespace Shader
    {
        constexpr const static char* PK_SHADER_ATTRIB_LOGVERBOSE = "#LogVerbose";
        constexpr const static char* PK_SHADER_ATTRIB_GENERATEDEBUGINFO = "#GenerateDebugInfo";
        constexpr const static char* PK_SHADER_ATTRIB_ZWRITE = "#ZWrite ";
        constexpr const static char* PK_SHADER_ATTRIB_ZTEST = "#ZTest ";
        constexpr const static char* PK_SHADER_ATTRIB_BLENDCOLOR = "#BlendColor ";
        constexpr const static char* PK_SHADER_ATTRIB_BLENDALPHA = "#BlendAlpha ";
        constexpr const static char* PK_SHADER_ATTRIB_COLORMASK = "#ColorMask ";
        constexpr const static char* PK_SHADER_ATTRIB_CULL = "#Cull ";
        constexpr const static char* PK_SHADER_ATTRIB_OFFSET = "#Offset ";
        constexpr const static char* PK_SHADER_ATTRIB_RASTERMODE = "#RasterMode ";
        constexpr const static char* PK_SHADER_ATTRIB_MULTI_COMPILE = "#multi_compile ";
        constexpr const static char* PK_SHADER_ATTRIB_MATERIAL_PROP = "#MaterialProperty ";
        constexpr const static char* PK_SHADER_ATTRIB_INSTANCING_PROP = "#EnableInstancing";
        constexpr const static char* PK_SHADER_ATTRIB_INSTANCING_NOFRAG_PROP = "#DisableFragmentInstancing";
        constexpr const static char* PK_SHADER_ATTRIB_ATOMICCOUNTER = "#WithAtomicCounter";

        constexpr const static char* PK_SHADER_INSTANCING_TRANSFORMS = "pk_Instancing_Transforms";
        constexpr const static char* PK_SHADER_INSTANCING_INDICES = "pk_Instancing_Indices";
        constexpr const static char* PK_SHADER_INSTANCING_PROPERTIES = "pk_Instancing_Properties";
        constexpr const static char* PK_SHADER_INSTANCING_TEXTURES2D = "pk_Instancing_Textures2D";
        constexpr const static char* PK_SHADER_INSTANCING_TEXTURES3D = "pk_Instancing_Textures3D";
        constexpr const static char* PK_SHADER_INSTANCING_TEXTURESCUBE = "pk_Instancing_TexturesCube";
        constexpr const static char* PK_SHADER_ATOMIC_COUNTER = "pk_BuiltInAtomicCounter";
        constexpr const static char* PK_SHADER_SET_NAMES[4] =
        {
            "GLOBAL",
            "PASS",
            "SHADER",
            "DRAW"
        };

        struct alignas(4) PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
            PKElementType type;                  // 66 bytes
            uint16_t location;                   // 68 bytes
        };

        struct alignas(4) PKMaterialProperty
        {
            char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
            PKElementType type;                  // 66 bytes

            uint16_t __padding = 0u;             // 68 bytes
        };

        struct alignas(4) PKConstantVariable
        {
            char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
            uint16_t size;                       // 66 bytes
            uint16_t offset;                     // 68 bytes
            PKShaderStageFlags stageFlags;       // 72 bytes
        };

        struct alignas(4) PKDescriptor
        {
            char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
            PKShaderStageFlags writeStageMask;   // 68 bytes
            uint16_t count;                      // 70 bytes
            PKDescriptorType type;               // 71 bytes

            uint8_t __padding0 = 0u;             // 72 bytes
        };

        struct alignas(4) PKShaderKeyword
        {
            char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
            uint32_t offsets;                    // 68 bytes
        };

        struct alignas(4) PKShaderFixedStateAttributes
        {
            PKComparison ztest = PKComparison::Off;                     // 2 bytes
            PKCullMode cull = PKCullMode::Off;                          // 4 bytes
            PKRasterMode rasterMode = PKRasterMode::Default;            // 6 bytes
            PKBlendFactor blendSrcFactorColor = PKBlendFactor::None;    // 8 bytes
            PKBlendFactor blendDstFactorColor = PKBlendFactor::None;    // 10 bytes
            PKBlendFactor blendSrcFactorAlpha = PKBlendFactor::None;    // 12 bytes
            PKBlendFactor blendDstFactorAlpha = PKBlendFactor::None;    // 14 bytes
            PKBlendOp blendOpColor = PKBlendOp::None;                   // 16 bytes
            PKBlendOp blendOpAlpha = PKBlendOp::None;                   // 18 bytes
            uint8_t overEstimation = 0x0;                               // 20 bytes
            uint8_t colorMask = 0xFF;                                   // 22 bytes
            uint8_t zwrite = 0x0;                                       // 24 bytes
            float zoffsets[3] = { 0, 0, 0 };                            // 36 bytes
        };

        struct alignas(4) PKDescriptorSet
        {
            RelativePtr<PKDescriptor> descriptors; // 4 bytes
            uint32_t descriptorCount;              // 8 bytes
            PKShaderStageFlags stageflags;         // 12 bytes
        };

        struct alignas(4) PKShaderVariant
        {
            uint32_t descriptorSetCount;                                        // 4 bytes
            uint32_t constantVariableCount;                                     // 8 bytes
            uint32_t groupSize[4]{};                                            // 24 bytes
            RelativePtr<PKDescriptorSet> descriptorSets;                        // 28 bytes
            RelativePtr<PKConstantVariable> constantVariables;                  // 32 bytes
            PKVertexAttribute vertexAttributes[PK_ASSET_MAX_VERTEX_ATTRIBUTES]; // 576 bytes
            uint32_t sprivSizes[(int)PKShaderStage::MaxCount];                  // 628 bytes
            RelativePtr<void> sprivBuffers[(int)PKShaderStage::MaxCount];       // 680 bytes
        };

        struct alignas(4) PKShader
        {
            uint32_t materialPropertyCount = 0;                 // 4 bytes
            uint32_t keywordCount = 0;                          // 8 bytes
            uint32_t variantcount = 0;                          // 12 bytes
            PKShaderFixedStateAttributes attributes;            // 48 bytes
            RelativePtr<PKMaterialProperty> materialProperties; // 52 bytes
            RelativePtr<PKShaderKeyword> keywords;              // 56 bytes
            RelativePtr<PKShaderVariant> variants;              // 60 bytes
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

        // Meshlets
        namespace Meshlet
        {
            constexpr static const uint32_t PK_MAX_VERTICES = 64u;
            constexpr static const uint32_t PK_MAX_TRIANGLES = 124u;
            constexpr static const float PK_CONE_WEIGHT = 0.9f;

            // packed as uint4
            struct PKVertex
            {
                uint32_t posxy;      // unorm16     position.xy
                uint16_t posz;       // unorm16     position.z
                uint16_t colorSigns; // 4r4g4b color, 4a tangent sign
                uint32_t texcoord;   // half2       texcoord
                uint32_t rotation;   // r10g10b10a2 quaternion
            };

            // packed as 2x uint4
            struct PKMeshlet
            {
                uint32_t vertexFirst;   // 4  bytes
                uint32_t triangleFirst; // 8  bytes
                int8_t   coneAxis[3];   // 11 bytes snorm direction
                int8_t   coneCutoff;    // 12 bytes snorm8
                uint8_t  vertexCount;   // 13 bytes
                uint8_t  triangleCount; // 14 bytes
                uint16_t coneApex[3];   // 20 bytes half3
                uint16_t center[3];     // 26 bytes half3
                uint16_t extents[3];    // 32 bytes half3
            };

            // packed as 2x uint4
            struct PKSubmesh
            {
                float bbmin[3];         // 12 bytes
                uint32_t firstMeshlet;  // 16 bytes
                float bbmax[3];         // 28 bytes
                uint32_t meshletCount;  // 32 bytes
            };

            struct PKMesh
            {
                uint32_t triangleCount;           // 4 bytes
                uint32_t vertexCount;             // 8 bytes
                uint32_t submeshCount;            // 12 bytes
                uint32_t meshletCount;            // 16 bytes
                RelativePtr<PKMeshlet> meshlets;  // 20 bytes
                RelativePtr<PKSubmesh> submeshes; // 24 bytes
                RelativePtr<PKVertex> vertices;   // 28 bytes
                RelativePtr<uint8_t> indices;     // 32 bytes
            };

            PKVertex PackVertex(const float* pPosition, 
                                const float* pTexcoord, 
                                const float* pNormal, 
                                const float* pTangent, 
                                const float* submeshbbmin, 
                                const float* submeshbbmax);

            PKMeshlet PackMeshlet(uint32_t vertexFirst, 
                                  uint32_t triangleFirst, 
                                  uint32_t vertexCount, 
                                  uint32_t triangleCount,
                                  const int8_t* coneAxis,
                                  int8_t coneCutoff,
                                  const float* coneApex,
                                  const float* center,
                                  const float* extents);
        }

        struct PKVertexAttribute
        {
            char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
            PKElementType type;                  // 66 bytes
            uint16_t size = 0;                   // 68 bytes
            uint16_t offset = 0;                 // 70 bytes
            uint16_t stream = 0;                 // 72 bytes
        };

        struct PKSubmesh
        {
            uint32_t firstIndex; // 4 bytes
            uint32_t indexCount; // 8 bytes
            float bbmin[3]{};    // 20 bytes
            float bbmax[3]{};    // 32 bytes
        };

        struct PKMesh
        {
            uint32_t indexCount;                                // 4 bytes
            uint32_t vertexCount;                               // 8 bytes
            uint32_t submeshCount;                              // 12 bytes
            uint32_t vertexAttributeCount;                      // 16 bytes
            RelativePtr<PKVertexAttribute> vertexAttributes;    // 20 bytes
            RelativePtr<PKSubmesh> submeshes;                   // 24 bytes
            RelativePtr<void> vertexBuffer;                     // 28 bytes
            RelativePtr<void> indexBuffer;                      // 32 bytes
            RelativePtr<Meshlet::PKMesh> meshletMesh;           // 36 bytes
            PKElementType indexType;                            // 38 bytes

            uint16_t __padding = 0u;                            // 40 bytes
        };
    }

    struct PKAsset
    {
        union
        {
            void* rawData = nullptr;
            PKAssetHeader* header;
        };
    };
}
