#include "PrecompiledHeader.h"
#include "Math/PKMath.h"
#include "Enums.h"

namespace PK::Rendering::Structs::ElementConvert
{
    using namespace PK::Math;

    uint16_t Size(ElementType format)
    {
        switch (format)
        {
            case Structs::ElementType::Float: return 4;
            case Structs::ElementType::Float2: return 4 * 2;
            case Structs::ElementType::Float3: return 4 * 3;
            case Structs::ElementType::Float4: return 4 * 4;

            case Structs::ElementType::Double: return 8;
            case Structs::ElementType::Double2: return 8 * 2;
            case Structs::ElementType::Double3: return 8 * 3;
            case Structs::ElementType::Double4: return 8 * 4;

            case Structs::ElementType::Half: return 2;
            case Structs::ElementType::Half2: return 2 * 2;
            case Structs::ElementType::Half3: return 2 * 3;
            case Structs::ElementType::Half4: return 2 * 4;

            case Structs::ElementType::Int: return 4;
            case Structs::ElementType::Int2: return 4 * 2;
            case Structs::ElementType::Int3: return 4 * 3;
            case Structs::ElementType::Int4: return 4 * 4;

            case Structs::ElementType::Uint: return 4;
            case Structs::ElementType::Uint2: return 4 * 2;
            case Structs::ElementType::Uint3: return 4 * 3;
            case Structs::ElementType::Uint4: return 4 * 4;

            case Structs::ElementType::Short: return 2;
            case Structs::ElementType::Short2: return 2 * 2;
            case Structs::ElementType::Short3: return 2 * 3;
            case Structs::ElementType::Short4: return 2 * 4;

            case Structs::ElementType::Ushort: return 2;
            case Structs::ElementType::Ushort2: return 2 * 2;
            case Structs::ElementType::Ushort3: return 2 * 3;
            case Structs::ElementType::Ushort4: return 2 * 4;

            case Structs::ElementType::Long: return 8;
            case Structs::ElementType::Long2: return 8 * 2;
            case Structs::ElementType::Long3: return 8 * 3;
            case Structs::ElementType::Long4: return 8 * 4;

            case Structs::ElementType::Ulong: return 8;
            case Structs::ElementType::Ulong2: return 8 * 2;
            case Structs::ElementType::Ulong3: return 8 * 3;
            case Structs::ElementType::Ulong4: return 8 * 4;

            case Structs::ElementType::Float2x2: return 4 * 2 * 2;
            case Structs::ElementType::Float3x3: return 4 * 3 * 3;
            case Structs::ElementType::Float4x4: return 4 * 4 * 4;

            case Structs::ElementType::Double2x2: return 8 * 2 * 2;
            case Structs::ElementType::Double3x3: return 8 * 3 * 3;
            case Structs::ElementType::Double4x4: return 8 * 4 * 4;

            case Structs::ElementType::Half2x2: return 2 * 2 * 2;
            case Structs::ElementType::Half3x3: return 2 * 3 * 3;
            case Structs::ElementType::Half4x4: return 2 * 4 * 4;
        }

        return 0;
    }
    
    uint16_t Alignment(ElementType format)
    {
        switch (format)
        {
            case Structs::ElementType::Float: return 4;
            case Structs::ElementType::Float2: return 4 * 2;
            case Structs::ElementType::Float3: return 4 * 4;
            case Structs::ElementType::Float4: return 4 * 4;

            case Structs::ElementType::Double: return 8;
            case Structs::ElementType::Double2: return 8 * 2;
            case Structs::ElementType::Double3: return 8 * 4;
            case Structs::ElementType::Double4: return 8 * 4;

            case Structs::ElementType::Half: return 2;
            case Structs::ElementType::Half2: return 2 * 2;
            case Structs::ElementType::Half3: return 2 * 4;
            case Structs::ElementType::Half4: return 2 * 4;

            case Structs::ElementType::Int: return 4;
            case Structs::ElementType::Int2: return 4 * 2;
            case Structs::ElementType::Int3: return 4 * 4;
            case Structs::ElementType::Int4: return 4 * 4;

            case Structs::ElementType::Uint: return 4;
            case Structs::ElementType::Uint2: return 4 * 2;
            case Structs::ElementType::Uint3: return 4 * 4;
            case Structs::ElementType::Uint4: return 4 * 4;

            case Structs::ElementType::Short: return 2;
            case Structs::ElementType::Short2: return 2 * 2;
            case Structs::ElementType::Short3: return 2 * 4;
            case Structs::ElementType::Short4: return 2 * 4;

            case Structs::ElementType::Ushort: return 2;
            case Structs::ElementType::Ushort2: return 2 * 2;
            case Structs::ElementType::Ushort3: return 2 * 4;
            case Structs::ElementType::Ushort4: return 2 * 4;

            case Structs::ElementType::Long: return 8;
            case Structs::ElementType::Long2: return 8 * 2;
            case Structs::ElementType::Long3: return 8 * 4;
            case Structs::ElementType::Long4: return 8 * 4;

            case Structs::ElementType::Ulong: return 8;
            case Structs::ElementType::Ulong2: return 8 * 2;
            case Structs::ElementType::Ulong3: return 8 * 4;
            case Structs::ElementType::Ulong4: return 8 * 4;

            case Structs::ElementType::Float2x2: return 4 * 2;
            case Structs::ElementType::Float3x3: return 4 * 4;
            case Structs::ElementType::Float4x4: return 4 * 4;

            case Structs::ElementType::Double2x2: return 8 * 2;
            case Structs::ElementType::Double3x3: return 8 * 4;
            case Structs::ElementType::Double4x4: return 8 * 4;

            case Structs::ElementType::Half2x2: return 2 * 2;
            case Structs::ElementType::Half3x3: return 2 * 4;
            case Structs::ElementType::Half4x4: return 2 * 4;
        }

        return 0;
    }

    uint16_t Components(ElementType format)
    {
        switch (format)
        {
            case Structs::ElementType::Float: return 1;
            case Structs::ElementType::Float2: return 2;
            case Structs::ElementType::Float3: return 3;
            case Structs::ElementType::Float4: return 4;

            case Structs::ElementType::Double: return 1;
            case Structs::ElementType::Double2: return 2;
            case Structs::ElementType::Double3: return 3;
            case Structs::ElementType::Double4: return 4;

            case Structs::ElementType::Half: return 1;
            case Structs::ElementType::Half2: return 2;
            case Structs::ElementType::Half3: return 3;
            case Structs::ElementType::Half4: return 4;

            case Structs::ElementType::Int: return 1;
            case Structs::ElementType::Int2: return 2;
            case Structs::ElementType::Int3: return 3;
            case Structs::ElementType::Int4: return 4;

            case Structs::ElementType::Uint: return 1;
            case Structs::ElementType::Uint2: return 2;
            case Structs::ElementType::Uint3: return 3;
            case Structs::ElementType::Uint4: return 4;

            case Structs::ElementType::Short: return 1;
            case Structs::ElementType::Short2: return 2;
            case Structs::ElementType::Short3: return 3;
            case Structs::ElementType::Short4: return 4;

            case Structs::ElementType::Ushort: return 1;
            case Structs::ElementType::Ushort2: return 2;
            case Structs::ElementType::Ushort3: return 3;
            case Structs::ElementType::Ushort4: return 4;

            case Structs::ElementType::Long: return 1;
            case Structs::ElementType::Long2: return 2;
            case Structs::ElementType::Long3: return 3;
            case Structs::ElementType::Long4: return 4;

            case Structs::ElementType::Ulong: return 1;
            case Structs::ElementType::Ulong2: return 2;
            case Structs::ElementType::Ulong3: return 3;
            case Structs::ElementType::Ulong4: return 4;

            case Structs::ElementType::Float2x2: return 2;
            case Structs::ElementType::Float3x3: return 3;
            case Structs::ElementType::Float4x4: return 4;

            case Structs::ElementType::Double2x2: return 2;
            case Structs::ElementType::Double3x3: return 3;
            case Structs::ElementType::Double4x4: return 4;

            case Structs::ElementType::Half2x2: return 2;
            case Structs::ElementType::Half3x3: return 3;
            case Structs::ElementType::Half4x4: return 4;
        }

        return 0;
    }
}