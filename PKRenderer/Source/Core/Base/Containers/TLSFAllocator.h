#pragma once
#include "Core/Base/Containers/ArrayList.h"

namespace PK
{
    struct TLSFAllocator
    {
        constexpr static const uint32_t SL_INDEX_COUNT_LOG2 = 3u;
        constexpr static const uint32_t SL_INDEX_COUNT = 1u << SL_INDEX_COUNT_LOG2;
        constexpr static const uint32_t FL_INDEX_OFFSET = 7u;
        constexpr static const uint32_t FL_INDEX_MAX = 32u;
        constexpr static const uint64_t SIZE_MIN = (1ull << FL_INDEX_OFFSET) / SL_INDEX_COUNT;

        struct Block
        {
            uint64_t offset_quantized : 31 = 0ull; 
            uint64_t free : 1 = 0ull;
            uint64_t prev_physical : 24 = 0u;
            uint64_t next_physical : 24 = 0u;
            uint64_t prev_free : 24 = 0u;
            uint64_t next_free : 24 = 0u;
        };

        struct Allocation
        {
            uint64_t offset : 40 = 0;
            uint64_t block_idx : 24 = 0u;
            explicit operator bool() const noexcept { return block_idx; }
        };

        constexpr TLSFAllocator() = default;
        TLSFAllocator(uint64_t size, size_t initialBlockCapacity);
        ~TLSFAllocator() = default;

        void ClearAndReserve(uint64_t size, size_t blockCapacity);
        Allocation Allocate(uint64_t size, uint64_t alignment = 16ull);
        void Free(Allocation& alloc);

    private:
        uint32_t FindOptimalBlock(uint64_t size);
        void InsertFreeBlock(uint32_t blockIdx);
        void RemoveFreeBlock(uint32_t blockIdx);
        uint32_t CreateBlock(uint64_t offset);
        void DestroyBlock(uint32_t blockIdx);

        uint32_t m_fl_mask = 0u;
        uint32_t m_sl_masks[FL_INDEX_MAX]{};
        uint32_t m_free_lists[FL_INDEX_MAX][SL_INDEX_COUNT]{};
        HeapList<Block> m_blocks;
        HeapList<uint32_t> m_free_pool;
    };
}