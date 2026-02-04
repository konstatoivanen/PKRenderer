#pragma once
#include <cstdint>

namespace PKAssets
{
    constexpr static const uint64_t PK_ASSET_MAGIC_NUMBER = 16056123332373007180ull;
    constexpr static const uint32_t PK_ASSET_NAME_MAX_LENGTH = 64u;
    constexpr static const uint32_t PK_ASSET_MAX_VERTEX_ATTRIBUTES = 8u;
    constexpr static const uint32_t PK_ASSET_MAX_DESCRIPTORS_PER_SET = 64u;
    constexpr static const uint32_t PK_ASSET_MAX_PUSH_CONSTANTS = 16u;
    constexpr static const uint32_t PK_ASSET_MAX_SHADER_KEYWORDS = 256u;
    constexpr static const uint32_t PK_ASSET_MAX_SHADER_DIRECTIVES = 16u;
    constexpr static const uint32_t PK_ASSET_MAX_UNBOUNDED_SIZE = 2048u;

    constexpr static const char* PK_ASSET_EXTENSION_SHADER = ".pkshader";
    constexpr static const char* PK_ASSET_EXTENSION_MESH = ".pkmesh";
    constexpr static const char* PK_ASSET_EXTENSION_FONT = ".pkfont";
    constexpr static const char* PK_ASSET_EXTENSION_TEXTURE = ".pktexture";

    // Base asset types
    enum class PKAssetType : uint8_t
    {
        Invalid,
        Shader,
        Mesh,
        Font,
        Texture
    };

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

    struct PKAssetMeta
    {
        char* optionNames = nullptr;
        uint32_t* optionValues = nullptr;
        uint32_t optionCount = 0u;
    };

    struct alignas(8) PKAssetHeader
    {
        uint64_t magicNumber = PK_ASSET_MAGIC_NUMBER;   // 8 bytes
        char name[PK_ASSET_NAME_MAX_LENGTH]{};          // 72 bytes
        PKAssetType type = PKAssetType::Invalid;        // 73 bytes
        bool isCompressed = false;                      // 74 bytes

        uint16_t __padding = 0u;                        // 76 bytes
        uint32_t uncompressedSize = 0u;                 // 80 bytes
    };

    struct PKAsset
    {
        union
        {
            void* rawData = nullptr;
            PKAssetHeader* header;
        };
    };

    struct PKAssetStream
    {
        void* stream = nullptr;
        PKAssetHeader header;
    };

    constexpr const static char* PK_SHADER_ENTRY_POINT_NAME = "main";

    constexpr const static char* PK_SHADER_ATTRIB_LOGVERBOSE = "#pragma pk_log_verbose";
    constexpr const static char* PK_SHADER_ATTRIB_GENERATEDEBUGINFO = "#pragma pk_generate_debug_info";
    constexpr const static char* PK_SHADER_ATTRIB_ZWRITE = "#pragma pk_zwrite ";
    constexpr const static char* PK_SHADER_ATTRIB_ZTEST = "#pragma pk_ztest ";
    constexpr const static char* PK_SHADER_ATTRIB_BLENDCOLOR = "#pragma pk_blend_color ";
    constexpr const static char* PK_SHADER_ATTRIB_BLENDALPHA = "#pragma pk_blend_alpha ";
    constexpr const static char* PK_SHADER_ATTRIB_COLORMASK = "#pragma pk_color_mask ";
    constexpr const static char* PK_SHADER_ATTRIB_CULL = "#pragma pk_cull ";
    constexpr const static char* PK_SHADER_ATTRIB_OFFSET = "#pragma pk_offset ";
    constexpr const static char* PK_SHADER_ATTRIB_RASTERMODE = "#pragma pk_raster_mode ";
    constexpr const static char* PK_SHADER_ATTRIB_MULTI_COMPILE = "#pragma pk_multi_compile ";
    constexpr const static char* PK_SHADER_ATTRIB_MATERIAL_PROP = "#pragma pk_material_property ";
    constexpr const static char* PK_SHADER_ATTRIB_INSTANCING_PROP = "#pragma pk_enable_instancing";
    constexpr const static char* PK_SHADER_ATTRIB_INSTANCING_NOFRAG_PROP = "#pragma pk_disable_fragment_instancing";
    constexpr const static char* PK_SHADER_ATTRIB_PROGRAM = "#pragma pk_program ";
    constexpr const static char* PK_SHADER_ATTRIB_LOCAL_OPEN = "[pk_local(";
    constexpr const static char* PK_SHADER_ATTRIB_LOCAL_CLOSE = ")]";
    constexpr const static char* PK_SHADER_ATTRIB_NUMTHREADS_OPEN = "[pk_numthreads(";
    constexpr const static char* PK_SHADER_ATTRIB_NUMTHREADS_CLOSE = ")]";
    constexpr const static char* PK_SHADER_ATTRIB_ALIAS = "_pkalias";

    constexpr const static char* PK_SHADER_INSTANCING_TRANSFORMS = "pk_Instancing_Transforms";
    constexpr const static char* PK_SHADER_INSTANCING_INDICES = "pk_Instancing_Indices";
    constexpr const static char* PK_SHADER_INSTANCING_PROPERTIES = "pk_Instancing_Properties";
    constexpr const static char* PK_SHADER_INSTANCING_TEXTURES2D = "pk_Instancing_Textures2D";
    constexpr const static char* PK_SHADER_INSTANCING_TEXTURES3D = "pk_Instancing_Textures3D";
    constexpr const static char* PK_SHADER_INSTANCING_TEXTURESCUBE = "pk_Instancing_TexturesCube";
    constexpr const static char* PK_SHADER_SET_NAMES[4] =
    {
        "GLOBAL",
        "PASS",
        "SHADER",
        "DRAW"
    };

    constexpr const static char* PK_MESH_VS_POSITION = "in_POSITION";
    constexpr const static char* PK_MESH_VS_NORMAL = "in_NORMAL";
    constexpr const static char* PK_MESH_VS_TANGENT = "in_TANGENT";
    constexpr const static char* PK_MESH_VS_COLOR = "in_COLOR";
    constexpr const static char* PK_MESH_VS_TEXCOORD0 = "in_TEXCOORD0";
    constexpr const static char* PK_MESH_VS_TEXCOORD1 = "in_TEXCOORD1";
    constexpr const static char* PK_MESH_VS_TEXCOORD2 = "in_TEXCOORD2";
    constexpr const static char* PK_MESH_VS_TEXCOORD3 = "in_TEXCOORD3";

    constexpr static const uint32_t PK_MESHLET_MAX_VERTICES = 64u;
    constexpr static const uint32_t PK_MESHLET_MAX_TRIANGLES = 124u;
    constexpr static const float PK_MESHLET_CONE_WEIGHT = 0.9f;
    constexpr static const float PK_MESHLET_LOD_MAX_ERROR = 65504.0f;
 
    constexpr static const float PK_FONT_MSDF_UNIT = 4.0f;


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

    enum class PKTextureType : uint8_t
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        Cubemap,
        CubemapArray,
    };

    enum class PKTextureFormat : uint8_t
    {
        Invalid = 0,
        R8,
        R8_SNORM,
        R8UI,
        R8I,
        Stencil8,
        R16F,
        R16UI,
        R16I,
        RG8,
        RG8_SNORM,
        RG8UI,
        RG8I,
        RGB565,
        RGB9E5,
        RGB5A1,
        RGBA4,
        Depth16,
        RGB8,
        RGB8_SRGB,
        RGB8_SNORM,
        RGB8UI,
        RGB8I,
        R32F,
        R32UI,
        R32I,
        RG16F,
        RG16UI,
        RG16I,
        B10G11R11UF,
        RGBA8,
        RGBA8_SRGB,
        RGBA8_SNORM,
        BGRA8,
        BGRA8_SRGB,
        RGB10A2,
        RGBA8UI,
        RGBA8I,
        Depth32F,
        Depth24_Stencil8,
        Depth32F_Stencil8,
        RGB16F,
        RGB16UI,
        RGB16I,
        RG32F,
        RG32UI,
        RG32I,
        RGBA16,
        RGBA16F,
        RGBA16UI,
        RGBA16I,
        RGB32F,
        RGB32UI,
        RGB32I,
        RGBA32F,
        RGBA32UI,
        RGBA32I,
        RGBA64UI,

        // compressed formats
        BC1_RGB,
        BC1_RGBA,
        BC1_SRGB,
        BC1_SRGBA,
        BC4,
        BC2_RGBA,
        BC2_SRGBA,
        BC3_RGBA,
        BC3_SRGBA,
        BC6H_RGBUF,
        BC6H_RGBF,
        BC7_UNORM,
    };

    enum class PKFilterMode : uint8_t
    {
        Point,
        Bilinear,
        Trilinear,
        Bicubic
    };

    enum class PKWrapMode : uint8_t
    {
        Clamp,
        Repeat,
        Mirror,
        MirrorOnce,
        Border
    };

    enum class PKBorderColor : uint8_t
    {
        FloatClear,
        IntClear,
        FloatBlack,
        IntBlack,
        FloatWhite,
        IntWhite
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
        AccelerationStructure
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

    enum class PKColorMask : uint8_t
    {
        NONE = 0u,
        R = 0x1u,
        G = 0x2u,
        B = 0x4u,
        A = 0x8u,
        RG = R | G,
        RGB = RG | B,
        RGBA = RGB | A,
    };

    enum class PKRasterMode : uint8_t
    {
        Default,
        OverEstimate,
        UnderEstimate,
    };

    // This is not an asset class. use this class to provide draw infos to instancing shaders
    // Packed as uint4
    struct PKDrawInfo
    {
        uint16_t material;
        uint16_t uniformScale;
        uint32_t transform;
        uint32_t submesh;
        uint32_t userdata;
    };



    // Shader asset types
    struct alignas(4) PKVertexInputAttribute
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
        PKColorMask colorMask = PKColorMask::RGBA;                  // 22 bytes
        uint8_t zwrite = 0x0;                                       // 24 bytes
        float zoffsets[3] = { 0, 0, 0 };                            // 36 bytes
    };

    struct alignas(4) PKShaderVariant
    {
        uint32_t descriptorCount;                                           // 4 bytes
        uint32_t constantVariableCount;                                     // 8 bytes
        uint32_t vertexAttributeCount;                                      // 12 bytes
        uint32_t groupSize[4]{};                                            // 28 bytes
        RelativePtr<PKDescriptor> descriptors;                              // 32 bytes
        RelativePtr<PKConstantVariable> constantVariables;                  // 36 bytes
        RelativePtr<PKVertexInputAttribute> vertexAttributes;               // 38 bytes
        uint32_t sprivSizes[(int)PKShaderStage::MaxCount];                  // 92 bytes
        RelativePtr<void> sprivBuffers[(int)PKShaderStage::MaxCount];       // 144 bytes
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


    // Mesh asset types
    // packed as uint4
    struct alignas(4) PKMeshletVertex
    {
        uint32_t posxy;      // unorm16     position.xy
        uint16_t posz;       // unorm16     position.z
        uint16_t tsign_color; // 1b tangent sign, rbg5 unorm color
        uint32_t texcoord;   // half2       texcoord
        uint32_t rotation;   // r10g10b10a2 quaternion
    };

    // packed as 3x uint4
    struct alignas(4) PKMeshlet
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

        // @TODO Bad packing. figure out how to fit this into 2x uint4
        // Cone data could be made redundant by better culling.
        uint16_t lodCenterErrorCurrent[4];   // 40 bytes half
        uint16_t lodCenterErrorParent[4];    // 48 bytes half
    };

    // packed as 2x uint4
    struct alignas(4) PKMeshletSubmesh
    {
        float bbmin[3];         // 12 bytes
        uint32_t firstMeshlet;  // 16 bytes
        float bbmax[3];         // 28 bytes
        uint32_t meshletCount;  // 32 bytes
    };

    struct alignas(4) PKMeshletMesh
    {
        uint32_t triangleCount;                  // 4 bytes
        uint32_t vertexCount;                    // 8 bytes
        uint32_t submeshCount;                   // 12 bytes
        uint32_t meshletCount;                   // 16 bytes
        RelativePtr<PKMeshlet> meshlets;         // 20 bytes
        RelativePtr<PKMeshletSubmesh> submeshes; // 24 bytes
        RelativePtr<PKMeshletVertex> vertices;   // 28 bytes
        RelativePtr<uint8_t> indices;            // 32 bytes
    };

    struct alignas(4) PKVertexAttribute
    {
        char name[PK_ASSET_NAME_MAX_LENGTH]; // 64 bytes
        PKElementType type;                  // 66 bytes
        uint16_t size = 0;                   // 68 bytes
        uint16_t offset = 0;                 // 70 bytes
        uint16_t stream = 0;                 // 72 bytes
    };

    struct alignas(4) PKSubmesh
    {
        uint32_t firstIndex; // 4 bytes
        uint32_t indexCount; // 8 bytes
        float bbmin[3]{};    // 20 bytes
        float bbmax[3]{};    // 32 bytes
    };

    struct alignas(4) PKMesh
    {
        uint32_t indexCount;                                // 4 bytes
        uint32_t vertexCount;                               // 8 bytes
        uint32_t submeshCount;                              // 12 bytes
        uint32_t vertexAttributeCount;                      // 16 bytes
        RelativePtr<PKVertexAttribute> vertexAttributes;    // 20 bytes
        RelativePtr<PKSubmesh> submeshes;                   // 24 bytes
        RelativePtr<void> vertexBuffer;                     // 28 bytes
        RelativePtr<void> indexBuffer;                      // 32 bytes
        RelativePtr<PKMeshletMesh> meshletMesh;             // 36 bytes
        PKElementType indexType;                            // 38 bytes

        uint16_t __padding = 0u;                            // 40 bytes
    };


    // Font asset types
    struct alignas(4) PKFontCharacter
    {
        float advance;          // 4 bytes
        float rect[4]{};        // 20 bytes
        uint16_t texrect[4]{};  // 28 bytes
        uint16_t unicode;       // 30 bytes
        uint16_t isWhiteSpace;  // 32 bytes
    };

    // Font data is rgba8 raw texture data
    // Use corresponding format when creating gpu textures
    struct alignas(4) PKFont
    {
        uint32_t characterCount;                 // 4 bytes
        uint16_t atlasResolution[2];             // 8 bytes
        uint32_t atlasDataSize;                  // 12 bytes
        RelativePtr<PKFontCharacter> characters; // 16 bytes
        RelativePtr<void> atlasData;             // 20 bytes
    };


    // Texture asset type
    struct alignas(4) PKTexture
    {
        RelativePtr<void> data;             // 4 bytes
        RelativePtr<uint32_t> levelOffsets; // 8 bytes
        uint32_t dataSize;                  // 12 bytes
        float anisotropy;                   // 16 bytes
        uint16_t resolution[3];             // 22 bytes
        uint16_t levels;                    // 24 bytes
        uint16_t layers;                    // 26 bytes
        PKTextureFormat format;             // 27 bytes
        PKTextureType type;                 // 28 bytes
        PKFilterMode filterMin;             // 29 bytes
        PKFilterMode filterMag;             // 30 bytes
        PKWrapMode wrap[3];                 // 33 bytes
        PKBorderColor borderColor;          // 34 bytes
        uint8_t __padding0;                 // 36 bytes
    };


    uint32_t PKElementTypeToSize(PKElementType type);
    uint32_t PKElementTypeToAlignment(PKElementType type);
    bool PKElementTypeIsResourceHandle(PKElementType type);

    PKAssetType StringToPKAssetType(const char* str);
    PKElementType StringToPKElementType(const char* str);
    PKTextureType StringToPKTextureType(const char* str);
    PKTextureFormat StringToPKTextureFormat(const char* str);
    PKFilterMode StringToPKFilterMode(const char* str);
    PKWrapMode StringToPKWrapMode(const char* str);
    PKBorderColor StringToPKBorderColor(const char* str);
    PKShaderStage StringToPKShaderStage(const char* str);
    PKDescriptorType StringToPKDescriptorType(const char* str);
    PKComparison StringToPKComparison(const char* str);
    PKCullMode StringToPKCullMode(const char* str);
    PKBlendFactor StringToPKBlendFactor(const char* str);
    PKBlendOp StringToPKBlendOp(const char* str);
    PKColorMask StringToPKColorMask(const char* str);
    PKRasterMode StringToPKRasterMode(const char* str);

    PKDrawInfo PackPKDrawInfo(uint16_t material, float uniformScale, uint32_t transform, uint32_t submesh, uint32_t userdata);

    PKMeshletVertex PackPKMeshletVertex(const float* pPosition, 
                        const float* pTexcoord, 
                        const float* pNormal, 
                        const float* pTangent, 
                        const float* pColor,
                        const float* submeshbbmin, 
                        const float* submeshbbmax);

    PKMeshlet PackPKMeshlet(uint32_t vertexFirst, 
                          uint32_t triangleFirst, 
                          uint32_t vertexCount, 
                          uint32_t triangleCount,
                          const int8_t* coneAxis,
                          int8_t coneCutoff,
                          const float* coneApex,
                          const float* center,
                          const float* extents,

                          const float* lodCenterCurrent,
                          float lodErrorCurrent,
                          const float* lodCenterParent,
                          float lodErrorParent);
}
