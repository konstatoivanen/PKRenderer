#pragma once
#ifndef PK_UTILITIES
#define PK_UTILITIES

float pow2(float x) { return x * x; }
float pow3(float x) { return x * x * x; }
float pow4(float x) { return x * x * x * x; }
float pow5(float x) { return x * x * x * x * x; }

half pow2(half x) { return x * x; }
half pow3(half x) { return x * x * x; }
half pow4(half x) { return x * x * x * x; }
half pow5(half x) { return x * x * x * x * x; }

float cmin(float2 x) { return min(x.x, x.y); }
float cmin(float3 x) { return min(min(x.x, x.y), x.z); }
float cmin(float4 x) { return min(min(min(x.x, x.y), x.z), x.w); }
float cmax(float2 x) { return max(x.x, x.y); }
float cmax(float3 x) { return max(max(x.x, x.y), x.z); }
float cmax(float4 x) { return max(max(max(x.x, x.y), x.z), x.w); }

half cmin(half2 x) { return min(x.x, x.y); }
half cmin(half3 x) { return min(min(x.x, x.y), x.z); }
half cmin(half4 x) { return min(min(min(x.x, x.y), x.z), x.w); }
half cmax(half2 x) { return max(x.x, x.y); }
half cmax(half3 x) { return max(max(x.x, x.y), x.z); }
half cmax(half4 x) { return max(max(max(x.x, x.y), x.z), x.w); }

uint2 packHalf4x16(float4 v) { return uint2(packHalf2x16(v.xy), packHalf2x16(v.zw)); }
float4 unpackHalf4x16(uint2 v) { return float4(unpackHalf2x16(v.x), unpackHalf2x16(v.y)); }

float AsUnorm(uint x) { return uintBitsToFloat(x & 0x007fffffu | 0x3f800000u) - 1.0f; }
uint Checkerboard(uint2 coord, uint offset) { return ((coord.x ^ coord.y) ^ offset) & 0x1u; }

float SafePositiveRcp(float f) { return mix(1.0f / f, 0.0f, f <= 1e-12f); }
float3 SafeNormalize(float3 v) { return v * SafePositiveRcp(length(v)); }
float4 NormalizeLength(float3 v) { float l = length(v); return float4(v.xyz * SafePositiveRcp(l), l); }

float4 Mul3x3(const float3x3 matrix, const float4 v) { return float4(matrix * v.xyz, v.w); }
float4 Mul3x3(const float4 v, const float3x3 matrix) { return float4(v.xyz * matrix, v.w); }

half2 RadianRotation(half radian) { return half2(cos(radian), sin(radian)); }
float2 RadianRotation(float radian) { return float2(cos(radian), sin(radian)); }
half2 MulRotation(half2 v, half2 r) { return half2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x); }
float2 MulRotation(float2 v, float2 r) { return float2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x); }

// Source: https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void CreateTBN(const float3 n, out float3 b1, out float3 b2)
{
    const float s = n.z >= 0.0f ? 1.0f : -1.0f;
    const float a = -1.0f / (s + n.z);
    const float b = n.x * n.y * a;
    b1 = float3(1.0f + s * n.x * n.x * a, s * b, -s * n.x);
    b2 = float3(b, s + n.y * n.y * a, -n.y);
}

float2x3 CreateTB(const float3 n, float scale) { float3 t, b; CreateTBN(n, t, b); return float2x3(t * scale, b * scale); }
float3x3 CreateTBN(const float3 n) { float3 t, b; CreateTBN(n,t,b); return float3x3(t, b, n); }

#define lerp_sat(a,b,c) mix(a,b,clamp(c,0,1))
#define lerp_outquad(a,b,c) (-((b) - (a)) * (c) * ((c) - 2) + (a))
#define lerp_inquad(a,b,c) (((b) - (a)) * (c) * (c) + (a))
#define unlerp(a,b,value) (((value) - (a)) / ((b) - (a)))
#define unlerp_sat(a,b,value) saturate(((value) - (a)) / ((b) - (a)))
#define saturate(v) clamp(v, 0.0, 1.0)

#define Any_IsNaN(v) any(isnan(v))
#define Any_GEqual(a, b) any(greaterThanEqual(a,b))
#define Any_Equal(a, b) any(equal(a,b))
#define Any_NotEqual(a, b) any(notEqual(a,b))
#define Any_LEqual(a, b) any(lessThanEqual(a,b))
#define Any_Less(a,b) any(lessThan(a,b))
#define Any_Greater(a,b) any(greaterThan(a,b))

#define All_GEqual(a, b) all(greaterThanEqual(a,b))
#define All_Equal(a, b) all(equal(a,b))
#define All_NotEqual(a, b) all(notEqual(a,b))
#define All_LEqual(a, b) all(lessThanEqual(a,b))
#define All_Less(a,b) all(lessThan(a,b))
#define All_Greater(a,b) all(greaterThan(a,b))
#define All_InArea(a,b,c) (all(greaterThanEqual(a,b)) && all(lessThan(a,c)))

#define Replace_NaN(v, r) (any(isnan(v)) ? (r) : (v))

#define Test_NaN_EPS4(v) (isnan(v) || v <= 1e-4f)
#define Test_NaN_EPS6(v) (isnan(v) || v <= 1e-6f)
#define Test_EPS4(v) (v <= 1e-4f)
#define Test_EPS6(v) (v <= 1e-6f)

#define PK_DECLARE_CBUFFER(BufferName) layout(std140) uniform BufferName

// Ray tracing utilities
#if defined(SHADER_STAGE_RAY_GENERATION) || defined(SHADER_STAGE_RAY_MISS) || defined(SHADER_STAGE_RAY_CLOSEST_HIT) || defined(SHADER_STAGE_RAY_ANY_HIT) || defined(SHADER_STAGE_RAY_INTERSECTION)
    #define PK_IS_RAYTRACING_STAGE 
#endif

#if defined(SHADER_STAGE_RAY_MISS) || defined(SHADER_STAGE_RAY_CLOSEST_HIT) || defined(SHADER_STAGE_RAY_ANY_HIT) || defined(SHADER_STAGE_RAY_INTERSECTION)
    #define PK_IS_RAYTRACING_SUB_STAGE 
#endif

#if defined(SHADER_STAGE_RAY_CLOSEST_HIT) || defined(SHADER_STAGE_RAY_ANY_HIT)
    #define PK_IS_RAYTRACING_HIT_STAGE
#endif

#define PK_DECLARE_RT_BARYCOORDS(name) hitAttributeEXT float2 name

#if defined(PK_IS_RAYTRACING_SUB_STAGE)
    #define PK_DECLARE_RT_PAYLOAD(type, name, index) layout(location = index) rayPayloadInEXT type name
#elif defined(SHADER_STAGE_RAY_GENERATION)
    #define PK_DECLARE_RT_PAYLOAD(type, name, index) layout(location = index) rayPayloadEXT type name
    #define gl_WorldRayDirectionEXT 0.0f.xxx
    #define gl_WorldRayOriginEXT 0.0f.xxx
#else
    #define PK_DECLARE_RT_PAYLOAD(type, name, index)
#endif

#if defined(PK_IS_RAYTRACING_HIT_STAGE)
    #define PK_GET_RT_VERTEX_POSITION(index) gl_HitTriangleVertexPositionsEXT[index]
#else
    #define gl_HitTEXT 0.0f
    #define gl_ObjectToWorldEXT float4x3(1)
    #define PK_GET_RT_VERTEX_POSITION(index) float3(0,0,0)
#endif

#if defined(SHADER_STAGE_FRAGMENT)
    #define PK_GET_PROG_COORD int2(gl_FragCoord.xy)
#elif defined(SHADER_STAGE_COMPUTE)
    #define PK_GET_PROG_COORD int2(gl_GlobalInvocationID)
#else
    #define PK_GET_PROG_COORD int2(0)
#endif

#if defined(PK_IS_RAYTRACING_STAGE) || defined(PK_ALLOW_TLAS_DECLARATION)
    #define PK_DECLARE_ACCELERATION_STRUCTURE(Name) uniform accelerationStructureEXT Name;
#else
    #define PK_DECLARE_ACCELERATION_STRUCTURE(Name)
#endif

#if defined(SHADER_STAGE_MESH_ASSEMBLY)
    #define PK_DECLARE_FS_OUT(variable) variable
    #define PK_DECLARE_VS_ATTRIB(variable) out variable[]
    #define PK_SET_VS_ATTRIB(variable, index, value) variable[index] = value
#elif defined(SHADER_STAGE_VERTEX)
    #define PK_DECLARE_FS_OUT(variable) variable
    #define PK_DECLARE_VS_ATTRIB(variable) out variable
    #define PK_SET_VS_ATTRIB(variable, index, value) variable = value
#elif defined(SHADER_STAGE_FRAGMENT)
    #define PK_DECLARE_FS_OUT(variable) out variable
    #define PK_DECLARE_VS_ATTRIB(variable) in variable
    #define PK_SET_VS_ATTRIB(variable, index, value)
#else
    #define PK_DECLARE_FS_OUT(variable) variable
    #define PK_DECLARE_VS_ATTRIB(variable)
    #define PK_SET_VS_ATTRIB(variable, index, value)
#endif

#endif
