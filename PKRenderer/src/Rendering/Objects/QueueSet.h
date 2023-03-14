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
        virtual Objects::CommandBuffer* GetCommandBuffer(Structs::QueueType type) = 0;
        virtual Structs::FenceRef GetFenceRef(Structs::QueueType type, int32_t submitOffset = 0) = 0;
        virtual Objects::CommandBuffer* Submit(Structs::QueueType type) = 0;
        virtual void Sync(Structs::QueueType from, Structs::QueueType to, int32_t submitOffset = 0) = 0;

        inline void Submit(Structs::QueueType type, Objects::CommandBuffer** commandBuffer)
        {
            *commandBuffer = Submit(type);
        }
    };
}