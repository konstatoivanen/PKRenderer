#include "PrecompiledHeader.h"

namespace PK::math
{
    uint16_t f32tof16(float v)
    {
        // @TODO add support for intrinsics conversions where available.
        const auto u0 = asuint(v);
        const auto u1 = u0 & 0x7FFFF000u;
        const auto u2 = (asuint(min(asfloat(u1) * 1.92592994e-34f, 260042752.0f)) + 0x1000u) >> 13u;
        return (u1 >= 0x7f800000u ? u1 > 0x7f800000u ? 0x7e00u : 0x7c00u : u2) | (u0 & ~0x7FFFF000u) >> 16u;
    }

    float f16tof32(uint16_t x)
    {
        uint32_t uf = (x & 0x7fffu) << 13u;
        uint32_t e = uf & 0xf800000u;
        uf += e == 0xf800000u ? 0x70000000u : 0x38000000u;
        uf = e == 0 ? asuint(asfloat(uf + (1 << 23)) - 6.10351563e-05f) : uf;
        return asfloat(uf | (x & 0x8000) << 16);
    }
}
