#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/FenceRef.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::RHI::Objects
{
    struct QueueSet : public PK::Utilities::NoCopy
    {
        virtual ~QueueSet() = 0;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)QueueType::MaxCount;
        virtual CommandBuffer* GetCommandBuffer(QueueType type) = 0;
        virtual Utilities::FenceRef GetFenceRef(QueueType type, int32_t submitOffset = 0) = 0;
        virtual CommandBuffer* Submit(QueueType type) = 0;
        virtual void Sync(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Wait(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Transfer(QueueType from, QueueType to) = 0;
        inline void Submit(QueueType type, CommandBuffer** commandBuffer) { *commandBuffer = Submit(type); }
    };
}