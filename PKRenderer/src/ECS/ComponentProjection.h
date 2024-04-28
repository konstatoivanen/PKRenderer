#pragma once
#include "Math/FunctionsMatrix.h"

namespace PK::ECS
{
    struct ComponentProjection
    {
        typedef enum
        {
            CustomMatrix,
            Perspective,
            OrthoGraphic
        } Mode;

        Mode mode = CustomMatrix;
        Math::float4x4 customViewToClip = Math::PK_FLOAT4X4_IDENTITY;
        Math::BoundingBox orthoBounds{};
        float fieldOfView = 90.0f;
        float zNear = 0.1f;
        float zFar = 100.0f;

        Math::float4x4 ResolveProjectionMatrix(float aspect);

        virtual ~ComponentProjection() = default;
    };
}