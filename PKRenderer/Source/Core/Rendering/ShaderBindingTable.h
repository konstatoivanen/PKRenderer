#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    // @TODO Refactor ray tracing pipelines & sbts to allow for more flexible arrangment
    // This should be a temporary utility class for handling pipeline versioning
    struct ShaderBindingTable : public NoCopy
    {
        RHIBufferRef buffer = nullptr;
        ShaderBindingTableInfo tableInfo{};
        uint64_t pipelineHash = 0ull;
        uint64_t variantIndex = 0ull;
        void Validate(CommandBufferExt cmd, ShaderAsset* shader);
    };
}