#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    float4x4 GetMatrixTRS(const float3& position, const quaternion& rotation, const float3& scale);
    float4x4 GetMatrixTRS(const float3& position, const float3& euler, const float3& scale);
    float4x4 GetMatrixInvTRS(const float3& position, const quaternion& rotation, const float3& scale);
    float4x4 GetMatrixInvTRS(const float3& position, const float3& euler, const float3& scale);
    float4x4 GetMatrixTR(const float3& position, const quaternion& rotation);
    float4x4 GetPerspective(float fov, float aspect, float nearClip, float farClip);
    float4x4 GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
    float4x4 GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
    inline float GetZNearFromProj(const float4x4& matrix) { return -matrix[3][2] / (matrix[2][2] + 1.0f); }
    inline float GetZFarFromProj(const float4x4& matrix) { return -matrix[3][2] / (matrix[2][2] - 1.0f); }
    inline float GetSizeOnScreen(float depth, float sizePerDepth, float radius) { return radius / (sizePerDepth * depth); }
    float4x4 GetOffsetPerspective(float left, float right, float bottom, float top, float fovy, float aspect, float zNear, float zFar);
    float4x4 GetPerspectiveSubdivision(int index, const int3& gridSize, float fovy, float aspect, float znear, float zfar);
    float4x4 GetFrustumBoundingOrthoMatrix(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float3& paddingLD, const float3& paddingRU, float* outZnear, float* outZFar);
    float GetShadowCascadeMatrices(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float* zPlanes, float zPadding, uint count, float4x4* matrices);
}