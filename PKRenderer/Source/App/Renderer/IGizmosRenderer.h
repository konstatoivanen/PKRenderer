#pragma once
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    struct IGizmosRenderer
    {
        virtual ~IGizmosRenderer() = default;

        virtual void GizmosDrawBounds(const AABB<float3>& aabb) = 0;
        virtual void GizmosDrawBox(const float3& origin, const float3& size) = 0;
        virtual void GizmosDrawLine(const float3& start, const float3& end) = 0;
        virtual void GizmosDrawRay(const float3& origin, const float3& vector) = 0;
        virtual void GizmosDrawFrustrum(const float4x4& matrix) = 0;
        virtual void GizmosSetColor(const color& color) = 0;
        virtual void GizmosSetMatrix(const float4x4& matrix) = 0;

        virtual const float4x4& GizmosGetWorldToClipMatrix() const = 0;
        virtual const short4& GizmosGetRenderAreaRect() const = 0;
    };
}
