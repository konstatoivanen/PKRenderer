#pragma once
#include "Math/Types.h"

namespace PK::Rendering::Structs
{
    struct ImageUploadRange
    {
		size_t bufferOffset;
		uint32_t level;
		uint32_t layer;
		uint32_t layers;
		Math::uint3 offset;
		Math::uint3 extent;
    };
}
