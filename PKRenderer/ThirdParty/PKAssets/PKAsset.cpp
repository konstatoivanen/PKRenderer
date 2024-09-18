#include "PKAsset.h"
#include <string>

namespace PKAssets
{
    PKElementType GetElementType(const char* string)
    {
        if (strcmp(string,"half") == 0)
        {
            return PKElementType::Half;
        }

        if (strcmp(string, "half2") == 0)
        {
            return PKElementType::Half2;
        }

        if (strcmp(string, "half3") == 0)
        {
            return PKElementType::Half3;
        }

        if (strcmp(string, "half4") == 0)
        {
            return PKElementType::Half4;
        }

        if (strcmp(string, "float") == 0)
        {
            return PKElementType::Float;
        }

        if (strcmp(string, "float2") == 0)
        {
            return PKElementType::Float2;
        }

        if (strcmp(string, "float3") == 0)
        {
            return PKElementType::Float3;
        }

        if (strcmp(string, "float4") == 0)
        {
            return PKElementType::Float4;
        }

        if (strcmp(string, "double") == 0)
        {
            return PKElementType::Double;
        }

        if (strcmp(string, "double2") == 0)
        {
            return PKElementType::Double2;
        }

        if (strcmp(string, "double3") == 0)
        {
            return PKElementType::Double3;
        }

        if (strcmp(string, "double4") == 0)
        {
            return PKElementType::Double4;
        }

        if (strcmp(string, "short") == 0)
        {
            return PKElementType::Short;
        }

        if (strcmp(string, "short2") == 0)
        {
            return PKElementType::Short2;
        }

        if (strcmp(string, "short3") == 0)
        {
            return PKElementType::Short3;
        }

        if (strcmp(string, "short4") == 0)
        {
            return PKElementType::Short4;
        }

        if (strcmp(string, "ushort") == 0)
        {
            return PKElementType::Ushort;
        }

        if (strcmp(string, "ushort2") == 0)
        {
            return PKElementType::Ushort2;
        }

        if (strcmp(string, "ushort3") == 0)
        {
            return PKElementType::Ushort3;
        }

        if (strcmp(string, "ushort4") == 0)
        {
            return PKElementType::Ushort4;
        }

        if (strcmp(string, "int") == 0)
        {
            return PKElementType::Int;
        }

        if (strcmp(string, "int2") == 0)
        {
            return PKElementType::Int2;
        }

        if (strcmp(string, "int3") == 0)
        {
            return PKElementType::Int3;
        }

        if (strcmp(string, "int4") == 0)
        {
            return PKElementType::Int4;
        }

        if (strcmp(string, "uint") == 0)
        {
            return PKElementType::Uint;
        }

        if (strcmp(string, "uint2") == 0)
        {
            return PKElementType::Uint2;
        }

        if (strcmp(string, "uint3") == 0)
        {
            return PKElementType::Uint3;
        }

        if (strcmp(string, "uint4") == 0)
        {
            return PKElementType::Uint4;
        }

        if (strcmp(string, "long") == 0)
        {
            return PKElementType::Long;
        }

        if (strcmp(string, "long2") == 0)
        {
            return PKElementType::Long2;
        }

        if (strcmp(string, "long3") == 0)
        {
            return PKElementType::Long3;
        }

        if (strcmp(string, "long4") == 0)
        {
            return PKElementType::Long4;
        }

        if (strcmp(string, "ulong") == 0)
        {
            return PKElementType::Ulong;
        }

        if (strcmp(string, "ulong2") == 0)
        {
            return PKElementType::Ulong2;
        }

        if (strcmp(string, "ulong3") == 0)
        {
            return PKElementType::Ulong3;
        }

        if (strcmp(string, "ulong4") == 0)
        {
            return PKElementType::Ulong4;
        }

        if (strcmp(string, "half2x2") == 0)
        {
            return PKElementType::Half2x2;
        }

        if (strcmp(string, "half3x3") == 0)
        {
            return PKElementType::Half3x3;
        }

        if (strcmp(string, "half4x4") == 0)
        {
            return PKElementType::Half4x4;
        }

        if (strcmp(string, "float2x2") == 0)
        {
            return PKElementType::Float2x2;
        }

        if (strcmp(string, "float3x3") == 0)
        {
            return PKElementType::Float3x3;
        }

        if (strcmp(string, "float4x4") == 0)
        {
            return PKElementType::Float4x4;
        }

        if (strcmp(string, "float3x4") == 0)
        {
            return PKElementType::Float3x4;
        }

        if (strcmp(string, "double2x2") == 0)
        {
            return PKElementType::Double2x2;
        }

        if (strcmp(string, "double3x3") == 0)
        {
            return PKElementType::Double3x3;
        }

        if (strcmp(string, "double4x4") == 0)
        {
            return PKElementType::Double4x4;
        }

        if (strcmp(string, "texture2D") == 0)
        {
            return PKElementType::Texture2DHandle;
        }

        if (strcmp(string, "texture3D") == 0)
        {
            return PKElementType::Texture3DHandle;
        }

        if (strcmp(string, "textureCube") == 0)
        {
            return PKElementType::TextureCubeHandle;
        }

        return PKElementType::Invalid;
    }

    uint32_t GetElementSize(PKElementType type)
    {
        switch (type)
        {
            case PKElementType::Float: return 4;
            case PKElementType::Float2: return 4 * 2;
            case PKElementType::Float3: return 4 * 3;
            case PKElementType::Float4: return 4 * 4;
            case PKElementType::Double: return 8;
            case PKElementType::Double2: return 8 * 2;
            case PKElementType::Double3: return 8 * 3;
            case PKElementType::Double4: return 8 * 4;
            case PKElementType::Half: return 2;
            case PKElementType::Half2: return 2 * 2;
            case PKElementType::Half3: return 2 * 3;
            case PKElementType::Half4: return 2 * 4;
            case PKElementType::Int: return 4;
            case PKElementType::Int2: return 4 * 2;
            case PKElementType::Int3: return 4 * 3;
            case PKElementType::Int4: return 4 * 4;
            case PKElementType::Uint: return 4;
            case PKElementType::Uint2: return 4 * 2;
            case PKElementType::Uint3: return 4 * 3;
            case PKElementType::Uint4: return 4 * 4;
            case PKElementType::Short: return 2;
            case PKElementType::Short2: return 2 * 2;
            case PKElementType::Short3: return 2 * 3;
            case PKElementType::Short4: return 2 * 4;
            case PKElementType::Ushort: return 2;
            case PKElementType::Ushort2: return 2 * 2;
            case PKElementType::Ushort3: return 2 * 3;
            case PKElementType::Ushort4: return 2 * 4;
            case PKElementType::Long: return 8;
            case PKElementType::Long2: return 8 * 2;
            case PKElementType::Long3: return 8 * 3;
            case PKElementType::Long4: return 8 * 4;
            case PKElementType::Ulong: return 8;
            case PKElementType::Ulong2: return 8 * 2;
            case PKElementType::Ulong3: return 8 * 3;
            case PKElementType::Ulong4: return 8 * 4;
            case PKElementType::Float2x2: return 4 * 2 * 2;
            case PKElementType::Float3x3: return 4 * 3 * 3;
            case PKElementType::Float4x4: return 4 * 4 * 4;
            case PKElementType::Float3x4: return 3 * 4 * 4;
            case PKElementType::Double2x2: return 8 * 2 * 2;
            case PKElementType::Double3x3: return 8 * 3 * 3;
            case PKElementType::Double4x4: return 8 * 4 * 4;
            case PKElementType::Half2x2: return 2 * 2 * 2;
            case PKElementType::Half3x3: return 2 * 3 * 3;
            case PKElementType::Half4x4: return 2 * 4 * 4;
            case PKElementType::Texture2DHandle: return 4;
            case PKElementType::Texture3DHandle: return 4;
            case PKElementType::TextureCubeHandle: return 4;
            default: return 0;
        }
    }

    uint32_t GetElementAlignment(PKElementType type)
    {
        switch (type)
        {
            case PKElementType::Float: return 4;
            case PKElementType::Float2: return 4 * 2;
            case PKElementType::Float3: return 4 * 4;
            case PKElementType::Float4: return 4 * 4;
            case PKElementType::Double: return 8;
            case PKElementType::Double2: return 8 * 2;
            case PKElementType::Double3: return 8 * 4;
            case PKElementType::Double4: return 8 * 4;
            case PKElementType::Half: return 2;
            case PKElementType::Half2: return 2 * 2;
            case PKElementType::Half3: return 2 * 4;
            case PKElementType::Half4: return 2 * 4;
            case PKElementType::Int: return 4;
            case PKElementType::Int2: return 4 * 2;
            case PKElementType::Int3: return 4 * 4;
            case PKElementType::Int4: return 4 * 4;
            case PKElementType::Uint: return 4;
            case PKElementType::Uint2: return 4 * 2;
            case PKElementType::Uint3: return 4 * 4;
            case PKElementType::Uint4: return 4 * 4;
            case PKElementType::Short: return 2;
            case PKElementType::Short2: return 2 * 2;
            case PKElementType::Short3: return 2 * 4;
            case PKElementType::Short4: return 2 * 4;
            case PKElementType::Ushort: return 2;
            case PKElementType::Ushort2: return 2 * 2;
            case PKElementType::Ushort3: return 2 * 4;
            case PKElementType::Ushort4: return 2 * 4;
            case PKElementType::Long: return 8;
            case PKElementType::Long2: return 8 * 2;
            case PKElementType::Long3: return 8 * 4;
            case PKElementType::Long4: return 8 * 4;
            case PKElementType::Ulong: return 8;
            case PKElementType::Ulong2: return 8 * 2;
            case PKElementType::Ulong3: return 8 * 4;
            case PKElementType::Ulong4: return 8 * 4;
            case PKElementType::Float2x2: return 4 * 2;
            case PKElementType::Float3x3: return 4 * 4;
            case PKElementType::Float4x4: return 4 * 4;
            case PKElementType::Float3x4: return 3 * 4;
            case PKElementType::Double2x2: return 8 * 2;
            case PKElementType::Double3x3: return 8 * 4;
            case PKElementType::Double4x4: return 8 * 4;
            case PKElementType::Half2x2: return 2 * 2;
            case PKElementType::Half3x3: return 2 * 4;
            case PKElementType::Half4x4: return 2 * 4;
            case PKElementType::Texture2DHandle: return 4;
            case PKElementType::Texture3DHandle: return 4;
            case PKElementType::TextureCubeHandle: return 4;
            default: return 0;
        }
    }

    uint32_t GetElementComponents(PKElementType type)
    {
        switch (type)
        {
            case PKElementType::Float: return 1;
            case PKElementType::Float2: return 2;
            case PKElementType::Float3: return 3;
            case PKElementType::Float4: return 4;
            case PKElementType::Double: return 1;
            case PKElementType::Double2: return 2;
            case PKElementType::Double3: return 3;
            case PKElementType::Double4: return 4;
            case PKElementType::Half: return 1;
            case PKElementType::Half2: return 2;
            case PKElementType::Half3: return 3;
            case PKElementType::Half4: return 4;
            case PKElementType::Int: return 1;
            case PKElementType::Int2: return 2;
            case PKElementType::Int3: return 3;
            case PKElementType::Int4: return 4;
            case PKElementType::Uint: return 1;
            case PKElementType::Uint2: return 2;
            case PKElementType::Uint3: return 3;
            case PKElementType::Uint4: return 4;
            case PKElementType::Short: return 1;
            case PKElementType::Short2: return 2;
            case PKElementType::Short3: return 3;
            case PKElementType::Short4: return 4;
            case PKElementType::Ushort: return 1;
            case PKElementType::Ushort2: return 2;
            case PKElementType::Ushort3: return 3;
            case PKElementType::Ushort4: return 4;
            case PKElementType::Long: return 1;
            case PKElementType::Long2: return 2;
            case PKElementType::Long3: return 3;
            case PKElementType::Long4: return 4;
            case PKElementType::Ulong: return 1;
            case PKElementType::Ulong2: return 2;
            case PKElementType::Ulong3: return 3;
            case PKElementType::Ulong4: return 4;
            case PKElementType::Float2x2: return 2;
            case PKElementType::Float3x3: return 3;
            case PKElementType::Float4x4: return 4;
            case PKElementType::Float3x4: return 3;
            case PKElementType::Double2x2: return 2;
            case PKElementType::Double3x3: return 3;
            case PKElementType::Double4x4: return 4;
            case PKElementType::Half2x2: return 2;
            case PKElementType::Half3x3: return 3;
            case PKElementType::Half4x4: return 4;
            case PKElementType::Texture2DHandle: return 1;
            case PKElementType::Texture3DHandle: return 1;
            case PKElementType::TextureCubeHandle: return 1;
            default: return 0;
        }
    }

    bool GetElementIsResourceHandle(PKElementType type)
    {
        switch (type)
        {
            case PKElementType::Texture2DHandle: return true;
            case PKElementType::Texture3DHandle: return true;
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

        // @TODO encode color 12 bit rgb
        vertex.colorSigns = 0u;

        if (pTexcoord)
        {
            auto t0 = (uint32_t)PackHalf(pTexcoord[0]);
            auto t1 = (uint32_t)PackHalf(pTexcoord[1]);
            vertex.texcoord = t0 | (t1 << 16u);
        }

        if (pNormal && pTangent)
        {
            vertex.rotation = EncodeQuaternion(pNormal, pTangent);
            vertex.colorSigns |= (pTangent[3] < 0.0f ? 0u : 1u) << 12u;
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
