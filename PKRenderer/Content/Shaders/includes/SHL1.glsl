#pragma once
#include "Constants.glsl"

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s9985-exploring-ray-traced-future-in-metro-exodus.pdf
struct SHCoCg
{
    float4 Y;
    float2 CoCg;
};

// Alternate compact SHL1 rgb formulation.
// Currently using this as it is trivially transformable to RGB SHL1 and compressible using rgb9e5 for A.
struct SHLuma
{
    float3 Y;
    float3 A;
};

#define pk_ZeroSHCoCg SHCoCg(0.0f.xxxx, 0.0f.xx)
#define pk_ZeroSHLuma SHLuma(0.0f.xxx, 0.0f.xxx)

float4 SH_GetBasis(const float3 d) { return float4(1.0f, d) * PK_L1BASIS; }

SHCoCg SH_Interpolate(const SHCoCg sha, const SHCoCg shb, const float i) { return SHCoCg(lerp(sha.Y, shb.Y, i), lerp(sha.CoCg, shb.CoCg, i)); }
SHCoCg SH_Scale(const SHCoCg sh, float s) { return SHCoCg(sh.Y * s, sh.CoCg * s); }
SHCoCg SH_Add(const SHCoCg sha, const SHCoCg shb, float sb) { return SHCoCg(sha.Y + shb.Y * sb, sha.CoCg + shb.CoCg * sb); }

SHLuma SH_Interpolate(const SHLuma sha, const SHLuma shb, const float i) { return SHLuma(lerp(sha.Y, shb.Y, i), lerp(sha.A, shb.A, i)); }
SHLuma SH_Scale(const SHLuma sh, float s) { return SHLuma(sh.Y * s, sh.A * s); }
SHLuma SH_Add(const SHLuma sha, const SHLuma shb, float sb) { return SHLuma(sha.Y + shb.Y * sb, sha.A + shb.A * sb); }

float SH_ToLuminanceL0(const SHCoCg sh) { return sh.Y.x / PK_L1BASIS.x; }
float3 SH_ToPeakDirection(const SHCoCg sh) { return sh.Y.yzw / (length(sh.Y.yzw) + 1e-6f); }
float SH_ToLuminanceL0(const SHLuma sh) { return dot(sh.A, PK_LUMA_BT709) / PK_L1BASIS.x; }
float3 SH_ToPeakDirection(const SHLuma sh) { return sh.Y / (length(sh.Y) + 1e-6f); }

float3 SH_ToDiffuse(const SHCoCg sh, const float3 dir)
{
    float Y = max(0.0f, 2.0f * (PK_L1BASIS_COSINE.y * dot(sh.Y.yzw, dir) + PK_L1BASIS_COSINE.x * sh.Y.x));
    float2 CoCg = sh.CoCg * PK_L1BASIS.x * Y / (sh.Y.x + 1e-6f);
    
    const float T = Y - CoCg.y * 0.5f;
    const float G = CoCg.y + T;
    const float B = T - CoCg.x * 0.5f;
    const float R = B + CoCg.x;
    return float3(R, G, B);
}

float3 SH_ToDiffuse(const SHLuma sh, const float3 dir)
{
    const float L = dot(sh.A, PK_LUMA_BT709); 
    const float Y = max(0.0f, 2.0f * (PK_L1BASIS_COSINE.y * dot(sh.Y, dir) + PK_L1BASIS_COSINE.x * L));
    return (sh.A / (1e-6f + L)) * Y;
}

float3 SH_ToColor(const SHCoCg sh) 
{
    const float Y = sh.Y.x / PK_L1BASIS.x;
    const float T = Y - sh.CoCg.y * 0.5f;
    const float G = sh.CoCg.y + T;
    const float B = T - sh.CoCg.x * 0.5f;
    const float R = B + sh.CoCg.x;
    return float3(R, G, B);
}

float3 SH_ToColor(const SHLuma sh) { return sh.A / PK_L1BASIS.x; }

SHCoCg SH_CoCg_FromRadiance(const float3 color, const float3 d)
{
    const float Co = color.r - color.b;
    const float T = color.b + Co * 0.5f;
    const float Cg = color.g - T;
    const float Y = max(T + Cg * 0.5f, 0.0);
    return SHCoCg(SH_GetBasis(d) * Y, float2(Co, Cg));
}

SHLuma SH_Luma_FromRadiance(const float3 color, const float3 d)
{
    const float4 basis = SH_GetBasis(d);
    const float luma = dot(color, PK_LUMA_BT709);
    SHLuma sh;
    sh.Y = basis.yzw * luma;
    sh.A = color * basis.x;
    return sh;
}
