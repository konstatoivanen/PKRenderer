#pragma once

#define Quat_Identity float4(0,0,0,1)

float4 Quat_Conjugate(float4 q) { return float4(-q.x, -q.y, -q.z, q.w); }

float4 Quat_inverse(float4 q) { return Quat_Conjugate(q) / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); }

float4 Quat_Relative(float4 q0, float4 q1) { return q1 * Quat_inverse(q0); }

float4 Quat_Multiply(float4 q0, float4 q1) { return float4(q1.xyz * q0.w + q0.xyz * q1.w + cross(q0.xyz, q1.xyz), q0.w * q1.w - dot(q0.xyz, q1.xyz)); }

float3 Quat_MultiplyVector(float4 q, float3 v)
{
    const float3 t = 2 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

float4 Quat_AngleAxis(float angle, float3 axis)
{
    float s = sin(angle * 0.5);
    float c = cos(angle * 0.5);
    return float4(axis * s, c);
}
