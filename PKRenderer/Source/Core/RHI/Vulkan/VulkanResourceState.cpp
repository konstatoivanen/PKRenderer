#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanResourceState.h"

namespace PK
{
    constexpr const static uint32_t NULL_NODE = UINT32_MAX;

    inline static uint64_t MinU64(uint64_t a, uint64_t b) { return a < b ? a : b; }
    inline static uint64_t MaxU64(uint64_t a, uint64_t b) { return a > b ? a : b; }
    inline static uint32_t MinU32(uint32_t a, uint32_t b) { return a < b ? a : b; }
    inline static uint32_t MaxU32(uint32_t a, uint32_t b) { return a > b ? a : b; }
    inline static uint64_t AddU64(uint64_t offset, uint64_t size) { return size == ~0ull ? ~0ull : (offset + size); }
    inline static uint64_t SubU64(uint64_t beg, uint64_t end) { return end == ~0ull ? ~0ull : (end - beg); }
    inline static uint32_t AddU32(uint32_t offset, uint32_t size) { return size == ~0u ? ~0u : (offset + size); }
    inline static uint32_t SubU32(uint32_t beg, uint32_t end) { return end == ~0u ? ~0u : (end - beg); }

    inline static bool IsTransfer(uint32_t srcQueue, uint32_t dstQueue)
    {
        return srcQueue != VK_QUEUE_FAMILY_IGNORED &&
               dstQueue != VK_QUEUE_FAMILY_IGNORED &&
               srcQueue != dstQueue;
    }
    
    inline static bool NeedsBarrier(const VulkanResourceState::State& a, const VulkanResourceState::State& b)
    {
        if (a.lastPass == b.lastPass)
        {
            // Only emit a barrier inside the same pass if an image was first accessed without layout (undefined)
            // and is now accessed with layout requirement (general).
            return !a.hasLayout && b.hasLayout;
        }

        return a.hasLayout != b.hasLayout ||
               IsTransfer(a.queue, b.queue) ||
               VulkanEnumConvert::IsWriteAccess(a.access) ||
               VulkanEnumConvert::IsWriteAccess(b.access);
    }
    
    inline static bool CanMerge(const VulkanResourceState::State& a, const VulkanResourceState::State& b)
    {
        if (a.hasLayout != b.hasLayout || IsTransfer(a.queue, b.queue))
        {
            return false;
        }

        if (VulkanEnumConvert::IsWriteAccess(a.access) || VulkanEnumConvert::IsWriteAccess(b.access))
        {
            return a.access == b.access && a.stage == b.stage;
        }

        return true;
    }

    uint32_t VulkanResourceState::AllocNode()
    {
        auto index = (uint32_t)m_nodeResidency.FindFirstZero();
        PK_DEBUG_FATAL_ASSERT(index != NULL_NODE, "VulkanResourceState::Node pool exhausted!");

        if (index != NULL_NODE)
        {
            m_nodeResidency[index] = true;
        }

        return index;
    }

    void VulkanResourceState::ListInsertBefore(uint32_t* head, uint32_t ref, uint32_t node)
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
    
    void VulkanResourceState::ListInsertAfter(uint32_t ref, uint32_t node)
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
    
    void VulkanResourceState::ListRemove(uint32_t* head, uint32_t node)
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
    

    void VulkanResourceState::ResetPasses()
    {
        m_bufferBarrierHead = 0u;
        m_imageBarrierHead = 0u;
        m_passCount = 0u;
    }
    
    uint64_t VulkanResourceState::AddPass(bool coalesce)
    {
        PK_DEBUG_FATAL_ASSERT(m_passCount < MAX_PASSES, "VulkanResourceState::PassBarriers pool exhausted!");
    
        if (m_passCount < MAX_PASSES)
        {
            auto passIndex = m_passCount++;
            auto* pass = &m_passes[passIndex];
            pass->bufferBarriers = &m_bufferBarriers[m_bufferBarrierHead];
            pass->imageBarriers = &m_imageBarriers[m_imageBarrierHead];
            pass->bufferBarrierCount = 0u;
            pass->imageBarrierCount = 0u;
            m_passCounter++;

            if (coalesce && m_isCoalescing)
            {
                pass->targetPass = m_coalescePass;
            }
            else
            {
                m_coalescePass = passIndex;
                m_isCoalescing = coalesce;
                pass->targetPass = passIndex;
            }
        }

        return m_passCount;
    }
    
    
    void VulkanResourceState::RegisterBuffer(VkBuffer buffer)
    {
        auto index = 0u;
        if (buffer != VK_NULL_HANDLE && m_resources.AddKey((uint64_t)buffer, &index))
        {
            m_resources[index].value.aspectMask = 0u;
            const auto node = AllocNode();
            
            if (node != NULL_NODE) 
            {
                auto* n = &m_nodes[node];
                n->buffer.offset = 0;
                n->buffer.size = VK_WHOLE_SIZE;
                n->state.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                n->state.access = VK_ACCESS_2_NONE;
                n->state.hasLayout = false;
                n->state.lastPass = 0u;
                n->state.queue = VK_QUEUE_FAMILY_IGNORED;
                n->next = NULL_NODE;
                n->prev = NULL_NODE;
                m_resources[index].value.head = node;
            }
        }
    }
    
    void VulkanResourceState::RegisterImage(VkImage image, VkImageAspectFlags aspectMask)
    {
        auto index = 0u;
        if (image != VK_NULL_HANDLE && m_resources.AddKey((uint64_t)image, &index))
        {
            m_resources[index].value.aspectMask = aspectMask;
            const auto node = AllocNode();
    
            if (node != NULL_NODE) 
            {
                auto* n = &m_nodes[node];
                n->image.baseMipLevel = 0;
                n->image.levelCount = VK_REMAINING_MIP_LEVELS;
                n->image.baseArrayLayer = 0;
                n->image.layerCount = VK_REMAINING_ARRAY_LAYERS;
                n->state.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                n->state.access= VK_ACCESS_2_NONE;
                n->state.hasLayout = false;
                n->state.lastPass = 0u;
                n->state.queue = VK_QUEUE_FAMILY_IGNORED;
                n->next = NULL_NODE;
                n->prev = NULL_NODE;
                m_resources[index].value.head = node;
            }
        }
    }

    void VulkanResourceState::UnregisterResource(void* resource)
    {
        auto handle = (uint64_t)resource;
        auto index = m_resources.GetIndex(handle);

        if (index != -1)
        {
            auto curr = m_resources[index].value.head;
            
            while (curr != NULL_NODE) 
            {
                auto next = m_nodes[curr].next;
                m_nodeResidency[curr] = false;
                curr = next;
            }
            
            m_resources.RemoveAt(index);
        }
    }

    void VulkanResourceState::EmitBufferBarrier(VkBuffer buffer, const State& src, const State& dst, VkDeviceSize beg, VkDeviceSize end)
    {
        auto passIndex = m_passCount - 1ull;
        auto targetPass = m_passes[passIndex].targetPass;
        auto* pass = &m_passes[targetPass];

        for (auto i = 0u; i < pass->bufferBarrierCount; ++i)
        {
            auto barrier = &pass->bufferBarriers[i];

            if (barrier->buffer == buffer)
            {
                auto bar_beg = barrier->offset;
                auto bar_end = AddU64(barrier->offset, barrier->size);
                auto res_beg = MinU64(beg, bar_beg);
                auto res_end = MaxU64(end, bar_end);

                if (MaxU64(beg, bar_beg) <= MinU64(end, bar_end))
                {
                    barrier->offset = res_beg;
                    barrier->size = SubU64(res_beg, res_end);
                    barrier->srcStageMask |= src.stage;
                    barrier->dstStageMask |= dst.stage;
                    barrier->srcAccessMask |= src.access;
                    barrier->dstAccessMask |= dst.access;
                    return;
                }
            }
        }

        {
            auto barrier = &m_bufferBarriers[m_bufferBarrierHead++];
            barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier->pNext = nullptr;
            barrier->srcStageMask = src.stage;
            barrier->srcAccessMask = src.access;
            barrier->dstStageMask = dst.stage;
            barrier->dstAccessMask = dst.access;
            barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier->buffer = buffer;
            barrier->offset = beg;
            barrier->size = SubU64(beg, end);
            pass->bufferBarrierCount++;
        }
    }

    void VulkanResourceState::EmitImageBarrier(VkImage image,
        VkImageAspectFlags aspect,
        const State& src,
        const State& dst,
        uint32_t layer_beg,
        uint32_t layer_end,
        uint32_t level_beg,
        uint32_t level_end)
    {
        auto passIndex = m_passCount - 1ull;
        auto targetPass = m_passes[passIndex].targetPass;
        auto* pass = &m_passes[targetPass];

        const auto is_transfer = IsTransfer(src.queue, dst.queue);
        const auto srcQ = is_transfer ? src.queue : VK_QUEUE_FAMILY_IGNORED;
        const auto dstQ = is_transfer ? dst.queue : VK_QUEUE_FAMILY_IGNORED;
        const auto srcLayout = src.hasLayout ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
        const auto dstLayout = dst.hasLayout ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;

        for (auto i = 0u; i < pass->imageBarrierCount; ++i)
        {
            auto barrier = &pass->imageBarriers[i];

            if (barrier->image == image &&
                barrier->oldLayout == srcLayout && 
                barrier->newLayout == dstLayout && 
                barrier->srcQueueFamilyIndex == srcQ &&
                barrier->dstQueueFamilyIndex == dstQ &&
                barrier->subresourceRange.aspectMask == aspect)
            {
                const auto bar_level_beg = barrier->subresourceRange.baseMipLevel;
                const auto bar_level_end = AddU32(bar_level_beg, barrier->subresourceRange.levelCount);
                const auto bar_layer_beg = barrier->subresourceRange.baseArrayLayer;
                const auto bar_layer_end = AddU32(bar_layer_beg, barrier->subresourceRange.layerCount);
                const auto level_overlap = MaxU32(level_beg, bar_level_beg) <= MinU32(level_end, bar_level_end);
                const auto layer_overlap = MaxU32(layer_beg, bar_layer_beg) <= MinU32(layer_end, bar_layer_end);

                if (level_overlap && layer_overlap)
                {
                    const auto res_level_beg = MinU32(level_beg, bar_level_beg);
                    const auto res_level_end = MaxU32(level_end, bar_level_end);
                    const auto res_layer_beg = MinU32(layer_beg, bar_layer_beg);
                    const auto res_layer_end = MaxU32(layer_end, bar_layer_end);

                    barrier->subresourceRange.baseMipLevel = res_level_beg;
                    barrier->subresourceRange.levelCount = SubU32(res_level_beg, res_level_end);
                    barrier->subresourceRange.baseArrayLayer = res_layer_beg;
                    barrier->subresourceRange.layerCount = SubU32(res_layer_beg, res_layer_end);

                    barrier->srcStageMask |= src.stage;
                    barrier->dstStageMask |= dst.stage;
                    barrier->srcAccessMask |= src.access;
                    barrier->dstAccessMask |= dst.access;
                    return;
                }
            }
        }

        {
            auto barrier = &m_imageBarriers[m_imageBarrierHead++];
            barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier->pNext = nullptr;
            barrier->srcStageMask = src.stage;
            barrier->srcAccessMask = src.access;
            barrier->dstStageMask = dst.stage;
            barrier->dstAccessMask = dst.access;
            barrier->oldLayout = srcLayout;
            barrier->newLayout = dstLayout;
            barrier->srcQueueFamilyIndex = srcQ;
            barrier->dstQueueFamilyIndex = dstQ;
            barrier->image = image;
            barrier->subresourceRange.aspectMask = aspect;
            barrier->subresourceRange.baseMipLevel = level_beg;
            barrier->subresourceRange.levelCount = SubU32(level_beg, level_end);
            barrier->subresourceRange.baseArrayLayer = layer_beg;
            barrier->subresourceRange.layerCount = SubU32(layer_beg, layer_end);
            pass->imageBarrierCount++;
        }
    }

    void VulkanResourceState::RecordBufferAccess(VkBuffer buffer, const VulkanAccessRecord& record)
    {
        auto* state = m_resources.GetValuePtr((uint64_t)buffer);
        PK_DEBUG_FATAL_ASSERT(m_passCount > 0, "VulkanResourceState::RecordBufferAccess called before calling VulkanResourceState::AddPass!");
        PK_DEBUG_FATAL_ASSERT(state != nullptr, "VulkanResourceState::RecordBufferAccess called for an untracked resource!");

        const auto new_beg = record.buffer.offset;
        const auto new_end = AddU64(record.buffer.offset, record.buffer.size);

        State new_state;
        new_state.stage = record.stage;
        new_state.access = record.access;
        new_state.hasLayout = false;
        new_state.lastPass = m_passCounter - 1u;
        new_state.queue = VK_QUEUE_FAMILY_IGNORED;

        for (auto curr = state->head; curr != NULL_NODE; curr = m_nodes[curr].next)
        {
            auto node = &m_nodes[curr];
            const auto cur_beg = node->buffer.offset;
            const auto cur_end = AddU64(node->buffer.offset, node->buffer.size);
            const auto overlap_beg = MaxU64(cur_beg, new_beg);
            const auto overlap_end = MinU64(cur_end, new_end);

            if (new_end != VK_WHOLE_SIZE && cur_beg >= new_end)
            {
                break;
            }

            if (overlap_beg >= overlap_end)
            {
                continue;
            }

            if (NeedsBarrier(node->state, new_state))
            {
                EmitBufferBarrier(buffer, node->state, new_state, overlap_beg, overlap_end);

                if (cur_beg < overlap_beg)
                {
                    const auto left = AllocNode();
                    m_nodes[left].buffer.offset = cur_beg;
                    m_nodes[left].buffer.size = overlap_beg - cur_beg;
                    m_nodes[left].state = node->state;
                    ListInsertBefore(&state->head, curr, left);
                }

                if (overlap_end < cur_end)
                {
                    const auto right = AllocNode();
                    m_nodes[right].buffer.offset = overlap_end;
                    m_nodes[right].buffer.size = SubU64(overlap_end, cur_end);
                    m_nodes[right].state = node->state;
                    ListInsertAfter(curr, right);
                }

                node->buffer.offset = overlap_beg;
                node->buffer.size = SubU64(overlap_beg, overlap_end);
                node->state = new_state;
            }
            else
            {
                node->state.stage |= new_state.stage;
                node->state.access |= new_state.access;
                node->state.lastPass = new_state.lastPass;
            }

            if (node->prev != NULL_NODE)
            {
                auto prev = &m_nodes[node->prev];

                if (CanMerge(prev->state, node->state))
                {
                    const auto curr_end_new = AddU64(node->buffer.offset, node->buffer.size);
                    node->buffer.offset = prev->buffer.offset;
                    node->buffer.size = SubU64(prev->buffer.offset, curr_end_new);
                    node->state.stage |= prev->state.stage;
                    node->state.access |= prev->state.access;
                    ListRemove(&state->head, node->prev);
                }
            }

            if (node->next != NULL_NODE)
            {
                auto* next = &m_nodes[node->next];

                if (CanMerge(node->state, next->state))
                {
                    const auto merged_end = AddU64(next->buffer.offset, next->buffer.size);
                    node->buffer.size = SubU64(node->buffer.offset, merged_end);
                    node->state.stage |= next->state.stage;
                    node->state.access |= next->state.access;
                    ListRemove(&state->head, node->next);
                }
            }
        }
    }

    bool VulkanResourceState::RecordImageAccess(VkImage image, const VulkanAccessRecord& record)
    {
        auto* state = m_resources.GetValuePtr((uint64_t)image);
        PK_DEBUG_FATAL_ASSERT(m_passCount > 0, "VulkanResourceState::RecordImageAccess called before calling VulkanResourceState::AddPass!");
        PK_DEBUG_FATAL_ASSERT(state != nullptr, "VulkanResourceState::RecordImageAccess called for an untracked resource!");

        const auto new_layer_beg = record.image.baseArrayLayer;
        const auto new_layer_end = AddU32(record.image.baseArrayLayer, record.image.layerCount);
        const auto new_level_beg = record.image.baseMipLevel;
        const auto new_level_end = AddU32(record.image.baseMipLevel, record.image.levelCount);

        State accumulated_state;
        accumulated_state.stage = record.stage;
        accumulated_state.access = record.access;
        accumulated_state.hasLayout = record.hasLayout;
        accumulated_state.queue = record.queue;
        accumulated_state.lastPass = m_passCounter - 1u;
        bool had_layout = true;

        for (auto curr = state->head; curr != NULL_NODE;)
        {
            auto* node = &m_nodes[curr];
            const auto next = node->next;
            const auto cur_layer_beg = node->image.baseArrayLayer;
            const auto cur_layer_end = AddU32(node->image.baseArrayLayer, node->image.layerCount);
            const auto cur_level_beg = node->image.baseMipLevel;
            const auto cur_level_end = AddU32(node->image.baseMipLevel, node->image.levelCount);
            const auto overlap_layer_beg = MaxU64(cur_layer_beg, new_layer_beg);
            const auto overlap_layer_end = MinU64(cur_layer_end, new_layer_end);
            const auto overlap_level_beg = MaxU64(cur_level_beg, new_level_beg);
            const auto overlap_level_end = MinU64(cur_level_end, new_level_end);

            if (overlap_layer_beg >= overlap_layer_end || overlap_level_beg >= overlap_level_end)
            {
                curr = next;
                continue;
            }

            if (NeedsBarrier(node->state, accumulated_state))
            {
                EmitImageBarrier(
                    image,
                    state->aspectMask,
                    node->state,
                    accumulated_state,
                    overlap_layer_beg,
                    overlap_layer_end,
                    overlap_level_beg,
                    overlap_level_end);
            }
            else
            {
                accumulated_state.stage |= node->state.stage;
                accumulated_state.access |= node->state.access;
            }

            if (cur_level_beg < overlap_level_beg)
            {
                const auto top = AllocNode();
                m_nodes[top].image.baseMipLevel = cur_level_beg;
                m_nodes[top].image.levelCount = overlap_level_beg - cur_level_beg;
                m_nodes[top].image.baseArrayLayer = cur_layer_beg;
                m_nodes[top].image.layerCount = node->image.layerCount;
                m_nodes[top].state = node->state;
                ListInsertBefore(&state->head, curr, top);
            }

            if (overlap_level_end < cur_level_end)
            {
                const auto bot = AllocNode();
                m_nodes[bot].image.baseMipLevel = overlap_level_end;
                m_nodes[bot].image.levelCount = SubU32(overlap_level_end, cur_level_end);
                m_nodes[bot].image.baseArrayLayer = cur_layer_beg;
                m_nodes[bot].image.layerCount = node->image.layerCount;
                m_nodes[bot].state = node->state;
                ListInsertBefore(&state->head, curr, bot);
            }

            if (cur_layer_beg < overlap_layer_beg)
            {
                const auto left = AllocNode();
                m_nodes[left].image.baseMipLevel = overlap_level_beg;
                m_nodes[left].image.levelCount = SubU32(overlap_level_beg, overlap_level_end);
                m_nodes[left].image.baseArrayLayer = cur_layer_beg;
                m_nodes[left].image.layerCount = overlap_layer_beg - cur_layer_beg;
                m_nodes[left].state = node->state;
                ListInsertBefore(&state->head, curr, left);
            }

            if (overlap_layer_end < cur_layer_end)
            {
                const auto right = AllocNode();
                m_nodes[right].image.baseMipLevel = overlap_level_beg;
                m_nodes[right].image.levelCount = SubU32(overlap_level_beg, overlap_level_end);
                m_nodes[right].image.baseArrayLayer = overlap_layer_end;
                m_nodes[right].image.layerCount = SubU32(overlap_layer_end, cur_layer_end);
                m_nodes[right].state = node->state;
                ListInsertBefore(&state->head, curr, right);
            }

            ListRemove(&state->head, curr);
            had_layout &= node->state.hasLayout;
            curr = node->next;
        }

        const auto final_node = AllocNode();
        m_nodes[final_node].image.baseMipLevel = record.image.baseMipLevel;
        m_nodes[final_node].image.levelCount = record.image.levelCount;
        m_nodes[final_node].image.baseArrayLayer = record.image.baseArrayLayer;
        m_nodes[final_node].image.layerCount = record.image.layerCount;
        m_nodes[final_node].state = accumulated_state;
        ListInsertBefore(&state->head, state->head, final_node);
        return had_layout;
    }

    void VulkanResourceState::GetPassBarriers(
        uint32_t passIdx,
        uint32_t* outBufferBarrierCount,
        const VkBufferMemoryBarrier2** outBufferBarriers,
        uint32_t* outImageBarrierCount,
        const VkImageMemoryBarrier2** outImageBarriers)
    {
        if (passIdx > m_passCount || passIdx == 0u) 
        {
            *outBufferBarrierCount = 0;
            *outBufferBarriers = nullptr;
            *outImageBarrierCount = 0;
            *outImageBarriers = nullptr;
            return;
        }
    
        const auto* pass = &m_passes[passIdx - 1u];
        *outBufferBarrierCount = pass->bufferBarrierCount;
        *outBufferBarriers = pass->bufferBarriers;
        *outImageBarrierCount = pass->imageBarrierCount;
        *outImageBarriers = pass->imageBarriers;
    }
}
