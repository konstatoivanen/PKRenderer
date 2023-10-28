#pragma once
#include "Math/Types.h"

namespace PK::ECS::Tokens
{
    struct ViewProjectionUpdateToken
    {
        Math::float4x4 worldToView;
        Math::float4x4 viewToClip;
        Math::float4 jitter;
    };
}