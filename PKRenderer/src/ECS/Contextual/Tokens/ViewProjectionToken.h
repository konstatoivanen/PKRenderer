#pragma once
#include "Math/Types.h"

namespace PK::ECS::Tokens
{
    struct ViewProjectionUpdateToken
    {
        Math::float4x4 view;
        Math::float4x4 projection;
        Math::float4 jitter;
    };
}