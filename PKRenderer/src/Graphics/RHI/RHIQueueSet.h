#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/RHI/Structs.h"
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics::RHI
{
    struct RHIQueueSet : public PK::Utilities::NoCopy
    {
        virtual ~RHIQueueSet() = 0;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)QueueType::MaxCount;
        virtual RHICommandBuffer* GetCommandBuffer(QueueType type) = 0;
        virtual Utilities::FenceRef GetFenceRef(QueueType type, int32_t submitOffset = 0) = 0;
        virtual RHICommandBuffer* Submit(QueueType type) = 0;
        virtual void Sync(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Wait(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Transfer(QueueType from, QueueType to) = 0;
        inline void Submit(QueueType type, RHICommandBuffer** cmd) { *cmd = Submit(type); }
    };
}