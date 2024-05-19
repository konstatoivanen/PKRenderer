#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/RHI/Layout.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    // @TODO Refactor ray tracing pipelines & sbts to allow for more flexible arrangment
    // This should be a temporary utility class for handling pipeline versioning
    struct ShaderBindingTable : public Utilities::NoCopy
    {
        BufferRef buffer = nullptr;
        RHI::ShaderBindingTableInfo tableInfo{};
        uint64_t pipelineHash = 0ull;
        uint64_t variantIndex = 0ull;
        void Validate(CommandBufferExt cmd, Shader* shader);
    };
}