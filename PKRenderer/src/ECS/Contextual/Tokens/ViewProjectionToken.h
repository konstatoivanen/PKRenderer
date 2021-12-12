#pragma once
#include "Math/PKMath.h"

namespace PK::ECS::Tokens
{
    using namespace PK::Math;

    struct ViewProjectionUpdateToken
    {
        float4x4 view;
        float4x4 projection;
    };
}