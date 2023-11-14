#include "PKAsset.h"
#include <string>

namespace PK::Assets
{
    PKElementType GetElementType(const char* string)
    {
        std::string format(string);

        if (format == "half")
        {
            return PKElementType::Half;
        }

        if (format == "half2")
        {
            return PKElementType::Half2;
        }

        if (format == "half3")
        {
            return PKElementType::Half3;
        }

        if (format == "half4")
        {
            return PKElementType::Half4;
        }

        if (format == "float")
        {
            return PKElementType::Float;
        }

        if (format == "float2")
        {
            return PKElementType::Float2;
        }

        if (format == "float3")
        {
            return PKElementType::Float3;
        }

        if (format == "float4")
        {
            return PKElementType::Float4;
        }

        if (format == "double")
        {
            return PKElementType::Double;
        }

        if (format == "double2")
        {
            return PKElementType::Double2;
        }

        if (format == "double3")
        {
            return PKElementType::Double3;
        }

        if (format == "double4")
        {
            return PKElementType::Double4;
        }

        if (format == "short")
        {
            return PKElementType::Short;
        }

        if (format == "short2")
        {
            return PKElementType::Short2;
        }

        if (format == "short3")
        {
            return PKElementType::Short3;
        }

        if (format == "short4")
        {
            return PKElementType::Short4;
        }

        if (format == "ushort")
        {
            return PKElementType::Ushort;
        }

        if (format == "ushort2")
        {
            return PKElementType::Ushort2;
        }

        if (format == "ushort3")
        {
            return PKElementType::Ushort3;
        }

        if (format == "ushort4")
        {
            return PKElementType::Ushort4;
        }

        if (format == "int")
        {
            return PKElementType::Int;
        }

        if (format == "int2")
        {
            return PKElementType::Int2;
        }

        if (format == "int3")
        {
            return PKElementType::Int3;
        }

        if (format == "int4")
        {
            return PKElementType::Int4;
        }

        if (format == "uint")
        {
            return PKElementType::Uint;
        }

        if (format == "uint2")
        {
            return PKElementType::Uint2;
        }

        if (format == "uint3")
        {
            return PKElementType::Uint3;
        }

        if (format == "uint4")
        {
            return PKElementType::Uint4;
        }

        if (format == "long")
        {
            return PKElementType::Long;
        }

        if (format == "long2")
        {
            return PKElementType::Long2;
        }

        if (format == "long3")
        {
            return PKElementType::Long3;
        }

        if (format == "long4")
        {
            return PKElementType::Long4;
        }

        if (format == "ulong")
        {
            return PKElementType::Ulong;
        }

        if (format == "ulong2")
        {
            return PKElementType::Ulong2;
        }

        if (format == "ulong3")
        {
            return PKElementType::Ulong3;
        }

        if (format == "ulong4")
        {
            return PKElementType::Ulong4;
        }

        if (format == "half2x2")
        {
            return PKElementType::Half2x2;
        }

        if (format == "half3x3")
        {
            return PKElementType::Half3x3;
        }

        if (format == "half4x4")
        {
            return PKElementType::Half4x4;
        }

        if (format == "float2x2")
        {
            return PKElementType::Float2x2;
        }

        if (format == "float3x3")
        {
            return PKElementType::Float3x3;
        }

        if (format == "float4x4")
        {
            return PKElementType::Float4x4;
        }

        if (format == "float3x4")
        {
            return PKElementType::Float3x4;
        }

        if (format == "double2x2")
        {
            return PKElementType::Double2x2;
        }

        if (format == "double3x3")
        {
            return PKElementType::Double3x3;
        }

        if (format == "double4x4")
        {
            return PKElementType::Double4x4;
        }

        if (format == "texture2D")
        {
            return PKElementType::Texture2DHandle;
        }

        if (format == "texture3D")
        {
            return PKElementType::Texture3DHandle;
        }

        if (format == "textureCube")
        {
            return PKElementType::TextureCubeHandle;
        }

        return PKElementType::Invalid;
    }

    uint32_t Assets::GetElementSize(PKElementType type)
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
        }

        return 0;
    }

    uint32_t Assets::GetElementAlignment(PKElementType type)
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
        }

        return 0;
    }

    uint32_t Assets::GetElementComponents(PKElementType type)
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
        }

        return 0;
    }
}

namespace PK::Assets::Mesh::Meshlet
{
    uint16_t PackHalf(float v)
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

    uint16_t PackUnorm16(float v)
    {
        auto i = (int32_t)roundf(v * 65535.0f);
        if (i < 0) { i = 0; }
        if (i > 65535) { i = 65535; }
        return (uint16_t)(i & 0xFFFFu);
    }

    uint32_t PackUnorm12(float v)
    {
        auto i = (int32_t)roundf(v * 4095.0f);
        if (i < 0) { i = 0; }
        if (i > 4095) { i = 4095; }
        return (uint32_t)(i & 0xFFFu);
    }

    uint8_t PackUnorm8(float v)
    {
        auto i = (int32_t)roundf(v * 255.0f);
        if (i < 0) { i = 0; }
        if (i > 255) { i = 255; }
        return (uint8_t)(i & 0xFFu);
    }

    float abs(float v) { return v < 0.0f ? -v : v; }

    void OctaEncode(const float* n, float* outuv)
    {
        float t[3] = { n[0], n[1], n[2] };
        float f = abs(n[0]) + abs(n[1]) + abs(n[2]);
        t[0] /= f;
        t[1] /= f;
        t[2] /= f;

        if (t[1] >= 0.0f)
        {
            outuv[0] = t[0];
            outuv[1] = t[2];
        }
        else
        {
            outuv[0] = (1.0f - abs(t[2])) * (t[0] >= 0.0f ? 1.0f : -1.0f);
            outuv[1] = (1.0f - abs(t[0])) * (t[2] >= 0.0f ? 1.0f : -1.0f);
        }

        outuv[0] = outuv[0] * 0.5f + 0.5f;
        outuv[1] = outuv[1] * 0.5f + 0.5f;
    }

    PKVertex PackVertex(const float* pPosition,
                        const float* pTexcoord,
                        const float* pNormal,
                        const float* pTangent,
                        const float* bbmin,
                        const float* bbmax)
    {
        PKVertex vertex = { 0u, 0u, 0u };

        uint32_t unormPositions[3] =
        {
            PackUnorm16((pPosition[0] - bbmin[0]) / (bbmax[0] - bbmin[0])),
            PackUnorm16((pPosition[1] - bbmin[1]) / (bbmax[1] - bbmin[1])),
            PackUnorm16((pPosition[2] - bbmin[2]) / (bbmax[2] - bbmin[2]))
        };

        vertex.packed0 = unormPositions[0] | (unormPositions[1] << 16u);
        vertex.packed1 = unormPositions[2];

        if (pTexcoord)
        {
            auto t0 = PackUnorm12(pTexcoord[0]);
            auto t1 = PackUnorm12(pTexcoord[1]);
            vertex.packed1 |= t0 << 16u;
            vertex.packed1 |= (t1 & 0xF) << 28u;
            vertex.packed2 |= (t1 >> 4u) & 0xFFu;
        }

        if (pNormal)
        {
            float normalOctaUV[2];
            OctaEncode(pNormal, normalOctaUV);
            auto n0 = PackUnorm12(normalOctaUV[0]);
            auto n1 = PackUnorm12(normalOctaUV[1]);
            vertex.packed2 |= n0 << 8u;
            vertex.packed2 |= n1 << 20u;
        }

        if (pTangent)
        {
            float octauv[2];
            OctaEncode(pTangent, octauv);
            uint8_t sign = pTangent[4] < 0.0f ? 0u : 3u;

            auto ui = (int32_t)(octauv[0] * 32766.0f);
            auto vi = (int32_t)(octauv[1] * 32766.0f);
            if (ui < 0) { ui = 0; }
            if (vi < 0) { vi = 0; }
            if (ui > 32766) { ui = 32766; }
            if (vi > 32766) { vi = 32766; }

            auto u = (uint32_t)ui;
            auto v = (uint32_t)vi;

            vertex.packed3 = (u & 0x7FFFu) | ((v & 0x7FFFu) << 15u) | ((sign & 0x3) << 30u);
        }

        return vertex;
    }
    
    PKMeshlet PackMeshlet(uint32_t vertexFirst,
                          uint32_t triangleFirst,
                          uint32_t vertexCount,
                          uint32_t triangleCount,
                          const float* bbmin,
                          const float* bbmax,
                          const float* submeshbbmin,
                          const float* submeshbbmax,
                          const float* coneAxis,
                          const float* coneApex,
                          float coneCutoff)
    {
        float octauv[2];
        OctaEncode(coneAxis, octauv);

        PKMeshlet meshlet;
        meshlet.vertexFirst = vertexFirst;
        meshlet.triangleFirst = triangleFirst;
        meshlet.bbmin[0] = PackUnorm16((bbmin[0] - submeshbbmin[0]) / (submeshbbmax[0] - submeshbbmin[0]));
        meshlet.bbmin[1] = PackUnorm16((bbmin[1] - submeshbbmin[1]) / (submeshbbmax[1] - submeshbbmin[1]));
        meshlet.bbmin[2] = PackUnorm16((bbmin[2] - submeshbbmin[2]) / (submeshbbmax[2] - submeshbbmin[2]));
        meshlet.vertexCount = (uint8_t)vertexCount;
        meshlet.triangleCount = (uint8_t)triangleCount;
        meshlet.bbmax[0] = PackUnorm16((bbmax[0] - submeshbbmin[0]) / (submeshbbmax[0] - submeshbbmin[0]));
        meshlet.bbmax[1] = PackUnorm16((bbmax[1] - submeshbbmin[1]) / (submeshbbmax[1] - submeshbbmin[1]));
        meshlet.bbmax[2] = PackUnorm16((bbmax[2] - submeshbbmin[2]) / (submeshbbmax[2] - submeshbbmin[2]));
        meshlet.coneAxis[0] = PackUnorm8(octauv[0]);
        meshlet.coneAxis[1] = PackUnorm8(octauv[1]);
        meshlet.coneApex[0] = PackUnorm16((coneApex[0] - bbmin[0]) / (bbmax[0] - bbmin[0]));
        meshlet.coneApex[1] = PackUnorm16((coneApex[1] - bbmin[1]) / (bbmax[1] - bbmin[1]));
        meshlet.coneApex[2] = PackUnorm16((coneApex[2] - bbmin[2]) / (bbmax[2] - bbmin[2]));
        meshlet.coneCutoff = PackUnorm16(coneCutoff * 0.5f + 0.5f);
        return meshlet;
    }

}