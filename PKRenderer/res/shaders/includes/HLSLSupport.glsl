#pragma once
#ifndef HLSL_SUPPORT
#define HLSL_SUPPORT

#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4

#define half float
#define half2 vec2
#define half3 vec3
#define half4 vec4

#define float float
#define float2 vec2
#define float3 vec3
#define float4 vec4

#define double float 
#define double2 vec2
#define double3 vec3
#define double4 vec4

#define short int
#define short2 ivec2
#define short3 ivec3
#define short4 ivec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define half2x2 mat2
#define half3x3 mat3
#define half4x4 mat4

#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4

#define double2x2 mat2
#define double3x3 mat3
#define double4x4 mat4

float pow2(float x) { return x * x; }
float pow3(float x) { return x * x * x; }
float pow4(float x) { return x * x * x * x; }
float pow5(float x) { return x * x * x * x * x; }

#define unlerp(a,b,value) (((value) - (a)) / ((b) - (a)))

#define unlerp_sat(a,b,value) saturate(((value) - (a)) / ((b) - (a)))

#define saturate(v) clamp(v, 0.0, 1.0)

#define lerp(a,b,v) mix(a,b,v)

#define lerp_true(x,y,s) ((x) + (s) * ((y) - (x)))

#define mul(a,b) (a * b)

#define POW2(x) ((x) * (x))

#define POW3(x) ((x) * (x) * (x))

#define POW4(x) ((x) * (x) * (x) * (x))

#define POW5(x) ((x) * (x) * (x) * (x) * (x))

#define mod(x,y) ((x) - (y) * floor((x) / (y)))

#define tex2D(a,b) texture(a,b)

#define tex2DLod(a,b,c) textureLod(a,b,c)

#define GEqual(a, b) any(greaterThanEqual(a,b))

#define Equal(a, b) any(equal(a,b))

#define NotEqual(a, b) any(notEqual(a,b))

#define LEqual(a, b) any(lessThanEqual(a,b))

#define Less(a,b) any(lessThan(a,b))

#define Greater(a,b) any(greaterThan(a,b))

#define PK_DECLARE_CBUFFER(BufferName) layout(std140) uniform BufferName

#define PK_DECLARE_BUFFER(ValueType, BufferName) layout(std430) buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_READONLY_BUFFER(ValueType, BufferName) layout(std430) readonly buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_WRITEONLY_BUFFER(ValueType, BufferName) layout(std430) writeonly buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_RESTRICTED_BUFFER(ValueType, BufferName) layout(std430) restrict buffer BufferName { ValueType BufferName##_Data[]; }
#define PK_DECLARE_RESTRICTED_READONLY_BUFFER(ValueType, BufferName) layout(std430) restrict readonly buffer BufferName { ValueType BufferName##_Data[]; }

#define PK_BUFFER_DATA(BufferName, index) BufferName##_Data[index]

#define PK_DECLARE_ATOMIC_VARIABLE(ValueType, BufferName) layout(std430) buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_READONLY_VARIABLE(ValueType, BufferName) layout(std430) readonly buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_WRITEONLY_VARIABLE(ValueType, BufferName) layout(std430) writeonly buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_RESTRICTED_VARIABLE(ValueType, BufferName) layout(std430) restrict buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_RESTRICTED_READONLY_VARIABLE(ValueType, BufferName) layout(std430) restrict readonly buffer BufferName { ValueType BufferName##_Data; }

#define PK_ATOMIC_DATA(BufferName) BufferName##_Data

#define PK_VERTEX_ID gl_VertexID

#endif