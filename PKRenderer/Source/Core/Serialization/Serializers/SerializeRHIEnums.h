#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    #define PK_DECLARE_READ_WRITE(Type)                             \
    template<> struct ISerializer<Type>                             \
    {                                                               \
        static void ReadVal(const SerialNodeConst& node, Type* rhs);\
        static void WriteVal(SerialNode& node, Type const* rhs);    \
    };                                                              \

    PK_DECLARE_READ_WRITE(ElementType)
    PK_DECLARE_READ_WRITE(RHIAPI)
    PK_DECLARE_READ_WRITE(QueueType)
    PK_DECLARE_READ_WRITE(TextureType)
    PK_DECLARE_READ_WRITE(TextureBindMode)
    PK_DECLARE_READ_WRITE(Comparison)
    PK_DECLARE_READ_WRITE(FilterMode)
    PK_DECLARE_READ_WRITE(PolygonMode)
    PK_DECLARE_READ_WRITE(Topology)
    PK_DECLARE_READ_WRITE(WrapMode)
    PK_DECLARE_READ_WRITE(ColorMask)
    PK_DECLARE_READ_WRITE(LogicOp)
    PK_DECLARE_READ_WRITE(FrontFace)
    PK_DECLARE_READ_WRITE(LoadOp)
    PK_DECLARE_READ_WRITE(StoreOp)
    PK_DECLARE_READ_WRITE(BorderColor)
    PK_DECLARE_READ_WRITE(InputRate)
    PK_DECLARE_READ_WRITE(TextureUsage)
    PK_DECLARE_READ_WRITE(TextureFormat)
    PK_DECLARE_READ_WRITE(ColorSpace)
    PK_DECLARE_READ_WRITE(VSyncMode)
    PK_DECLARE_READ_WRITE(RayTracingShaderGroup)

    #undef PK_DECLARE_READ_WRITE
}
#endif
