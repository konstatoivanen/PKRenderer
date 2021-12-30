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

    uint_t Assets::GetElementSize(PKElementType type)
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

    uint_t Assets::GetElementAlignment(PKElementType type)
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

    uint_t Assets::GetElementComponents(PKElementType type)
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