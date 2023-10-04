#include "PrecompiledHeader.h"
#include "ShaderPropertyBlock.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Math;
    using namespace PK::Rendering::Structs;

    void ShaderPropertyBlock::ReserveLayout(const BufferLayout& layout)
    {
        for (auto& element : layout)
        {
            switch (element.Type)
            {
                case ElementType::Half:      Reserve<ushort>(element.NameHashId, sizeof(ushort)); break;
                case ElementType::Half2:     Reserve<ushort2>(element.NameHashId, sizeof(ushort2)); break;
                case ElementType::Half3:     Reserve<ushort3>(element.NameHashId, sizeof(ushort3)); break;
                case ElementType::Half4:     Reserve<ushort4>(element.NameHashId, sizeof(ushort4)); break;
                case ElementType::Half2x2:   Reserve<ushort2x2>(element.NameHashId, sizeof(ushort2x2)); break;
                case ElementType::Half3x3:   Reserve<ushort3x3>(element.NameHashId, sizeof(ushort3x3)); break;
                case ElementType::Half4x4:   Reserve<ushort4x4>(element.NameHashId, sizeof(ushort4x4)); break;

                case ElementType::Float:     Reserve<float>(element.NameHashId, element.Count); break;
                case ElementType::Float2:    Reserve<float2>(element.NameHashId, element.Count); break;
                case ElementType::Float3:    Reserve<float3>(element.NameHashId, element.Count); break;
                case ElementType::Float4:    Reserve<float4>(element.NameHashId, element.Count); break;
                case ElementType::Float2x2:  Reserve<float2x2>(element.NameHashId, element.Count); break;
                case ElementType::Float3x3:  Reserve<float3x3>(element.NameHashId, element.Count); break;
                case ElementType::Float4x4:  Reserve<float4x4>(element.NameHashId, element.Count); break;
                case ElementType::Float3x4:  Reserve<float3x4>(element.NameHashId, element.Count); break;

                case ElementType::Double:    Reserve<double>(element.NameHashId, element.Count); break;
                case ElementType::Double2:   Reserve<double2>(element.NameHashId, element.Count); break;
                case ElementType::Double3:   Reserve<double3>(element.NameHashId, element.Count); break;
                case ElementType::Double4:   Reserve<double4>(element.NameHashId, element.Count); break;
                case ElementType::Double2x2: Reserve<double2x2>(element.NameHashId, element.Count); break;
                case ElementType::Double3x3: Reserve<double3x3>(element.NameHashId, element.Count); break;
                case ElementType::Double4x4: Reserve<double4x4>(element.NameHashId, element.Count); break;

                case ElementType::Int:       Reserve<int>(element.NameHashId, element.Count); break;
                case ElementType::Int2:      Reserve<int2>(element.NameHashId, element.Count); break;
                case ElementType::Int3:      Reserve<int3>(element.NameHashId, element.Count); break;
                case ElementType::Int4:      Reserve<int4>(element.NameHashId, element.Count); break;

                case ElementType::Uint:      Reserve<uint>(element.NameHashId, element.Count); break;
                case ElementType::Uint2:     Reserve<uint2>(element.NameHashId, element.Count); break;
                case ElementType::Uint3:     Reserve<uint3>(element.NameHashId, element.Count); break;
                case ElementType::Uint4:     Reserve<uint4>(element.NameHashId, element.Count); break;

                case ElementType::Short:     Reserve<short>(element.NameHashId, element.Count); break;
                case ElementType::Short2:    Reserve<short2>(element.NameHashId, element.Count); break;
                case ElementType::Short3:    Reserve<short3>(element.NameHashId, element.Count); break;
                case ElementType::Short4:    Reserve<short4>(element.NameHashId, element.Count); break;

                case ElementType::Ushort:    Reserve<ushort>(element.NameHashId, element.Count); break;
                case ElementType::Ushort2:   Reserve<ushort2>(element.NameHashId, element.Count); break;
                case ElementType::Ushort3:   Reserve<ushort3>(element.NameHashId, element.Count); break;
                case ElementType::Ushort4:   Reserve<ushort4>(element.NameHashId, element.Count); break;

                case ElementType::Long:      Reserve<long>(element.NameHashId, element.Count); break;
                case ElementType::Long2:     Reserve<long2>(element.NameHashId, element.Count); break;
                case ElementType::Long3:     Reserve<long3>(element.NameHashId, element.Count); break;
                case ElementType::Long4:     Reserve<long4>(element.NameHashId, element.Count); break;

                case ElementType::Ulong:     Reserve<ulong>(element.NameHashId, element.Count); break;
                case ElementType::Ulong2:    Reserve<ulong2>(element.NameHashId, element.Count); break;
                case ElementType::Ulong3:    Reserve<ulong3>(element.NameHashId, element.Count); break;
                case ElementType::Ulong4:    Reserve<ulong4>(element.NameHashId, element.Count); break;

                case ElementType::Texture2DHandle:
                case ElementType::Texture3DHandle:
                case ElementType::TextureCubeHandle: Reserve<uint>(element.NameHashId, element.Count); break;
            }
        }
    }
}