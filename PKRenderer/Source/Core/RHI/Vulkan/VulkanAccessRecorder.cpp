#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanAccessRecorder.h"

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

    inline static bool NeedsBarrier(const VulkanAccessRecorder::State& a, const VulkanAccessRecorder::State& b)
    {
        if (a.lastPass == b.lastPass || a.access == VK_ACCESS_2_NONE)
        {
            return false;
        }

        return VulkanEnumConvert::IsWriteAccess(a.access) || VulkanEnumConvert::IsWriteAccess(b.access);
    }

    inline static bool CanMerge(const VulkanAccessRecorder::State& a, const VulkanAccessRecorder::State& b)
    {
        if (VulkanEnumConvert::IsWriteAccess(a.access) || VulkanEnumConvert::IsWriteAccess(b.access))
        {
            return a.access == b.access && a.stage == b.stage && a.lastPass == b.lastPass && a.queueMask == b.queueMask;
        }

        return a.queueMask == b.queueMask;
    }

    void VulkanAccessRecorder::ResetPasses()
    {
        m_imageTransitionHead = 0u;
        m_passCount = 0u;
    }
    
    uint64_t VulkanAccessRecorder::AddPass()
    {
        PK_DEBUG_FATAL_ASSERT(m_passCount < PK_VK_MAX_ACCESS_PASSES, "VulkanAccessRecorder::PassBarriers pool exhausted!");
    
        if (m_passCount < PK_VK_MAX_ACCESS_PASSES)
        {
            auto passIndex = m_passCount++;
            auto* pass = &m_passes[passIndex];
            pass->imageTransitions = &m_imageTransitions[m_imageTransitionHead];
            pass->memoryBarrier = {};
            pass->memoryBarrier.sType = VK_STRUCTURE_TYPE_MAX_ENUM;
            pass->imageTransitionCount = 0u;
            pass->dependencyMask = 0u;
            m_passCounter++;
        }

        return m_passCount;
    }

    void VulkanAccessRecorder::RegisterBuffer(VkBuffer buffer)
    {
        auto index = 0u;
        if (buffer != VK_NULL_HANDLE && m_resources.AddKey((uint64_t)buffer, &index))
        {
            m_resources[index].value.aspectMask = 0u;
            m_resources[index].value.isNew = true;
            const auto node = AllocNode();
            
            if (node != NULL_NODE) 
            {
                auto* n = &m_nodes[node];
                n->buffer.offset = 0;
                n->buffer.size = VK_WHOLE_SIZE;
                n->state.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                n->state.access = VK_ACCESS_2_NONE;
                n->state.lastPass = 0u;
                n->state.queueMask = 0u;
                n->next = NULL_NODE;
                n->prev = NULL_NODE;
                m_resources[index].value.head = node;
            }
        }
    }
    
    void VulkanAccessRecorder::RegisterImage(VkImage image, VkImageAspectFlags aspectMask)
    {
        auto index = 0u;
        if (image != VK_NULL_HANDLE && m_resources.AddKey((uint64_t)image, &index))
        {
            m_resources[index].value.aspectMask = aspectMask;
            m_resources[index].value.isNew = true;
            const auto node = AllocNode();
    
            if (node != NULL_NODE) 
            {
                auto* n = &m_nodes[node];
                n->image.baseMipLevel = 0;
                n->image.levelCount = VK_REMAINING_MIP_LEVELS;
                n->image.baseArrayLayer = 0;
                n->image.layerCount = VK_REMAINING_ARRAY_LAYERS;
                n->state.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                n->state.access = VK_ACCESS_2_NONE;
                n->state.lastPass = 0u;
                n->state.queueMask = 0u;
                n->next = NULL_NODE;
                n->prev = NULL_NODE;
                m_resources[index].value.head = node;
            }
        }
    }

    void VulkanAccessRecorder::UnregisterResource(void* resource)
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

    void VulkanAccessRecorder::RecordBufferAccess(VkBuffer buffer, const VulkanAccessRecord& record)
    {
        auto* resource = m_resources.GetValuePtr((uint64_t)buffer);
        PK_DEBUG_FATAL_ASSERT(m_passCount > 0, "VulkanAccessRecorder::RecordBufferAccess called before calling VulkanAccessRecorder::AddPass!");
        PK_DEBUG_FATAL_ASSERT(resource != nullptr, "VulkanAccessRecorder::RecordBufferAccess called for an untracked resource!");

        const auto new_beg = record.buffer.offset;
        const auto new_end = AddU64(record.buffer.offset, record.buffer.size);

        State new_state;
        new_state.stage = record.stage;
        new_state.access = record.access;
        new_state.lastPass = m_passCounter;
        new_state.queueMask = 1u << record.queue;
        resource->isNew = false;

        for (auto curr = resource->head; curr != NULL_NODE; curr = m_nodes[curr].next)
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
                EmitMemoryBarrier(node->state, new_state);

                if (cur_beg < overlap_beg)
                {
                    const auto left = AllocNode();
                    m_nodes[left].buffer.offset = cur_beg;
                    m_nodes[left].buffer.size = overlap_beg - cur_beg;
                    m_nodes[left].state = node->state;
                    ListInsertBefore(&resource->head, curr, left);
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
                node->state.queueMask |= new_state.queueMask;
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
                    ListRemove(&resource->head, node->prev);
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
                    ListRemove(&resource->head, node->next);
                }
            }
        }
    }

    void VulkanAccessRecorder::RecordImageAccess(VkImage image, const VulkanAccessRecord& record)
    {
        auto* resource = m_resources.GetValuePtr((uint64_t)image);
        PK_DEBUG_FATAL_ASSERT(m_passCount > 0, "VulkanAccessRecorder::RecordImageAccess called before calling VulkanAccessRecorder::AddPass!");
        PK_DEBUG_FATAL_ASSERT(resource != nullptr, "VulkanAccessRecorder::RecordImageAccess called for an untracked resource!");

        const auto new_layer_beg = record.image.baseArrayLayer;
        const auto new_layer_end = AddU32(record.image.baseArrayLayer, record.image.layerCount);
        const auto new_level_beg = record.image.baseMipLevel;
        const auto new_level_end = AddU32(record.image.baseMipLevel, record.image.levelCount);

        // Initial and only layout transition from undefined to general.
        if (resource->isNew)
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
            barrier->subresourceRange.aspectMask = resource->aspectMask;
            barrier->subresourceRange.baseMipLevel = 0u;
            barrier->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier->subresourceRange.baseArrayLayer = 0u;
            barrier->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            m_passes[m_passCount - 1ull].imageTransitionCount++;
            resource->isNew = false;
        }

        State new_state;
        new_state.stage = record.stage;
        new_state.access = record.access;
        new_state.queueMask = 1u << record.queue;
        new_state.lastPass = m_passCounter;
        State accumulated_state = new_state;

        for (auto curr = resource->head; curr != NULL_NODE;)
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

            if (NeedsBarrier(node->state, new_state))
            {
                EmitMemoryBarrier(node->state, new_state);
            }
            else
            {
                accumulated_state.stage |= node->state.stage;
                accumulated_state.access |= node->state.access;
                accumulated_state.queueMask |= node->state.queueMask;
            }

            if (cur_level_beg < overlap_level_beg)
            {
                const auto top = AllocNode();
                m_nodes[top].image.baseMipLevel = cur_level_beg;
                m_nodes[top].image.levelCount = overlap_level_beg - cur_level_beg;
                m_nodes[top].image.baseArrayLayer = cur_layer_beg;
                m_nodes[top].image.layerCount = node->image.layerCount;
                m_nodes[top].state = node->state;
                ListInsertBefore(&resource->head, curr, top);
            }

            if (overlap_level_end < cur_level_end)
            {
                const auto bot = AllocNode();
                m_nodes[bot].image.baseMipLevel = overlap_level_end;
                m_nodes[bot].image.levelCount = SubU32(overlap_level_end, cur_level_end);
                m_nodes[bot].image.baseArrayLayer = cur_layer_beg;
                m_nodes[bot].image.layerCount = node->image.layerCount;
                m_nodes[bot].state = node->state;
                ListInsertBefore(&resource->head, curr, bot);
            }

            if (cur_layer_beg < overlap_layer_beg)
            {
                const auto left = AllocNode();
                m_nodes[left].image.baseMipLevel = overlap_level_beg;
                m_nodes[left].image.levelCount = SubU32(overlap_level_beg, overlap_level_end);
                m_nodes[left].image.baseArrayLayer = cur_layer_beg;
                m_nodes[left].image.layerCount = overlap_layer_beg - cur_layer_beg;
                m_nodes[left].state = node->state;
                ListInsertBefore(&resource->head, curr, left);
            }

            if (overlap_layer_end < cur_layer_end)
            {
                const auto right = AllocNode();
                m_nodes[right].image.baseMipLevel = overlap_level_beg;
                m_nodes[right].image.levelCount = SubU32(overlap_level_beg, overlap_level_end);
                m_nodes[right].image.baseArrayLayer = overlap_layer_end;
                m_nodes[right].image.layerCount = SubU32(overlap_layer_end, cur_layer_end);
                m_nodes[right].state = node->state;
                ListInsertBefore(&resource->head, curr, right);
            }

            ListRemove(&resource->head, curr);
            curr = node->next;
        }

        const auto final_node = AllocNode();
        m_nodes[final_node].image.baseMipLevel = record.image.baseMipLevel;
        m_nodes[final_node].image.levelCount = record.image.levelCount;
        m_nodes[final_node].image.baseArrayLayer = record.image.baseArrayLayer;
        m_nodes[final_node].image.layerCount = record.image.layerCount;
        m_nodes[final_node].state = accumulated_state;
        
        if (resource->head == NULL_NODE)
        {
            resource->head = final_node;
        }
        else
        {
            ListInsertBefore(&resource->head, resource->head, final_node);
        }
    }

    VulkanPassDependencies VulkanAccessRecorder::GetPassDependencies(uint32_t passIndex)
    {
        VulkanPassDependencies dependencies{};
        
        if (passIndex <= m_passCount && passIndex > 0u)
        {
            const auto* pass = &m_passes[passIndex - 1u];
            dependencies.memoryBarrier = pass->memoryBarrier.sType != VK_STRUCTURE_TYPE_MAX_ENUM ? &pass->memoryBarrier : nullptr;
            dependencies.imageTransitions = pass->imageTransitions;
            dependencies.imageTransitionCount = pass->imageTransitionCount;
            dependencies.dependencyMask = pass->dependencyMask;
        }

        return dependencies;
    }

    void VulkanAccessRecorder::EmitMemoryBarrier(const State& src, const State& dst)
    {
        auto* pass = &m_passes[m_passCount - 1ull];
        pass->memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        pass->memoryBarrier.srcStageMask |= src.stage;
        pass->memoryBarrier.srcAccessMask |= src.access;
        pass->memoryBarrier.dstStageMask |= dst.stage;
        pass->memoryBarrier.dstAccessMask |= dst.access;
        pass->dependencyMask |= src.queueMask & ~dst.queueMask;
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
