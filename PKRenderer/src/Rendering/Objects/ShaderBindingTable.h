#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::Objects
{
    // @TODO Refactor ray tracing pipelines & sbts to allow for more flexible arrangment
    // This should be a temporary utility class for handling pipeline versioning
    struct ShaderBindingTable : public Utilities::NoCopy
    {
        RHI::Objects::BufferRef buffer = nullptr;
        RHI::ShaderBindingTableInfo tableInfo{};
        uint64_t pipelineHash = 0ull;
        uint64_t variantIndex = 0ull;
        void Validate(RHI::Objects::CommandBuffer* cmd, RHI::Objects::Shader* shader);
        void Bind(RHI::Objects::CommandBuffer* cmd);
    };
}