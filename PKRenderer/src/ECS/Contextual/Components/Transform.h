#pragma once
#include "Math/FunctionsMatrix.h"

namespace PK::ECS::Components
{
    using namespace PK::Math;

    struct Transform
    {
        float3 position = PK_FLOAT3_ZERO;
        quaternion rotation = PK_QUATERNION_IDENTITY;
        float3 scale = PK_FLOAT3_ONE;

        float4x4 localToWorld = PK_FLOAT4X4_IDENTITY;
        float4x4 worldToLocal = PK_FLOAT4X4_IDENTITY;

        inline float4x4 GetLocalToWorld() const { return Functions::GetMatrixTRS(position, rotation, scale); }
        inline float4x4 GetWorldToLocal() const { return Functions::GetMatrixInvTRS(position, rotation, scale); }

        virtual ~Transform() = default;
    };
}