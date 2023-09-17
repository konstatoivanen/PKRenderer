#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/Objects/CommandBuffer.h"

namespace PK::Rendering::Objects
{
    // @TODO Refactor ray tracing pipelines & sbts to allow for more flexible arrangment
    // This should be a temporary utility class for handling pipeline versioning
    struct ShaderBindingTable : public Utilities::NoCopy
    {
        Utilities::Ref<Buffer> buffer = nullptr;
        Structs::ShaderBindingTableInfo tableInfo{};
        uint64_t pipelineHash = 0ull;
        uint64_t variantIndex = 0ull;
        void Validate(CommandBuffer* cmd, Shader* shader);
        void Bind(CommandBuffer* cmd);
    };
}