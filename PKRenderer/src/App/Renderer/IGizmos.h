#pragma once
#include "Core/Math/MathFwd.h"

namespace PK::App
{
    struct IGizmos
    {
        virtual ~IGizmos() = default;
        virtual void DrawBounds(const BoundingBox& aabb) = 0;
        virtual void DrawBox(const float3& origin, const float3& size) = 0;
        virtual void DrawLine(const float3& start, const float3& end) = 0;
        virtual void DrawRay(const float3& origin, const float3& vector) = 0;
        virtual void DrawFrustrum(const float4x4& matrix) = 0;
        virtual void SetColor(const color& color) = 0;
        virtual void SetMatrix(const float4x4& matrix) = 0;
    };
}