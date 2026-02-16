#include "PKAsset.h"
#include <string>

namespace PKAssets
{
    const static char* PKAssetType_NAMES[] =
    {
        "Invalid",
        "Shader",
        "Mesh",
        "Font",
        "Texture"
    };


    const static char* PKElementType_NAMES[] =
    {
        "Invalid",
        "float",
        "float2",
        "float3",
        "float4",
        "double",
        "double2",
        "double3",
        "double4",
        "half",
        "half2",
        "half3",
        "half4",
        "int",
        "int2",
        "int3",
        "int4",
        "uint",
        "uint2",
        "uint3",
        "uint4",
        "short",
        "short2",
        "short3",
        "short4",
        "ushort",
        "ushort2",
        "ushort3",
        "ushort4",
        "long",
        "long2",
        "long3",
        "long4",
        "ulong",
        "ulong2",
        "ulong3",
        "ulong4",
        "float2x2",
        "float3x3",
        "float4x4",
        "float3x4",
        "double2x2",
        "double3x3",
        "double4x4",
        "half2x2",
        "half3x3",
        "half4x4",
        "texture2D",
        "texture3D",
        "textureCube",
        "keyword",
    };

    const static uint32_t PKElementType_SIZES[] =
    {
        0u,         /*Invalid*/
        4u,         /*Float*/
        4u * 2u,    /*Float2*/
        4u * 3u,    /*Float3*/
        4u * 4u,    /*Float4*/
        8u,         /*Double*/
        8u * 2u,    /*Double2*/
        8u * 3u,    /*Double3*/
        8u * 4u,    /*Double4*/
        2u,         /*Half*/
        2u * 2u,    /*Half2*/
        2u * 3u,    /*Half3*/
        2u * 4u,    /*Half4*/
        4u,         /*Int*/
        4u * 2u,    /*Int2*/
        4u * 3u,    /*Int3*/
        4u * 4u,    /*Int4*/
        4u,         /*Uint*/
        4u * 2u,    /*Uint2*/
        4u * 3u,    /*Uint3*/
        4u * 4u,    /*Uint4*/
        2u,         /*Short*/
        2u * 2u,    /*Short2*/
        2u * 3u,    /*Short3*/
        2u * 4u,    /*Short4*/
        2u,         /*Ushort*/
        2u * 2u,    /*Ushort2*/
        2u * 3u,    /*Ushort3*/
        2u * 4u,    /*Ushort4*/
        8u,         /*Long*/
        8u * 2u,    /*Long2*/
        8u * 3u,    /*Long3*/
        8u * 4u,    /*Long4*/
        8u,         /*Ulong*/
        8u * 2u,    /*Ulong2*/
        8u * 3u,    /*Ulong3*/
        8u * 4u,    /*Ulong4*/
        4 * 2 * 2,  /*Float2x2*/
        4 * 3 * 3,  /*Float3x3*/
        4 * 4 * 4,  /*Float4x4*/
        3 * 4 * 4,  /*Float3x4*/
        8 * 2 * 2,  /*Double2x2*/
        8 * 3 * 3,  /*Double3x3*/
        8 * 4 * 4,  /*Double4x4*/
        2 * 2 * 2,  /*Half2x2*/
        2 * 3 * 3,  /*Half3x3*/
        2 * 4 * 4,  /*Half4x4*/
        4u, /*Texture2DHandle*/
        4u, /*Texture3DHandle*/
        4u, /*TextureCubeHandle*/
        1u  /*Keyword*/
    };

    const static uint32_t PKElementType_ALIGNMENTS[] =
    {
        0u,         /*Invalid*/
        4u,         /*Float*/
        4u * 2u,    /*Float2*/
        4u * 4u,    /*Float3*/
        4u * 4u,    /*Float4*/
        8u,         /*Double*/
        8u * 2u,    /*Double2*/
        8u * 4u,    /*Double3*/
        8u * 4u,    /*Double4*/
        2u,         /*Half*/
        2u * 2u,    /*Half2*/
        2u * 4u,    /*Half3*/
        2u * 4u,    /*Half4*/
        4u,         /*Int*/
        4u * 2u,    /*Int2*/
        4u * 4u,    /*Int3*/
        4u * 4u,    /*Int4*/
        4u,         /*Uint*/
        4u * 2u,    /*Uint2*/
        4u * 4u,    /*Uint3*/
        4u * 4u,    /*Uint4*/
        2u,         /*Short*/
        2u * 2u,    /*Short2*/
        2u * 4u,    /*Short3*/
        2u * 4u,    /*Short4*/
        2u,         /*Ushort*/
        2u * 2u,    /*Ushort2*/
        2u * 4u,    /*Ushort3*/
        2u * 4u,    /*Ushort4*/
        8u,         /*Long*/
        8u * 2u,    /*Long2*/
        8u * 4u,    /*Long3*/
        8u * 4u,    /*Long4*/
        8u,         /*Ulong*/
        8u * 2u,    /*Ulong2*/
        8u * 4u,    /*Ulong3*/
        8u * 4u,    /*Ulong4*/
        4u * 2u,    /*Float2x2*/
        4u * 4u,    /*Float3x3*/
        4u * 4u,    /*Float4x4*/
        3u * 4u,    /*Float3x4*/
        8u * 2u,    /*Double2x2*/
        8u * 4u,    /*Double3x3*/
        8u * 4u,    /*Double4x4*/
        2u * 2u,    /*Half2x2*/
        2u * 4u,    /*Half3x3*/
        2u * 4u,    /*Half4x4*/
        4u,         /*Texture2DHandle*/
        4u,         /*Texture3DHandle*/
        4u,         /*TextureCubeHandle*/
        1u          /*Keyword*/
    };

    const static char* PKTextureType_NAMES[] =
    {
        "Texture2D",
        "Texture2DArray",
        "Texture3D",
        "Cubemap",
        "CubemapArray",
    };

    const static char* PKTextureFormat_NAMES[] =
    {
        "Invalid",
        "R8",
        "R8_SNORM",
        "R8UI",
        "R8I",
        "Stencil8",
        "R16F",
        "R16UI",
        "R16I",
        "RG8",
        "RG8_SNORM",
        "RG8UI",
        "RG8I",
        "RGB565",
        "RGB9E5",
        "RGB5A1",
        "RGBA4",
        "Depth16",
        "RGB8",
        "RGB8_SRGB",
        "RGB8_SNORM",
        "RGB8UI",
        "RGB8I",
        "R32F",
        "R32UI",
        "R32I",
        "RG16F",
        "RG16UI",
        "RG16I",
        "B10G11R11UF",
        "RGBA8",
        "RGBA8_SRGB",
        "RGBA8_SNORM",
        "BGRA8",
        "BGRA8_SRGB",
        "RGB10A2",
        "RGBA8UI",
        "RGBA8I",
        "Depth32F",
        "Depth24_Stencil8",
        "Depth32F_Stencil8",
        "RGB16F",
        "RGB16UI",
        "RGB16I",
        "RG32F",
        "RG32UI",
        "RG32I",
        "RGBA16",
        "RGBA16F",
        "RGBA16UI",
        "RGBA16I",
        "RGB32F",
        "RGB32UI",
        "RGB32I",
        "RGBA32F",
        "RGBA32UI",
        "RGBA32I",
        "RGBA64UI",
        "BC1_RGB",
        "BC1_RGBA",
        "BC1_SRGB",
        "BC1_SRGBA",
        "BC4",
        "BC2_RGBA",
        "BC2_SRGBA",
        "BC3_RGBA",
        "BC3_SRGBA",
        "BC6H_RGBUF",
        "BC6H_RGBF",
        "BC7_UNORM",
    };

    const static char* PKFilterMode_NAMES[] =
    {
        "Point",
        "Bilinear",
        "Trilinear",
        "Bicubic"
    };

    const static char* PKWrapMode_NAMES[] =
    {
        "Clamp",
        "Repeat",
        "Mirror",
        "MirrorOnce",
        "Border"
    };

    const static char* PKBorderColor_NAMES[] =
    {
        "FloatClear",
        "IntClear",
        "FloatBlack",
        "IntBlack",
        "FloatWhite",
        "IntWhite"
    };

    // Special names for these. Follow defines
    const static char* PKShaderStage_NAMES[] =
    {
        "SHADER_STAGE_VERTEX",
        "SHADER_STAGE_TESSELATION_CONTROL",
        "SHADER_STAGE_TESSELATION_EVALUATE",
        "SHADER_STAGE_GEOMETRY",
        "SHADER_STAGE_FRAGMENT",
        "SHADER_STAGE_COMPUTE",
        "SHADER_STAGE_MESH_TASK",
        "SHADER_STAGE_MESH_ASSEMBLY",
        "SHADER_STAGE_RAY_GENERATION",
        "SHADER_STAGE_RAY_MISS",
        "SHADER_STAGE_RAY_CLOSEST_HIT",
        "SHADER_STAGE_RAY_ANY_HIT",
        "SHADER_STAGE_RAY_INTERSECTION",
        "SHADER_STAGE_INVALID"
    };

    const static char* PKDescriptorType_NAMES[] =
    {
        "Invalid",
        "Sampler",
        "SamplerTexture",
        "Texture",
        "Image",
        "ConstantBuffer",
        "StorageBuffer",
        "DynamicConstantBuffer",
        "DynamicStorageBuffer",
        "InputAttachment",
        "AccelerationStructure"
    };

    const static char* PKComparison_NAMES[] =
    {
        "Off",
        "Never",
        "Less",
        "Equal",
        "LessEqual",
        "Greater",
        "NotEqual",
        "GreaterEqual",
        "Always",
    };

    const static char* PKCullMode_NAMES[] =
    {
        "Off",
        "Front",
        "Back"
    };

    const static char* PKBlendFactor_NAMES[] =
    {
        "None",
        "One",
        "Zero",
        "SrcColor",
        "SrcAlpha",
        "DstColor",
        "DstAlpha",
        "OneMinusSrcColor",
        "OneMinusSrcAlpha",
        "OneMinusDstColor",
        "OneMinusDstAlpha",
        "ConstColor",
        "OneMinusConstColor",
        "ConstAlpha",
        "OneMinusConstAlpha",
    };

    const static char* PKBlendOp_NAMES[] =
    {
        "None",
        "Add",
        "Subtract",
        "ReverseSubtract",
        "Min",
        "Max",
    };

    const static char* PKRasterMode_NAMES[] =
    {
        "Default",
        "OverEstimate",
        "UnderEstimate",
    };

    template<size_t size>
    uint32_t FindEnumIndexFromString(const char* (&arr)[size], const char* str, uint32_t fallback)
    {
        auto length = str ? strnlen(str, 64) : 0u;

        for (auto i = 0u; length && i < size; ++i)
        {
            if (strncmp(arr[i], str, length) == 0)
            {
                return i;
            }
        }

        return fallback;
    }

    template<size_t size>
    const char* FindStringFromEnum(const char* (&arr)[size], const uint32_t value, uint32_t fallback)
    {
        if (value >= size)
        {
            return arr[fallback];
        }

        return arr[value];
    }

    #define DECLARE_STRING_TO_ENUM(TType, TFallback) TType StringTo##TType(const char* str) { return (TType)FindEnumIndexFromString(TType##_NAMES, str, (uint32_t)TFallback); }
    #define DECLARE_ENUM_TO_STRING(TType, TFallback) const char* TType##ToString(TType value) { return FindStringFromEnum(TType##_NAMES, (uint32_t)value, (uint32_t)TFallback); }

    DECLARE_STRING_TO_ENUM(PKAssetType, PKAssetType::Invalid)
    DECLARE_STRING_TO_ENUM(PKElementType, PKElementType::Invalid)
    DECLARE_STRING_TO_ENUM(PKTextureType, PKTextureType::Texture2D)
    DECLARE_STRING_TO_ENUM(PKTextureFormat, PKTextureFormat::Invalid)
    DECLARE_STRING_TO_ENUM(PKFilterMode, PKFilterMode::Point)
    DECLARE_STRING_TO_ENUM(PKWrapMode, PKWrapMode::Clamp)
    DECLARE_STRING_TO_ENUM(PKBorderColor, PKBorderColor::FloatClear)
    DECLARE_STRING_TO_ENUM(PKShaderStage, PKShaderStage::MaxCount)
    DECLARE_STRING_TO_ENUM(PKDescriptorType, PKDescriptorType::Invalid)
    DECLARE_STRING_TO_ENUM(PKComparison, PKComparison::Off)
    DECLARE_STRING_TO_ENUM(PKCullMode, PKCullMode::Off)
    DECLARE_STRING_TO_ENUM(PKBlendFactor, PKBlendFactor::None)
    DECLARE_STRING_TO_ENUM(PKBlendOp, PKBlendOp::None)
    DECLARE_STRING_TO_ENUM(PKRasterMode, PKRasterMode::Default)

    DECLARE_ENUM_TO_STRING(PKAssetType, PKAssetType::Invalid)
    DECLARE_ENUM_TO_STRING(PKElementType, PKElementType::Invalid)
    DECLARE_ENUM_TO_STRING(PKTextureType, PKTextureType::Texture2D)
    DECLARE_ENUM_TO_STRING(PKTextureFormat, PKTextureFormat::Invalid)
    DECLARE_ENUM_TO_STRING(PKFilterMode, PKFilterMode::Point)
    DECLARE_ENUM_TO_STRING(PKWrapMode, PKWrapMode::Clamp)
    DECLARE_ENUM_TO_STRING(PKBorderColor, PKBorderColor::FloatClear)
    DECLARE_ENUM_TO_STRING(PKShaderStage, PKShaderStage::MaxCount)
    DECLARE_ENUM_TO_STRING(PKDescriptorType, PKDescriptorType::Invalid)
    DECLARE_ENUM_TO_STRING(PKComparison, PKComparison::Off)
    DECLARE_ENUM_TO_STRING(PKCullMode, PKCullMode::Off)
    DECLARE_ENUM_TO_STRING(PKBlendFactor, PKBlendFactor::None)
    DECLARE_ENUM_TO_STRING(PKBlendOp, PKBlendOp::None)
    DECLARE_ENUM_TO_STRING(PKRasterMode, PKRasterMode::Default)

    #undef DECLARE_STRING_TO_ENUM
    #undef DECLARE_ENUM_TO_STRING

    PKColorMask StringToPKColorMask(const char* str)
    {
        auto length = str ? strnlen(str, 4) : 0;

        if (length == 0)
        {
            return PKColorMask::RGBA;
        }

        uint8_t mask = (uint8_t)PKColorMask::NONE;

        for (auto i = 0u; i < length; ++i)
        {
            switch (str[i])
            {
                case 'X':
                case 'R': mask = mask | (uint8_t)PKColorMask::R; break;
                case 'Y':
                case 'G': mask = mask | (uint8_t)PKColorMask::G; break;
                case 'Z':
                case 'B': mask = mask | (uint8_t)PKColorMask::B; break;
                case 'W':
                case 'A': mask = mask | (uint8_t)PKColorMask::A; break;
                default: break;
            }
        }

        return (PKColorMask)mask;
    }

    uint32_t PKElementTypeToSize(PKElementType type) { return PKElementType_SIZES[(uint32_t)type]; }
    uint32_t PKElementTypeToAlignment(PKElementType type) { return PKElementType_ALIGNMENTS[(uint32_t)type]; }

    bool PKElementTypeIsResourceHandle(PKElementType type)
    {
        switch (type)
        {
            case PKElementType::Texture2DHandle:
            case PKElementType::Texture3DHandle:
            case PKElementType::TextureCubeHandle: return true;
            default: return false;
        }
    }


    static uint16_t PackHalf(float v)
    {
        if (v < -65536.0f)
        {
            v = -65536.0f;
        }

        if (v > 65536.0f)
        {
            v = 65536.0f;
        }

        v *= 1.925930e-34f;
        int32_t i = *(int*)&v;
        uint32_t ui = (uint32_t)i;
        return ((i >> 16) & (int)0xffff8000) | ((int)(ui >> 13));
    }

    static uint16_t PackUnorm16(float v)
    {
        auto i = (int32_t)roundf(v * 65535.0f);
        if (i < 0) { i = 0; }
        if (i > 65535) { i = 65535; }
        return (uint16_t)(i & 0xFFFFu);
    }

    static uint32_t PackUnorm10(float v)
    {
        auto i = (int32_t)roundf(v * 1023.0f);
        if (i < 0) { i = 0; }
        if (i > 1023) { i = 1023; }
        return (uint32_t)(i & 0x3FFu);
    }

    static uint32_t PackUnorm5(float v)
    {
        auto i = (int32_t)roundf(v * 31.0f);
        if (i < 0) { i = 0; }
        if (i > 31) { i = 31; }
        return (uint32_t)(i & 0x1Fu);
    }

    static float abs(float v) { return v < 0.0f ? -v : v; }

    static uint32_t EncodeQuaternion(const float* n, const float* t)
    {
        float m[3][3];
        m[0][0] = t[0];
        m[0][1] = t[1];
        m[0][2] = t[2];
        m[1][0] = n[1] * t[2] - t[1] * n[2];
        m[1][1] = n[2] * t[0] - t[2] * n[0];
        m[1][2] = n[0] * t[1] - t[0] * n[1];
        m[2][0] = n[0];
        m[2][1] = n[1];
        m[2][2] = n[2];

        float lengths[4]
        {
            m[0][0] - m[1][1] - m[2][2],
            m[1][1] - m[0][0] - m[2][2],
            m[2][2] - m[0][0] - m[1][1],
            m[0][0] + m[1][1] + m[2][2]
        };

        auto index = 0u;
        auto length = lengths[0];

        for (auto i = 1u; i < 4u; ++i)
        {
            if (lengths[i] > length)
            {
                length = lengths[i];
                index = i;
            }
        }

        float quat[4];
        quat[index] = sqrtf(length + 1.0f) * 0.5f;

        switch (index)
        {
            case 0:
                quat[1] = (m[0][1] + m[1][0]) * (0.25f / quat[index]);
                quat[2] = (m[2][0] + m[0][2]) * (0.25f / quat[index]);
                quat[3] = (m[1][2] - m[2][1]) * (0.25f / quat[index]);
                break;
            case 1:
                quat[0] = (m[0][1] + m[1][0]) * (0.25f / quat[index]);
                quat[2] = (m[1][2] + m[2][1]) * (0.25f / quat[index]);
                quat[3] = (m[2][0] - m[0][2]) * (0.25f / quat[index]);
                break;
            case 2:
                quat[0] = (m[2][0] + m[0][2]) * (0.25f / quat[index]);
                quat[1] = (m[1][2] + m[2][1]) * (0.25f / quat[index]);
                quat[3] = (m[0][1] - m[1][0]) * (0.25f / quat[index]);
                break;
            case 3:
                quat[0] = (m[1][2] - m[2][1]) * (0.25f / quat[index]);
                quat[1] = (m[2][0] - m[0][2]) * (0.25f / quat[index]);
                quat[2] = (m[0][1] - m[1][0]) * (0.25f / quat[index]);
                break;
        }

        // Normalize
        length = quat[0];
        index = 0u;

        for (auto i = 1u; i < 4u; ++i)
        {
            if (abs(quat[i]) > abs(length))
            {
                length = quat[i];
                index = i;
            }
        }

        uint32_t quantized[3];

        for (auto i = 0u; i < 3u; ++i)
        {
            auto e = quat[(i + index + 1u) % 4u] / length;
            quantized[i] = PackUnorm10(e * 0.5f + 0.5f);
        }

        return quantized[0] | (quantized[1] << 10u) | (quantized[2] << 20u) | ((index & 0x3u) << 30u);
    }


    PKDrawInfo PackPKDrawInfo(uint16_t material, float uniformScale, uint32_t transform, uint32_t submesh, uint32_t userdata)
    {
        PKDrawInfo info;
        info.material = material;
        info.uniformScale = PackHalf(uniformScale);
        info.transform = transform;
        info.submesh = submesh;
        info.userdata = userdata;
        return info;
    }

    PKMeshletVertex PackPKMeshletVertex(const float* pPosition,
                        const float* pTexcoord,
                        const float* pNormal,
                        const float* pTangent,
                        const float* pColor,
                        const float* submeshbbmin,
                        const float* submeshbbmax)
    {
        PKMeshletVertex vertex = { 0u, 0u, 0u, 0u, 0u };

        uint32_t unormPositions[3] =
        {
            PackUnorm16((pPosition[0] - submeshbbmin[0]) / (submeshbbmax[0] - submeshbbmin[0])),
            PackUnorm16((pPosition[1] - submeshbbmin[1]) / (submeshbbmax[1] - submeshbbmin[1])),
            PackUnorm16((pPosition[2] - submeshbbmin[2]) / (submeshbbmax[2] - submeshbbmin[2]))
        };

        vertex.posxy = unormPositions[0] | (unormPositions[1] << 16u);
        vertex.posz = unormPositions[2];
        vertex.tsign_color = 0u;

        if (pTexcoord)
        {
            auto t0 = (uint32_t)PackHalf(pTexcoord[0]);
            auto t1 = (uint32_t)PackHalf(pTexcoord[1]);
            vertex.texcoord = t0 | (t1 << 16u);
        }

        if (pNormal && pTangent)
        {
            vertex.rotation = EncodeQuaternion(pNormal, pTangent);
            vertex.tsign_color |= (pTangent[3] < 0.0f ? 0u : 1u);
        }

        if (pColor)
        {
            vertex.tsign_color |= (uint16_t)(PackUnorm5(pColor[0]) << 1u);
            vertex.tsign_color |= (uint16_t)(PackUnorm5(pColor[1]) << 6u);
            vertex.tsign_color |= (uint16_t)(PackUnorm5(pColor[2]) << 11u);
        }

        return vertex;
    }
    
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
                          float lodErrorParent)
    {
        PKMeshlet meshlet{};
        meshlet.vertexFirst = vertexFirst;
        meshlet.triangleFirst = triangleFirst;
        meshlet.coneAxis[0] = coneAxis[0];
        meshlet.coneAxis[1] = coneAxis[1];
        meshlet.coneAxis[2] = coneAxis[2];
        meshlet.coneCutoff = coneCutoff;
        meshlet.vertexCount = (uint8_t)vertexCount;
        meshlet.triangleCount = (uint8_t)triangleCount;
        meshlet.coneApex[0] = PackHalf(coneApex[0]);
        meshlet.coneApex[1] = PackHalf(coneApex[1]);
        meshlet.coneApex[2] = PackHalf(coneApex[2]);
        meshlet.center[0] = PackHalf(center[0]);
        meshlet.center[1] = PackHalf(center[1]);
        meshlet.center[2] = PackHalf(center[2]);
        meshlet.extents[0] = PackHalf(extents[0]);
        meshlet.extents[1] = PackHalf(extents[1]);
        meshlet.extents[2] = PackHalf(extents[2]);

        meshlet.lodCenterErrorCurrent[0] = PackHalf(lodCenterCurrent[0]);
        meshlet.lodCenterErrorCurrent[1] = PackHalf(lodCenterCurrent[1]);
        meshlet.lodCenterErrorCurrent[2] = PackHalf(lodCenterCurrent[2]);
        meshlet.lodCenterErrorCurrent[3] = PackHalf(lodErrorCurrent);
        meshlet.lodCenterErrorParent[0] = PackHalf(lodCenterParent[0]);
        meshlet.lodCenterErrorParent[1] = PackHalf(lodCenterParent[1]);
        meshlet.lodCenterErrorParent[2] = PackHalf(lodCenterParent[2]);
        meshlet.lodCenterErrorParent[3] = PackHalf(lodErrorParent);

        return meshlet;
    }
}
