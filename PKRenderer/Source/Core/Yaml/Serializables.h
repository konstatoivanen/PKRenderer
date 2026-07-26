#pragma once
#if defined(PK_CUSTOM_SERIALIZABLES)

#include "Core/Math/Forward.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct CVariablesYaml;
    struct CommandInputKeyBindingMap;
    struct InputKeyCommandBindings;

    template<typename TChar, size_t capacity> struct IFixedString;

    template<size_t capacity> using FixedString = IFixedString<char, capacity>;

    typedef FixedString<16> FixedString16;
    typedef FixedString<32> FixedString32;

    using FixedString16 = FixedString<16>;
    using FixedString32 = FixedString<32>;
    using FixedString64 = FixedString<64>;
    using FixedString128 = FixedString<128>;
    using FixedString256 = FixedString<256>;
    using FixedString512 = FixedString<512>;
    using FixedString1024 = FixedString<1024>;
}

// Forward declare custom serializers so that struct reflection can pick up on them.
// @TODO figure out something better....
namespace PK::Serialize
{
#define PK_DECLARE_READ_WRITE(Type)                               \
    template<> void Read<Type>(const ConstNode& node, Type* rhs); \
    template<> void Write<Type>(Node& node, Type const* rhs);     \

    template<> void Read<FixedString16>(const ConstNode& node, FixedString16* rhs);
    template<> void ReadKey<FixedString16>(const ConstNode& node, FixedString16* rhs);
    template<> void Write<FixedString16>(Node& node, const FixedString16* rhs);

    template<> void Read<FixedString32>(const ConstNode& node, FixedString32* rhs);
    template<> void ReadKey<FixedString32>(const ConstNode& node, FixedString32* rhs);
    template<> void Write<FixedString32>(Node& node, const FixedString32* rhs);

    template<> void Read<FixedString64>(const ConstNode& node, FixedString64* rhs);
    template<> void ReadKey<FixedString64>(const ConstNode& node, FixedString64* rhs);
    template<> void Write<FixedString64>(Node& node, const FixedString64* rhs);

    template<> void Read<FixedString128>(const ConstNode& node, FixedString128* rhs);
    template<> void ReadKey<FixedString128>(const ConstNode& node, FixedString128* rhs);
    template<> void Write<FixedString128>(Node& node, const FixedString128* rhs);

    template<> void Read<FixedString256>(const ConstNode& node, FixedString256* rhs);
    template<> void ReadKey<FixedString256>(const ConstNode& node, FixedString256* rhs);
    template<> void Write<FixedString256>(Node& node, const FixedString256* rhs);

    template<> void Read<FixedString512>(const ConstNode& node, FixedString512* rhs);
    template<> void ReadKey<FixedString512>(const ConstNode& node, FixedString512* rhs);
    template<> void Write<FixedString512>(Node& node, const FixedString512* rhs);

    template<> void Read<CVariablesYaml>(const ConstNode& node, CVariablesYaml* rhs);
    template<> void Read<CommandInputKeyBindingMap>(const ConstNode& node, CommandInputKeyBindingMap* rhs);
    template<> void Read<InputKeyCommandBindings>(const ConstNode& node, InputKeyCommandBindings* rhs);

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
    PK_DECLARE_READ_WRITE(Material*)
    PK_DECLARE_READ_WRITE(MaterialRef)
    PK_DECLARE_READ_WRITE(MaterialTarget)
    PK_DECLARE_READ_WRITE(MeshStatic*)
    PK_DECLARE_READ_WRITE(MeshStaticRef)
    PK_DECLARE_READ_WRITE(Mesh*)
    PK_DECLARE_READ_WRITE(MeshRef)
    PK_DECLARE_READ_WRITE(ShaderAsset*)
    PK_DECLARE_READ_WRITE(ShaderAssetRef)
    PK_DECLARE_READ_WRITE(TextureAsset*)
    PK_DECLARE_READ_WRITE(TextureAssetRef)
    PK_DECLARE_READ_WRITE(RHITexture*)
    PK_DECLARE_READ_WRITE(bool)
    PK_DECLARE_READ_WRITE(uint8_t)
    PK_DECLARE_READ_WRITE(int8_t)
    PK_DECLARE_READ_WRITE(uint16_t)
    PK_DECLARE_READ_WRITE(int16_t)
    PK_DECLARE_READ_WRITE(int32_t)
    PK_DECLARE_READ_WRITE(uint32_t)
    PK_DECLARE_READ_WRITE(int64_t)
    PK_DECLARE_READ_WRITE(uint64_t)
    PK_DECLARE_READ_WRITE(float)
    PK_DECLARE_READ_WRITE(double)

    PK_DECLARE_READ_WRITE(float2)
    PK_DECLARE_READ_WRITE(float3)
    PK_DECLARE_READ_WRITE(float4)

    PK_DECLARE_READ_WRITE(double2)
    PK_DECLARE_READ_WRITE(double3)
    PK_DECLARE_READ_WRITE(double4)

    PK_DECLARE_READ_WRITE(short2)
    PK_DECLARE_READ_WRITE(short3)
    PK_DECLARE_READ_WRITE(short4)

    PK_DECLARE_READ_WRITE(ushort2)
    PK_DECLARE_READ_WRITE(ushort3)
    PK_DECLARE_READ_WRITE(ushort4)

    PK_DECLARE_READ_WRITE(byte4)
    PK_DECLARE_READ_WRITE(sbyte4)

    PK_DECLARE_READ_WRITE(int2)
    PK_DECLARE_READ_WRITE(int3)
    PK_DECLARE_READ_WRITE(int4)

    PK_DECLARE_READ_WRITE(uint2)
    PK_DECLARE_READ_WRITE(uint3)
    PK_DECLARE_READ_WRITE(uint4)

    PK_DECLARE_READ_WRITE(long2)
    PK_DECLARE_READ_WRITE(long3)
    PK_DECLARE_READ_WRITE(long4)

    PK_DECLARE_READ_WRITE(ulong2)
    PK_DECLARE_READ_WRITE(ulong3)
    PK_DECLARE_READ_WRITE(ulong4)

    PK_DECLARE_READ_WRITE(bool2)
    PK_DECLARE_READ_WRITE(bool3)
    PK_DECLARE_READ_WRITE(bool4)

    PK_DECLARE_READ_WRITE(float2x2)
    PK_DECLARE_READ_WRITE(float3x3)
    PK_DECLARE_READ_WRITE(float3x4)
    PK_DECLARE_READ_WRITE(float4x4)

    PK_DECLARE_READ_WRITE(double2x2)
    PK_DECLARE_READ_WRITE(double3x3)
    PK_DECLARE_READ_WRITE(double3x4)
    PK_DECLARE_READ_WRITE(double4x4)

    PK_DECLARE_READ_WRITE(ushort2x2)
    PK_DECLARE_READ_WRITE(ushort3x3)
    PK_DECLARE_READ_WRITE(ushort3x4)
    PK_DECLARE_READ_WRITE(ushort4x4)
}
#endif
