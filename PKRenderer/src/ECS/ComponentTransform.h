#pragma once
#include "Math/FunctionsMatrix.h"

namespace PK::ECS
{
    struct ComponentTransform
    {
        Math::float3 position = Math::PK_FLOAT3_ZERO;
        Math::quaternion rotation = Math::PK_QUATERNION_IDENTITY;
        Math::float3 scale = Math::PK_FLOAT3_ONE;
        Math::float4x4 localToWorld = Math::PK_FLOAT4X4_IDENTITY;
        Math::float4x4 worldToLocal = Math::PK_FLOAT4X4_IDENTITY;

        inline Math::float4x4 GetLocalToWorld() const { return Math::Functions::GetMatrixTRS(position, rotation, scale); }
        inline Math::float4x4 GetWorldToLocal() const { return Math::Functions::GetMatrixInvTRS(position, rotation, scale); }

        virtual ~ComponentTransform() = default;
    };
}