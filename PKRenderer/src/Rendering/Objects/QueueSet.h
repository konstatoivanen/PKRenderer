#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/CommandBuffer.h"

namespace PK::Rendering::Objects
{
    struct QueueSet : public PK::Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)Structs::QueueType::MaxCount;
        virtual void QueueResourceSync(Structs::QueueType source, Structs::QueueType destination) = 0;
        virtual void QueueWait(Structs::QueueType source, Structs::QueueType destination) = 0;
        virtual void SubmitCurrent(Structs::QueueType type) = 0;
        virtual Objects::CommandBuffer* GetCommandBuffer(Structs::QueueType type) = 0;
        virtual Structs::FenceRef GetFenceRef(Structs::QueueType type) = 0;
    };
}