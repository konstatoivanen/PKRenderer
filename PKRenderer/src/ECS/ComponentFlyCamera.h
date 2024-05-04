#pragma once
#include "Math/FunctionsMatrix.h"

namespace PK::ECS
{
    struct ComponentFlyCamera
    {
        Math::float3 snashotPosition;
        Math::float3 snashotRotation;
        Math::float3 eulerAngles;
        Math::float3 targetPosition;
        float moveSpeed;
        float moveSmoothing;
        float rotationSmoothing;
        float sensitivity;
        virtual ~ComponentFlyCamera() = default;
    };
}