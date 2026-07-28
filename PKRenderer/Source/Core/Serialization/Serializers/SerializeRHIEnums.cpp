#include "PrecompiledHeader.h"
#include "Core/Serialization/Serialize.h"
#include "Core/Utilities/FixedString.h"
#include "Core/RHI/Structs.h"

namespace PK
{
    #define DECLARE_RHI_ENUM_READ(TType)                                        \
    void ISerializer<TType>::ReadVal(SerialNodeRead node, TType* rhs)           \
    {                                                                           \
        auto value = node.val();                                                \
        FixedString128 valuestr(value.len, value.data());                       \
        *rhs = RHIEnumConvert::StringTo##TType(valuestr);                       \
    }                                                                           \
    void ISerializer<TType>::WriteVal(SerialNodeWrite node, const TType* rhs)   \
    {                                                                           \
        node << RHIEnumConvert::TType##ToString(*rhs) |= ryml::VAL_DQUO;        \
    }                                                                           \


    DECLARE_RHI_ENUM_READ(ElementType)
    DECLARE_RHI_ENUM_READ(RHIAPI)
    DECLARE_RHI_ENUM_READ(QueueType)
    DECLARE_RHI_ENUM_READ(TextureType)
    DECLARE_RHI_ENUM_READ(TextureBindMode)
    DECLARE_RHI_ENUM_READ(Comparison)
    DECLARE_RHI_ENUM_READ(FilterMode)
    DECLARE_RHI_ENUM_READ(PolygonMode)
    DECLARE_RHI_ENUM_READ(Topology)
    DECLARE_RHI_ENUM_READ(WrapMode)
    DECLARE_RHI_ENUM_READ(ColorMask)
    DECLARE_RHI_ENUM_READ(LogicOp)
    DECLARE_RHI_ENUM_READ(FrontFace)
    DECLARE_RHI_ENUM_READ(LoadOp)
    DECLARE_RHI_ENUM_READ(StoreOp)
    DECLARE_RHI_ENUM_READ(BorderColor)
    DECLARE_RHI_ENUM_READ(InputRate)
    DECLARE_RHI_ENUM_READ(TextureUsage)
    DECLARE_RHI_ENUM_READ(TextureFormat)
    DECLARE_RHI_ENUM_READ(ColorSpace)
    DECLARE_RHI_ENUM_READ(VSyncMode)
    DECLARE_RHI_ENUM_READ(RayTracingShaderGroup)
}
