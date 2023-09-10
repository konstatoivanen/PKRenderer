#pragma once
#ifndef PK_UTILITIES
#define PK_UTILITIES

float pow2(float x) { return x * x; }
float pow3(float x) { return x * x * x; }
float pow4(float x) { return x * x * x * x; }
float pow5(float x) { return x * x * x * x * x; }
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
float2 make_moments(float v) { return float2(v, v * v); }
float2 make_rotation(float radian)  { return float2(cos(radian), sin(radian)); }
float2 rotate2D(float2 v, float2 r)  { return float2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x); }
float4 mul3x3(const float3x3 matrix, const float4 v) { return float4(matrix * v.xyz, v.w); }
float4 unpackHalf4x16(uint2 v) { return float4(unpackHalf2x16(v.x), unpackHalf2x16(v.y)); }
uint2 packHalf4x16(float4 v) { return uint2(packHalf2x16(v.xy), packHalf2x16(v.zw)); }
float safePositiveRcp(float f) { return f <= 1e-12f ? 0.0 : 1.0 / f; }
float4 normalizeLength(float3 v) { float l = length(v); return float4(v.xyz * safePositiveRcp(l), l); }

#define lerp_sat(a,b,c) mix(a,b,clamp(c,0,1))
#define lerp_outquad(a,b,c) (-((b) - (a)) * (c) * ((c) - 2) + (a))
#define lerp_inquad(a,b,c) (((b) - (a)) * (c) * (c) + (a))
#define unlerp(a,b,value) (((value) - (a)) / ((b) - (a)))
#define unlerp_sat(a,b,value) saturate(((value) - (a)) / ((b) - (a)))
#define saturate(v) clamp(v, 0.0, 1.0)
#define lerp_true(x,y,s) ((x) + (s) * ((y) - (x)))
#define POW2(x) ((x) * (x))
#define POW3(x) ((x) * (x) * (x))
#define POW4(x) ((x) * (x) * (x) * (x))
#define POW5(x) ((x) * (x) * (x) * (x) * (x))
#define mul(a,b) (a * b)

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

#define PK_SET_GLOBAL 0
#define PK_SET_PASS 1
#define PK_SET_SHADER 2
#define PK_SET_DRAW 3
#define PK_DECLARE_SET_GLOBAL layout(set = 0)
#define PK_DECLARE_SET_PASS layout(set = 1)
#define PK_DECLARE_SET_SHADER layout(set = 2)
#define PK_DECLARE_SET_DRAW layout(set = 3)

#define PK_DECLARE_LOCAL_CBUFFER(BufferName) layout(push_constant) uniform BufferName
#define PK_DECLARE_CBUFFER(BufferName, Set) layout(std140, set = Set) uniform BufferName

#define PK_DECLARE_BUFFER(ValueType, BufferName, Set) layout(std430, set = Set) buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_READONLY_BUFFER(ValueType, BufferName, Set) layout(std430, set = Set) readonly buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_WRITEONLY_BUFFER(ValueType, BufferName, Set) layout(std430, set = Set) writeonly buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_RESTRICTED_BUFFER(ValueType, BufferName, Set) layout(std430, set = Set) restrict buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_RESTRICTED_READONLY_BUFFER(ValueType, BufferName, Set) layout(std430, set = Set) restrict readonly buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_VARIABLE(ValueType, BufferName, Set) layout(std430, set = Set) buffer BufferName { ValueType BufferName##_Data; }

#define PK_BUFFER_DATA(BufferName, index) BufferName##_Data[index]
#define PK_VARIABLE_DATA(BufferName) BufferName##_Data

// Ray tracing utilities
#define PK_GET_RAY_HIT_POINT (gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT)
#define PK_GET_RAY_HIT_DISTANCE gl_HitTEXT
#define PK_DECLARE_RT_BARYCOORDS(name) hitAttributeEXT float2 name
#define PK_DECLARE_RT_PAYLOAD_IN(type, name, index) layout(location = index) rayPayloadInEXT type name
#define PK_DECLARE_RT_PAYLOAD_OUT(type, name, index) layout(location = index) rayPayloadEXT type name

#if defined(SHADER_STAGE_RAY_GENERATION) || defined(SHADER_STAGE_RAY_MISS) || defined(SHADER_STAGE_RAY_CLOSEST_HIT) || defined(SHADER_STAGE_RAY_ANY_HIT) || defined(SHADER_STAGE_RAY_INTERSECTION)
    #define PK_IS_RAYTRACING_STAGE 
#endif

#if defined(PK_IS_RAYTRACING_STAGE) || defined(PK_ALLOW_TLAS_DECLARATION)
    #define PK_DECLARE_ACCELERATION_STRUCTURE(Set, Name) layout(set = Set) uniform accelerationStructureEXT Name;
#else
    #define PK_DECLARE_ACCELERATION_STRUCTURE(Set, Name)
#endif

#endif