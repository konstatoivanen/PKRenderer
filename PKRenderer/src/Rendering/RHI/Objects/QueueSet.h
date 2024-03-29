#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"

namespace PK::Rendering::RHI::Objects
{
    struct QueueSet : public PK::Utilities::NoCopy
    {
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)QueueType::MaxCount;
        virtual CommandBuffer* GetCommandBuffer(QueueType type) = 0;
        virtual FenceRef GetFenceRef(QueueType type, int32_t submitOffset = 0) = 0;
        virtual CommandBuffer* Submit(QueueType type) = 0;
        virtual void Sync(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Wait(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Transfer(QueueType from, QueueType to) = 0;

        inline void Submit(QueueType type, CommandBuffer** commandBuffer)
        {
            *commandBuffer = Submit(type);
        }
    };
}