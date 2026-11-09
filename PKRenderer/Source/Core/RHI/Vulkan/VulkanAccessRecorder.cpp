#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanAccessRecorder.h"

namespace PK
{
    constexpr const static uint32_t NULL_NODE = UINT32_MAX;
    constexpr const static uint8_t NULL_QUEUE = 0xFFu;

    inline static uint64_t MinU64(uint64_t a, uint64_t b) { return a < b ? a : b; }
    inline static uint64_t MaxU64(uint64_t a, uint64_t b) { return a > b ? a : b; }
    inline static uint32_t MinU32(uint32_t a, uint32_t b) { return a < b ? a : b; }
    inline static uint32_t MaxU32(uint32_t a, uint32_t b) { return a > b ? a : b; }
    inline static uint64_t AddU64(uint64_t offset, uint64_t size) { return size == ~0ull ? ~0ull : (offset + size); }
    inline static uint64_t SubU64(uint64_t beg, uint64_t end) { return end == ~0ull ? ~0ull : (end - beg); }
    inline static uint32_t AddU32(uint32_t offset, uint32_t size) { return size == ~0u ? ~0u : (offset + size); }
    inline static uint32_t SubU32(uint32_t beg, uint32_t end) { return end == ~0u ? ~0u : (end - beg); }

    inline static void ClearState(VulkanAccessRecorder::State& state)
    {
        state = {};
        state.writeQueue = NULL_QUEUE;
    }

    inline static void MergeState(VulkanAccessRecorder::State& dst, const VulkanAccessRecorder::State& src)
    {
        if (src.writePass)
        {
            dst.writeStage |= src.writeStage;
            dst.writeAccess |= src.writeAccess;

            if (src.writePass > dst.writePass)
            {
                dst.writePass = src.writePass;
                dst.writeQueue = src.writeQueue;
            }
        }

        dst.isLocked |= src.isLocked;
        dst.readMask |= src.readMask;

        for (auto queue = 0u; queue < (uint32_t)QueueType::EnumCount; ++queue)
        {
            if (src.readMask & (1u << queue))
            {
                dst.readStage[queue] |= src.readStage[queue];
                dst.readPass[queue] = MaxU32(dst.readPass[queue], src.readPass[queue]);
            }
        }
    }

    inline static bool CanMerge(const VulkanAccessRecorder::State& a, const VulkanAccessRecorder::State& b)
    {
        if (a.isLocked != b.isLocked || a.writeQueue != b.writeQueue || a.writePass != b.writePass || a.readMask != b.readMask)
        {
            return false;
        }

        if (a.writePass && (a.writeStage != b.writeStage || a.writeAccess != b.writeAccess))
        {
            return false;
        }

        for (auto queue = 0u; queue < (uint32_t)QueueType::EnumCount; ++queue)
        {
            if ((a.readMask & (1u << queue)) && a.readPass[queue] != b.readPass[queue])
            {
                return false;
            }
        }

        return true;
    }

    inline static bool ResetState(VulkanAccessRecorder::State& state, uint64_t passFlush)
    {
        state.isLocked = 0u;

        if (state.writePass <= passFlush)
        {
            state.writeStage = VK_PIPELINE_STAGE_2_NONE;
            state.writeAccess = VK_ACCESS_2_NONE;
            state.writePass = 0u;
            state.writeQueue = NULL_QUEUE;
        }

        for (auto queue = 0u; queue < (uint32_t)QueueType::EnumCount; ++queue)
        {
            if (state.readPass[queue] <= passFlush)
            {
                state.readStage[queue] = VK_PIPELINE_STAGE_2_NONE;
                state.readPass[queue] = 0u;
                state.readMask &= ~(1u << queue);
            }
        }

        return state.writePass == 0u && state.readMask == 0u;
    }

    void VulkanAccessRecorder::ResetPasses(uint64_t flushedPassIndex)
    {
        m_imageTransitionHead = 0u;
        m_passCount = 0u;
        m_passFlush = MaxU64(m_passFlush, flushedPassIndex);
        m_passFirst = m_passCounter;
        memset(m_residencyWaitPasses, 0, sizeof(m_residencyWaitPasses));

        for (auto curr = m_memoryHead; curr != NULL_NODE; curr = m_nodes[curr].next)
        {
            auto* node = &m_nodes[curr];

            if (ResetState(node->state, m_passFlush) &&
                node->prev != NULL_NODE &&
                m_nodes[node->prev].state.writePass == 0u &&
                m_nodes[node->prev].state.readMask == 0u)
            {
                auto* prev = &m_nodes[node->prev];
                const auto currEnd = AddU64(node->memory.offset, node->memory.size);
                node->memory.offset = prev->memory.offset;
                node->memory.size = SubU64(prev->memory.offset, currEnd);
                ListRemove(&m_memoryHead, node->prev);
            }
        }

        for (auto i = 0u; i < m_images.GetCount(); ++i)
        {
            auto head = m_images[i].value.head;
            auto isIdle = true;

            for (auto curr = head; curr != NULL_NODE; curr = m_nodes[curr].next)
            {
                isIdle &= ResetState(m_nodes[curr].state, m_passFlush);
            }

            if (isIdle && head != NULL_NODE && m_nodes[head].next != NULL_NODE)
            {
                while (m_nodes[head].next != NULL_NODE)
                {
                    ListRemove(&m_images[i].value.head, m_nodes[head].next);
                }

                m_nodes[head].image.baseMipLevel = 0u;
                m_nodes[head].image.levelCount = VK_REMAINING_MIP_LEVELS;
                m_nodes[head].image.baseArrayLayer = 0u;
                m_nodes[head].image.layerCount = VK_REMAINING_ARRAY_LAYERS;
            }
        }
    }

    uint64_t VulkanAccessRecorder::AddPass()
    {
        PK_DEBUG_FATAL_ASSERT(m_passCount < PK_VK_MAX_ACCESS_PASSES, "VulkanAccessRecorder::PassBarriers pool exhausted!");

        if (m_passCount < PK_VK_MAX_ACCESS_PASSES)
        {
            auto passIndex = m_passCount++;
            m_passes[passIndex] = {};
            m_passes[passIndex].imageTransitions = &m_imageTransitions[m_imageTransitionHead];
            m_passes[passIndex].memoryBarrier.sType = VK_STRUCTURE_TYPE_MAX_ENUM;
            m_passCounter++;
        }

        return m_passCount;
    }

    void VulkanAccessRecorder::RegisterImage(VkImage image, VkImageAspectFlags aspectMask)
    {
        auto index = 0u;
        if (image != VK_NULL_HANDLE && m_images.AddKey((uint64_t)image, &index))
        {
            m_images[index].value.aspectMask = aspectMask;
            m_images[index].value.isNew = true;
            const auto node = AllocNode();

            if (node != NULL_NODE)
            {
                auto* n = &m_nodes[node];
                n->image.baseMipLevel = 0;
                n->image.levelCount = VK_REMAINING_MIP_LEVELS;
                n->image.baseArrayLayer = 0;
                n->image.layerCount = VK_REMAINING_ARRAY_LAYERS;
                n->next = NULL_NODE;
                n->prev = NULL_NODE;
                ClearState(n->state);
                m_images[index].value.head = node;
            }
        }
    }

    void VulkanAccessRecorder::UnregisterImage(VkImage image)
    {
        auto index = m_images.GetIndex((uint64_t)image);

        if (index != -1)
        {
            auto curr = m_images[index].value.head;

            while (curr != NULL_NODE)
            {
                auto next = m_nodes[curr].next;
                m_nodeResidency[curr] = false;
                curr = next;
            }

            m_images.RemoveAt(index);
        }
    }

    void VulkanAccessRecorder::RecordMemoryAccess(const VulkanAccessRecord& record)
    {
        PK_DEBUG_FATAL_ASSERT(m_passCount > 0, "VulkanAccessRecorder::RecordMemoryAccess called before calling VulkanAccessRecorder::AddPass!");

        if (m_memoryHead == NULL_NODE)
        {
            m_memoryHead = AllocNode();
            auto* root = &m_nodes[m_memoryHead];
            root->memory.offset = 0ull;
            root->memory.size = VK_WHOLE_SIZE;
            ClearState(root->state);
        }

        const auto new_beg = record.memory.address;
        const auto new_end = AddU64(record.memory.address, record.memory.size);

        for (auto curr = m_memoryHead; curr != NULL_NODE; curr = m_nodes[curr].next)
        {
            auto* node = &m_nodes[curr];
            const auto cur_beg = node->memory.offset;
            const auto cur_end = AddU64(node->memory.offset, node->memory.size);
            const auto overlap_beg = MaxU64(cur_beg, new_beg);
            const auto overlap_end = MinU64(cur_end, new_end);

            if (new_end != VK_WHOLE_SIZE && cur_beg >= new_end)
            {
                break;
            }

            if (overlap_beg < overlap_end)
            {
                if (cur_beg < overlap_beg)
                {
                    const auto left = AllocNode();
                    m_nodes[left].memory.offset = cur_beg;
                    m_nodes[left].memory.size = overlap_beg - cur_beg;
                    m_nodes[left].state = node->state;
                    ListInsertBefore(&m_memoryHead, curr, left);
                }

                if (overlap_end < cur_end)
                {
                    const auto right = AllocNode();
                    m_nodes[right].memory.offset = overlap_end;
                    m_nodes[right].memory.size = SubU64(overlap_end, cur_end);
                    m_nodes[right].state = node->state;
                    ListInsertAfter(curr, right);
                }

                node->memory.offset = overlap_beg;
                node->memory.size = SubU64(overlap_beg, overlap_end);
                ResolveAccess(node->state, record);

                if (node->prev != NULL_NODE)
                {
                    auto* prev = &m_nodes[node->prev];
                    if (CanMerge(prev->state, node->state))
                    {
                        const auto curr_end_new = AddU64(node->memory.offset, node->memory.size);
                        node->memory.offset = prev->memory.offset;
                        node->memory.size = SubU64(prev->memory.offset, curr_end_new);
                        MergeState(node->state, prev->state);
                        ListRemove(&m_memoryHead, node->prev);
                    }
                }

                if (node->next != NULL_NODE)
                {
                    auto* next = &m_nodes[node->next];
                    if (CanMerge(node->state, next->state))
                    {
                        const auto merged_end = AddU64(next->memory.offset, next->memory.size);
                        node->memory.size = SubU64(node->memory.offset, merged_end);
                        MergeState(node->state, next->state);
                        ListRemove(&m_memoryHead, node->next);
                    }
                }
            }
        }
    }

    void VulkanAccessRecorder::RecordImageAccess(VkImage image, const VulkanAccessRecord& record)
    {
        auto* imageHead = m_images.GetValuePtr((uint64_t)image);
        PK_DEBUG_FATAL_ASSERT(m_passCount > 0, "VulkanAccessRecorder::RecordImageAccess called before calling VulkanAccessRecorder::AddPass!");
        PK_DEBUG_FATAL_ASSERT(imageHead != nullptr, "VulkanAccessRecorder::RecordImageAccess called for an untracked image!");

        const auto new_layer_beg = record.image.baseArrayLayer;
        const auto new_layer_end = AddU32(record.image.baseArrayLayer, record.image.layerCount);
        const auto new_level_beg = record.image.baseMipLevel;
        const auto new_level_end = AddU32(record.image.baseMipLevel, record.image.levelCount);

        // Initial and only layout transition from undefined to general.
        if (imageHead->isNew)
        {
            auto barrier = &m_imageTransitions[m_imageTransitionHead++];
            barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier->pNext = nullptr;
            barrier->srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            barrier->srcAccessMask = VK_ACCESS_2_NONE;
            barrier->dstStageMask = record.stage;
            barrier->dstAccessMask = record.access;
            barrier->oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier->newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier->image = image;
            barrier->subresourceRange.aspectMask = imageHead->aspectMask;
            barrier->subresourceRange.baseMipLevel = 0u;
            barrier->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier->subresourceRange.baseArrayLayer = 0u;
            barrier->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            m_passes[m_passCount - 1ull].imageTransitionCount++;
            imageHead->isNew = false;
        }

        State accumulated_state;
        ClearState(accumulated_state);

        for (auto curr = imageHead->head; curr != NULL_NODE;)
        {
            auto* node = &m_nodes[curr];
            const auto next = node->next;
            const auto cur_layer_beg = node->image.baseArrayLayer;
            const auto cur_layer_end = AddU32(node->image.baseArrayLayer, node->image.layerCount);
            const auto cur_level_beg = node->image.baseMipLevel;
            const auto cur_level_end = AddU32(node->image.baseMipLevel, node->image.levelCount);
            const auto overlap_layer_beg = MaxU32(cur_layer_beg, new_layer_beg);
            const auto overlap_layer_end = MinU32(cur_layer_end, new_layer_end);
            const auto overlap_level_beg = MaxU32(cur_level_beg, new_level_beg);
            const auto overlap_level_end = MinU32(cur_level_end, new_level_end);

            if (overlap_layer_beg >= overlap_layer_end || overlap_level_beg >= overlap_level_end)
            {
                curr = next;
                continue;
            }

            if (cur_level_beg < overlap_level_beg)
            {
                const auto top = AllocNode();
                m_nodes[top].image.baseMipLevel = cur_level_beg;
                m_nodes[top].image.levelCount = overlap_level_beg - cur_level_beg;
                m_nodes[top].image.baseArrayLayer = cur_layer_beg;
                m_nodes[top].image.layerCount = node->image.layerCount;
                m_nodes[top].state = node->state;
                ListInsertBefore(&imageHead->head, curr, top);
            }

            if (overlap_level_end < cur_level_end)
            {
                const auto bot = AllocNode();
                m_nodes[bot].image.baseMipLevel = overlap_level_end;
                m_nodes[bot].image.levelCount = SubU32(overlap_level_end, cur_level_end);
                m_nodes[bot].image.baseArrayLayer = cur_layer_beg;
                m_nodes[bot].image.layerCount = node->image.layerCount;
                m_nodes[bot].state = node->state;
                ListInsertBefore(&imageHead->head, curr, bot);
            }

            if (cur_layer_beg < overlap_layer_beg)
            {
                const auto left = AllocNode();
                m_nodes[left].image.baseMipLevel = overlap_level_beg;
                m_nodes[left].image.levelCount = SubU32(overlap_level_beg, overlap_level_end);
                m_nodes[left].image.baseArrayLayer = cur_layer_beg;
                m_nodes[left].image.layerCount = overlap_layer_beg - cur_layer_beg;
                m_nodes[left].state = node->state;
                ListInsertBefore(&imageHead->head, curr, left);
            }

            if (overlap_layer_end < cur_layer_end)
            {
                const auto right = AllocNode();
                m_nodes[right].image.baseMipLevel = overlap_level_beg;
                m_nodes[right].image.levelCount = SubU32(overlap_level_beg, overlap_level_end);
                m_nodes[right].image.baseArrayLayer = overlap_layer_end;
                m_nodes[right].image.layerCount = SubU32(overlap_layer_end, cur_layer_end);
                m_nodes[right].state = node->state;
                ListInsertBefore(&imageHead->head, curr, right);
            }

            ResolveAccess(node->state, record);
            MergeState(accumulated_state, node->state);

            ListRemove(&imageHead->head, curr);
            curr = next;
        }

        const auto final_node = AllocNode();
        m_nodes[final_node].image.baseMipLevel = record.image.baseMipLevel;
        m_nodes[final_node].image.levelCount = record.image.levelCount;
        m_nodes[final_node].image.baseArrayLayer = record.image.baseArrayLayer;
        m_nodes[final_node].image.layerCount = record.image.layerCount;
        m_nodes[final_node].state = accumulated_state;

        if (imageHead->head == NULL_NODE)
        {
            imageHead->head = final_node;
        }
        else
        {
            ListInsertBefore(&imageHead->head, imageHead->head, final_node);
        }
    }

    VulkanPassDependencies VulkanAccessRecorder::GetPassDependencies(uint32_t passIndex)
    {
        VulkanPassDependencies dependencies{};

        if (passIndex <= m_passCount && passIndex > 0u)
        {
            auto* pass = &m_passes[passIndex - 1u];
            dependencies.memoryBarrier = pass->memoryBarrier.sType != VK_STRUCTURE_TYPE_MAX_ENUM ? &pass->memoryBarrier : nullptr;
            dependencies.imageTransitions = pass->imageTransitions;
            dependencies.imageTransitionCount = pass->imageTransitionCount;
            dependencies.waitStageMask = pass->waitStageMask;
            dependencies.waitPasses = pass->waitPasses;
        }

        return dependencies;
    }

    void VulkanAccessRecorder::ResolveAccess(State& state, const VulkanAccessRecord& record)
    {
        const auto currentPass = (uint32_t)m_passCounter;
        const auto isWrite = VulkanEnumConvert::IsWriteAccess(record.access);
        PK_DEBUG_FATAL_ASSERT(!state.isLocked || !isWrite, "Cannot write to a read-only locked resident resource during the same graph scope!");

        if (record.isLocked)
        {
            if (state.writePass)
            {
                EmitBarrier(state.writeQueue, state.writePass, state.writeStage, state.writeAccess, record.stage, record.access);
                EmitWait(state.writeQueue, state.writePass, record.stage);
                m_residencyWaitPasses[state.writeQueue] = MaxU32(m_residencyWaitPasses[state.writeQueue], state.writePass);
                ClearState(state);
            }

            state.isLocked = 1u;
            return;
        }

        // RAW & WAW: Synchronize against last write
        const auto dstStage = isWrite ? record.stage : (record.stage & ~state.readStage[record.queue]);

        if (state.writePass && state.writePass != currentPass && dstStage)
        {
            if (state.writeQueue == record.queue)
            {
                EmitBarrier(record.queue, state.writePass, state.writeStage, state.writeAccess, dstStage, record.access);
            }
            else
            {
                EmitWait(state.writeQueue, state.writePass, dstStage);
            }
        }

        if (!isWrite)
        {
            if (state.writePass != currentPass)
            {
                state.readStage[record.queue] |= record.stage;
                state.readPass[record.queue] = currentPass;
                state.readMask |= (1u << record.queue);
            }

            return;
        }

        // WAR: Synchronize against active readers since last write
        for (auto queue = 0u; queue < (uint32_t)QueueType::EnumCount; ++queue)
        {
            if ((state.readMask & (1u << queue)) && state.readPass[queue] != currentPass)
            {
                if (queue == record.queue)
                {
                    EmitBarrier(queue, state.readPass[queue], state.readStage[queue], VK_ACCESS_2_NONE, record.stage, VK_ACCESS_2_NONE);
                }
                else
                {
                    EmitWait(queue, state.readPass[queue], record.stage);
                }
            }
        }

        // Record new write state and clear readers
        if (state.writePass != currentPass || state.writeQueue != record.queue)
        {
            ClearState(state);
            state.writePass = currentPass;
            state.writeQueue = record.queue;
        }

        state.writeStage |= record.stage;
        state.writeAccess |= record.access;
    }

    void VulkanAccessRecorder::EmitBarrier(
        uint32_t srcQueue,
        uint32_t srcPass,
        VkPipelineStageFlags2 srcStage,
        VkAccessFlags2 srcAccess,
        VkPipelineStageFlags2 dstStage,
        VkAccessFlags2 dstAccess)
    {
        auto* pass = &m_passes[m_passCount - 1ull];
        pass->memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        pass->memoryBarrier.srcStageMask |= srcStage;
        pass->memoryBarrier.srcAccessMask |= srcAccess;
        pass->memoryBarrier.dstStageMask |= dstStage;
        pass->memoryBarrier.dstAccessMask |= dstAccess;
        pass->waitPasses[srcQueue] = MaxU32(pass->waitPasses[srcQueue], srcPass);
    }

    void VulkanAccessRecorder::EmitWait(uint32_t srcQueue, uint32_t srcPass, VkPipelineStageFlags2 dstStage)
    {
        auto* pass = &m_passes[m_passCount - 1ull];
        pass->waitStageMask |= dstStage;
        pass->waitPasses[srcQueue] = MaxU32(pass->waitPasses[srcQueue], srcPass);
    }


    uint32_t VulkanAccessRecorder::AllocNode()
    {
        auto index = (uint32_t)m_nodeResidency.FindFirstZero();
        PK_DEBUG_FATAL_ASSERT(index != NULL_NODE, "VulkanAccessRecorder::Node pool exhausted!");

        if (index != NULL_NODE)
        {
            m_nodeResidency[index] = true;
            m_nodes[index].next = NULL_NODE;
            m_nodes[index].prev = NULL_NODE;
        }

        return index;
    }

    void VulkanAccessRecorder::ListInsertBefore(uint32_t* head, uint32_t ref, uint32_t node)
    {
        auto* n = &m_nodes[node];
        auto* r = &m_nodes[ref];
        n->next = ref;
        n->prev = r->prev;

        if (r->prev != NULL_NODE)
        {
            m_nodes[r->prev].next = node;
        }
        else
        {
            *head = node;
        }

        r->prev = node;
    }

    void VulkanAccessRecorder::ListInsertAfter(uint32_t ref, uint32_t node)
    {
        auto* n = &m_nodes[node];
        auto* r = &m_nodes[ref];
        n->prev = ref;
        n->next = r->next;

        if (r->next != NULL_NODE)
        {
            m_nodes[r->next].prev = node;
        }

        r->next = node;
    }

    void VulkanAccessRecorder::ListRemove(uint32_t* head, uint32_t node)
    {
        auto* n = &m_nodes[node];

        if (n->prev != NULL_NODE)
        {
            m_nodes[n->prev].next = n->next;
        }
        else
        {
            *head = n->next;
        }

        if (n->next != NULL_NODE)
        {
            m_nodes[n->next].prev = n->prev;
        }

        m_nodeResidency[node] = false;
    }
}
