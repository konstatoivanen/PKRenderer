#pragma once
#ifndef PK_UTILITIES
#define PK_UTILITIES

float pow2(float x) { return x * x; }
float pow3(float x) { return x * x * x; }
float pow4(float x) { return x * x * x * x; }
float pow5(float x) { return x * x * x * x * x; }
#define unlerp(a,b,value) (((value) - (a)) / ((b) - (a)))
#define unlerp_sat(a,b,value) saturate(((value) - (a)) / ((b) - (a)))
#define saturate(v) clamp(v, 0.0, 1.0)
#define lerp_true(x,y,s) ((x) + (s) * ((y) - (x)))
#define POW2(x) ((x) * (x))
#define POW3(x) ((x) * (x) * (x))
#define POW4(x) ((x) * (x) * (x) * (x))
#define POW5(x) ((x) * (x) * (x) * (x) * (x))
#define mod(x,y) ((x) - (y) * floor((x) / (y)))
#define mul(a,b) (a * b)
// @TODO Refactor math to produce correct 0-1 z matrices & remove this hack.
#define NORMALIZE_GL_Z gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0

#define GEqual(a, b) any(greaterThanEqual(a,b))
#define Equal(a, b) any(equal(a,b))
#define NotEqual(a, b) any(notEqual(a,b))
#define LEqual(a, b) any(lessThanEqual(a,b))
#define Less(a,b) any(lessThan(a,b))
#define Greater(a,b) any(greaterThan(a,b))

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

#define PK_DECLARE_ATOMIC_VARIABLE(ValueType, BufferName, Set) layout(std430, set = Set) buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_READONLY_VARIABLE(ValueType, BufferName, Set) layout(std430, set = Set) readonly buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_WRITEONLY_VARIABLE(ValueType, BufferName, Set) layout(std430, set = Set) writeonly buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_RESTRICTED_VARIABLE(ValueType, BufferName, Set) layout(std430, set = Set) restrict buffer BufferName { ValueType BufferName##_Data; }
#define PK_DECLARE_ATOMIC_RESTRICTED_READONLY_VARIABLE(ValueType, BufferName, Set) layout(std430, set = Set) restrict readonly buffer BufferName { ValueType BufferName##_Data; }

#define PK_BUFFER_DATA(BufferName, index) BufferName##_Data[index]
#define PK_ATOMIC_DATA(BufferName) BufferName##_Data
#endif