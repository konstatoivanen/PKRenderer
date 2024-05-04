#pragma once
#include "Math/Types.h"

namespace PK::Rendering::Geometry
{
    struct IGizmos
    {
        virtual ~IGizmos() = default;
        virtual void DrawBounds(const Math::BoundingBox& aabb) = 0;
        virtual void DrawBox(const Math::float3& origin, const Math::float3& size) = 0;
        virtual void DrawLine(const Math::float3& start, const Math::float3& end) = 0;
        virtual void DrawRay(const Math::float3& origin, const Math::float3& vector) = 0;
        virtual void DrawFrustrum(const Math::float4x4& matrix) = 0;
        virtual void SetColor(const Math::color& color) = 0;
        virtual void SetMatrix(const Math::float4x4& matrix) = 0;
    };
}