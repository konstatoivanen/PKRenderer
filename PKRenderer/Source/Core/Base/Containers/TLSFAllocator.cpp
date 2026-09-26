#pragma once
#include "PrecompiledHeader.h"
#include "TLSFAllocator.h"

namespace PK
{
    // Sad math duplication, but I dont want this to be dependent on math.h
    inline static uint64_t Min(uint64_t a, uint64_t b) { return a < b ? a : b; }
    inline static uint64_t Max(uint64_t a, uint64_t b) { return a > b ? a : b; }

    constexpr uint64_t AlignUp(uint64_t val, uint64_t align) noexcept
    {
        if (align <= 1ull)
        {
            return val;
        }
        
        const auto remainder = val % align;
        return remainder ? (val + align - remainder) : val;
    }

    static void SizeToBucketIndices(uint64_t size, uint32_t& out_lvl0, uint32_t& out_lvl1)
    {
        if (size < (1ull << TLSFAllocator::FL_INDEX_OFFSET))
        {
            out_lvl0 = 0u;
            out_lvl1 = static_cast<uint32_t>(size / TLSFAllocator::SIZE_MIN);
        }
        else
        {
            const auto idx_msb = static_cast<uint32_t>(Platform::ReverseBitScan64(size));
            out_lvl1 = static_cast<uint32_t>((size >> (idx_msb - TLSFAllocator::SL_INDEX_COUNT_LOG2)) ^ TLSFAllocator::SL_INDEX_COUNT);
            out_lvl0 = idx_msb - (TLSFAllocator::FL_INDEX_OFFSET - 1u);
        }

        out_lvl0 = static_cast<uint32_t>(Min(out_lvl0, TLSFAllocator::FL_INDEX_MAX - 1u));
        out_lvl1 = static_cast<uint32_t>(Min(out_lvl1, TLSFAllocator::SL_INDEX_COUNT - 1u));
    }

    static void SizeToSearchBucketIndices(uint64_t size, uint32_t& out_lvl0, uint32_t& out_lvl1)
    {
        if (size >= (1ull << TLSFAllocator::FL_INDEX_OFFSET))
        {
            const auto idx_msb = static_cast<uint32_t>(Platform::ReverseBitScan64(size));
            const auto alignment = (1ull << (idx_msb - TLSFAllocator::SL_INDEX_COUNT_LOG2)) - 1ull;
            size += alignment;
        }
        else
        {
            size += TLSFAllocator::SIZE_MIN - 1ull;
        }

        SizeToBucketIndices(size, out_lvl0, out_lvl1);
    }


    TLSFAllocator::TLSFAllocator(uint64_t size, size_t initialBlockCapacity)
    {
        ClearAndReserve(size, initialBlockCapacity);
    }

    void TLSFAllocator::ClearAndReserve(uint64_t size, size_t blockCapacity)
    {
        m_fl_mask = 0u;
        memset(m_sl_masks, 0, sizeof(m_sl_masks));
        memset(m_free_lists, 0, sizeof(m_free_lists));
        m_blocks.ClearFast();
        m_free_pool.ClearFast();
        m_blocks.Reserve(blockCapacity, false);
        m_free_pool.Reserve(blockCapacity, false);

        Block dummy_block{};
        m_blocks.Add(dummy_block);
        const auto idx_head = CreateBlock(0ull);
        const auto idx_tail = CreateBlock(size);
        m_blocks[idx_head].prev_physical = 0u;
        m_blocks[idx_head].next_physical = idx_tail;
        m_blocks[idx_tail].prev_physical = idx_head;
        m_blocks[idx_tail].next_physical = 0u;
        InsertFreeBlock(idx_head);
    }

    TLSFAllocator::Allocation TLSFAllocator::Allocate(uint64_t size, uint64_t alignment)
    {
        if (size == 0ull)
        {
            return {};
        }
    
        const auto size_search = Max(size + (Max(alignment, 1ull) - 1ull), SIZE_MIN);
        const auto idx_block = FindOptimalBlock(size_search);
    
        if (!idx_block)
        {
            return {};
        }
    
        RemoveFreeBlock(idx_block);
    
        const auto offset_cur = (uint64_t)m_blocks[idx_block].offset_quantized << 4ull;
        const auto offset_nxt = (uint64_t)m_blocks[m_blocks[idx_block].next_physical].offset_quantized << 4ull;
        const auto size_cur = offset_nxt - offset_cur;

        const auto offset_aligned = AlignUp(offset_cur, alignment);
        const auto size_alloc = AlignUp(Max(offset_aligned - offset_cur + size, SIZE_MIN), SIZE_MIN);

        if (size_cur >= size_alloc + SIZE_MIN)
        {
            const auto offset_split = offset_cur + size_alloc;
            const auto idx_split = CreateBlock(offset_split);
            auto& block_cur = m_blocks[idx_block];
            auto& block_spl = m_blocks[idx_split];
            block_spl.prev_physical = idx_block;
            block_spl.next_physical = block_cur.next_physical;

            if (block_cur.next_physical)
            {
                m_blocks[block_cur.next_physical].prev_physical = idx_split;
            }

            block_cur.next_physical = idx_split;
            InsertFreeBlock(idx_split);
        }

        Allocation alloc{};
        alloc.offset = offset_aligned;
        alloc.block_idx = idx_block; 
        return alloc;
    }

    void TLSFAllocator::Free(Allocation& alloc)
    {
        auto idx_block = alloc.block_idx;

        if (idx_block && idx_block < m_blocks.GetCount() && !m_blocks[idx_block].free)
        {
            if (m_blocks[m_blocks[idx_block].prev_physical].free)
            {
                const auto idx_prev = m_blocks[idx_block].prev_physical;
                RemoveFreeBlock(idx_prev);
                
                m_blocks[idx_prev].next_physical = m_blocks[idx_block].next_physical;

                if (m_blocks[idx_block].next_physical)
                {
                    m_blocks[m_blocks[idx_block].next_physical].prev_physical = idx_prev;
                }

                DestroyBlock(idx_block);
                idx_block = idx_prev;
            }

            if (m_blocks[idx_block].next_physical && m_blocks[m_blocks[idx_block].next_physical].free)
            {
                auto idx_next = m_blocks[idx_block].next_physical;
                RemoveFreeBlock(idx_next);

                m_blocks[idx_block].next_physical = m_blocks[idx_next].next_physical;

                if (m_blocks[idx_next].next_physical)
                {
                    m_blocks[m_blocks[idx_next].next_physical].prev_physical = idx_block;
                }

                DestroyBlock(idx_next);
            }

            InsertFreeBlock(idx_block);
            alloc = {};
        }
    }

    uint32_t TLSFAllocator::FindOptimalBlock(uint64_t size)
    {
        auto fl = 0u;
        auto sl = 0u;
        SizeToSearchBucketIndices(size, fl, sl);

        const auto sl_mask = m_sl_masks[fl] & (~0u << sl);

        if (sl_mask)
        {
            sl = static_cast<uint32_t>(Platform::BitScan64(sl_mask));
            return m_free_lists[fl][sl];
        }

        const auto fl_mask = (fl + 1u < FL_INDEX_MAX) ? (m_fl_mask & (~0u << (fl + 1u))) : 0u;

        if (fl_mask)
        {
            fl = static_cast<uint32_t>(Platform::BitScan64(fl_mask));
            sl = static_cast<uint32_t>(Platform::BitScan64(m_sl_masks[fl]));
            return m_free_lists[fl][sl];
        }

        return 0u;
    }

    void TLSFAllocator::InsertFreeBlock(uint32_t blockIdx)
    {
        auto& block = m_blocks[blockIdx];
        block.free = true;

        const auto offset_cur = (uint64_t)block.offset_quantized << 4ull;
        const auto offset_nxt = (uint64_t)m_blocks[block.next_physical].offset_quantized << 4ull;
        const auto size_block = offset_nxt - offset_cur;

        auto fl = 0u;
        auto sl = 0u;
        SizeToBucketIndices(size_block, fl, sl);

        block.next_free = m_free_lists[fl][sl];
        block.prev_free = 0u;

        if (m_free_lists[fl][sl])
        {
            m_blocks[m_free_lists[fl][sl]].prev_free = blockIdx;
        }

        m_free_lists[fl][sl] = blockIdx;
        m_sl_masks[fl] |= 1u << sl;
        m_fl_mask |= 1u << fl;
    }

    void TLSFAllocator::RemoveFreeBlock(uint32_t blockIdx)
    {
        auto& block = m_blocks[blockIdx];
        const auto offset_cur = (uint64_t)block.offset_quantized << 4ull;
        const auto offset_nxt = (uint64_t)m_blocks[block.next_physical].offset_quantized << 4ull;
        const auto size_block = offset_nxt - offset_cur;

        auto fl = 0u;
        auto sl = 0u;
        SizeToBucketIndices(size_block, fl, sl);

        if (block.prev_free)
        {
            m_blocks[block.prev_free].next_free = block.next_free;
        }
        else
        {
            m_free_lists[fl][sl] = block.next_free;
        }

        if (block.next_free)
        {
            m_blocks[block.next_free].prev_free = block.prev_free;
        }

        if (!m_free_lists[fl][sl])
        {
            m_sl_masks[fl] &= ~(1u << sl);

            if (!m_sl_masks[fl])
            {
                m_fl_mask &= ~(1u << fl);
            }
        }

        block.next_free = 0u;
        block.prev_free = 0u;
        block.free = false;
    }

    uint32_t TLSFAllocator::CreateBlock(uint64_t offset)
    {
        Block block{};
        block.offset_quantized = offset >> 4ull;
        block.free = false;
        uint32_t idx;

        if (m_free_pool.GetCount())
        {
            idx = m_free_pool.Pop();
            m_blocks[idx] = block;
        }
        else
        {
            idx = m_blocks.GetCount();
            m_blocks.Add(block);
        }

        return idx;
    }

    void TLSFAllocator::DestroyBlock(uint32_t blockIdx)
    {
        m_blocks[blockIdx] = {};
        m_free_pool.Add(blockIdx);
    }
}
