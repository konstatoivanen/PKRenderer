#pragma once
#include "Core/Math/Math.h"

namespace PK::App
{
    struct ComponentProjection
    {
        typedef enum
        {
            CustomMatrix,
            Perspective,
            OrthoGraphic
        }
        Mode;

        float4x4 customViewToClip = PK_FLOAT4X4_IDENTITY;
        AABB<float3> orthoBounds{};
        Mode mode = CustomMatrix;
        float fieldOfView = 90.0f;
        float zNear = 0.1f;
        float zFar = 100.0f;

        float4x4 ResolveProjectionMatrix(float aspect);

        virtual ~ComponentProjection() = default;
    };
}
