#pragma once
#include "Core/Math/Math.h"
#include "Core/Math/Transform.h"

namespace PK::App
{
    struct ComponentTransform
    {
        float3 position = PK_FLOAT3_ZERO;
        quaternion rotation = PK_QUATERNION_IDENTITY;
        float3 scale = PK_FLOAT3_ONE;

        float3x4 localToWorld = PK_FLOAT3X4_IDENTITY;
        float4x4 worldToLocal = PK_FLOAT4X4_IDENTITY;
        float minUniformScale = 1.0f;

        inline float3x4 GetLocalToWorld() const { return math::transformTRS3x4(position, rotation, scale); }
        inline float4x4 GetWorldToLocal() const { return math::transformTRSInverse(position, rotation, scale); }

        virtual ~ComponentTransform() = default;
    };
}
