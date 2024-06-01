#pragma once
#include "Core/Math/Math.h"

namespace PK::App
{
    struct ComponentFlyCamera
    {
        float3 snashotPosition;
        float3 snashotRotation;
        float3 eulerAngles;
        float3 targetPosition;
        float moveSpeed;
        float moveSmoothing;
        float rotationSmoothing;
        float sensitivity;
        virtual ~ComponentFlyCamera() = default;
    };
}