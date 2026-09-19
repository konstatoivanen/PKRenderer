#include "PKAsset.h"
#include <string.h>
#include <math.h>

namespace PKAssets
{
    const static uint8_t PKElementType_SIZES[] =
    {
        0u,  /*Invalid*/
        1u * 4u, /*Float*/
        2u * 4u, /*Float2*/
        3u * 4u, /*Float3*/
        4u * 4u, /*Float4*/
        
        1u * 8u, /*Double*/
        2u * 8u, /*Double2*/
        3u * 8u, /*Double3*/
        4u * 8u, /*Double4*/
        
        1u * 2u, /*Half*/
        2u * 2u, /*Half2*/
        3u * 2u, /*Half3*/
        4u * 2u, /*Half4*/
        
        1u * 4u, /*Int*/
        2u * 4u, /*Int2*/
        3u * 4u, /*Int3*/
        4u * 4u, /*Int4*/
        
        1u * 4u, /*Uint*/
        2u * 4u, /*Uint2*/
        3u * 4u, /*Uint3*/
        4u * 4u, /*Uint4*/
        
        1u * 2u, /*Short*/
        2u * 2u, /*Short2*/
        3u * 2u, /*Short3*/
        4u * 2u, /*Short4*/
        
        1u * 2u, /*Ushort*/
        2u * 2u, /*Ushort2*/
        3u * 2u, /*Ushort3*/
        4u * 2u, /*Ushort4*/
        
        1u * 8u, /*Long*/
        2u * 8u, /*Long2*/
        3u * 8u, /*Long3*/
        4u * 8u, /*Long4*/
        
        1u * 8u, /*Ulong*/
        2u * 8u, /*Ulong2*/
        3u * 8u, /*Ulong3*/
        4u * 8u, /*Ulong4*/

        2u * 2u * 4u,  /*Float2x2*/
        3u * 3u * 4u,  /*Float3x3*/
        4u * 4u * 4u,  /*Float4x4*/
        3u * 4u * 4u,  /*Float3x4*/
        
        2u * 2u * 8u,  /*Double2x2*/
        3u * 3u * 8u,  /*Double3x3*/
        4u * 4u * 8u,  /*Double4x4*/
        
        2u * 2u * 2u,  /*Half2x2*/
        3u * 3u * 2u,  /*Half3x3*/
        4u * 4u * 2u,  /*Half4x4*/
        
        4u, /*Texture2D*/
        4u, /*Texture3D*/
        4u, /*TextureCube*/
        1u  /*Keyword*/
    };

    const static PKElementType PKElementType_SCALAR[] =
    {
        PKElementType::Invalid,
        PKElementType::Float,
        PKElementType::Float,
        PKElementType::Float,
        PKElementType::Float,
        PKElementType::Double,
        PKElementType::Double,
        PKElementType::Double,
        PKElementType::Double,
        PKElementType::Half,
        PKElementType::Half,
        PKElementType::Half,
        PKElementType::Half,
        PKElementType::Int,
        PKElementType::Int,
        PKElementType::Int,
        PKElementType::Int,
        PKElementType::Uint,
        PKElementType::Uint,
        PKElementType::Uint,
        PKElementType::Uint,
        PKElementType::Short,
        PKElementType::Short,
        PKElementType::Short,
        PKElementType::Short,
        PKElementType::Ushort,
        PKElementType::Ushort,
        PKElementType::Ushort,
        PKElementType::Ushort,
        PKElementType::Long,
        PKElementType::Long,
        PKElementType::Long,
        PKElementType::Long,
        PKElementType::Ulong,
        PKElementType::Ulong,
        PKElementType::Ulong,
        PKElementType::Ulong,
        PKElementType::Float,
        PKElementType::Float,
        PKElementType::Float,
        PKElementType::Float,
        PKElementType::Double,
        PKElementType::Double,
        PKElementType::Double,
        PKElementType::Half,
        PKElementType::Half,
        PKElementType::Half,
        PKElementType::Texture2D,
        PKElementType::Texture3D,
        PKElementType::TextureCube,
        PKElementType::Keyword
    };

    uint32_t PKElementTypeToSize(PKElementType type)
    {
        return PKElementType_SIZES[(uint32_t)type];
    }

    PKElementType PKElementTypeToScalar(PKElementType type)
    {
        return PKElementType_SCALAR[(uint32_t)type];
    }

    uint32_t PKElementTypeToComponents(PKElementType type)
    {
        return PKElementType_SIZES[(uint32_t)type] / PKElementType_SIZES[(uint32_t)PKElementType_SCALAR[(uint32_t)type]];
    }

    uint32_t PKElementTypeToAlignment(PKElementType type)
    {
        const auto size = PKElementType_SIZES[(uint32_t)type];
        const auto scalarSize = PKElementType_SIZES[(uint32_t)PKElementType_SCALAR[(uint32_t)type]];
        auto components = (size / (scalarSize ? scalarSize : 1u)) - 1u;
        components |= components >> 1u;
        components |= components >> 2u;
        components = (components & 0x3u) + 1u;
        return scalarSize * components;
    }


    bool PKElementTypeIsResourceHandle(PKElementType type)
    {
        switch (type)
        {
            case PKElementType::Texture2D:
            case PKElementType::Texture3D:
            case PKElementType::TextureCube: return true;
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
