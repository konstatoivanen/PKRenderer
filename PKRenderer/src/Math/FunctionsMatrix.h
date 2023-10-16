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
    float4x4 GetPerspectiveInvZ(float fov, float aspect, float zNear, float zFar);
    float4x4 GetOrtho(float left, float right, float bottom, float top, float zNear, float zFar);
    float4x4 GetOrthoInvZ(float left, float right, float bottom, float top, float zNear, float zFar);
    float3x4 TransposeTo3x4(const float4x4& matrix);
    float4x4 TransposeTo4x4(const float3x4& matrix);
    inline float GetZNearFromProj(const float4x4& matrix) { return -matrix[3][2] / (matrix[2][2] + 1.0f); }
    inline float GetZFarFromProj(const float4x4& matrix) { return -matrix[3][2] / (matrix[2][2] - 1.0f); }
    inline float GetSizeOnScreen(float depth, float sizePerDepth, float radius) { return radius / (sizePerDepth * depth); }
    float4x4 GetOffsetPerspective(float left, float right, float bottom, float top, float fovy, float aspect, float zNear, float zFar);
    float4x4 GetPerspectiveSubdivision(uint32_t index, const int3& gridSize, float fovy, float aspect, float znear, float zfar);
    float4x4 GetFrustumBoundingOrthoMatrix(const float4x4& worldToLocal, const float4x4& inverseViewProjection, const float3& paddingLD, const float3& paddingRU, float* outZnear, float* outZFar);
    float4x4 GetPerspectiveJittered(const float4x4& matrix, const float2& jitter);

    struct ShadowCascadeCreateInfo
    {
        float4x4 worldToLocal;
        float4x4 projToWorld;
        float* splitPlanes;
        float zPadding;
        uint32_t resolution;
        uint32_t count;
    };

    void GetShadowCascadeMatrices(const ShadowCascadeCreateInfo params, float4x4* outMatrices, float* outRange = nullptr);
}