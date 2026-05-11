#pragma once
#include "Pool.h"

namespace PK
{
    template<typename TAllocation, typename TMask>
    struct RangeTable : NoCopy
    {
        struct Range
        {
            Range* next;
            size_t start;
            size_t end;
            Range(Range* next, size_t start, size_t end) : next(next), start(start), end(end) {}
        };

        constexpr RangeTable() noexcept : ranges(), baseIndex(0ul) { }
        RangeTable(size_t capacity) noexcept : RangeTable() { Resize(capacity); }
        RangeTable(RangeTable&& other) noexcept : RangeTable() { Move(PK::Forward<RangeTable>(other)); }
        RangeTable(const RangeTable& other) noexcept : RangeTable() { Copy(other); }
       
        constexpr size_t GetCapacity() const { return ranges.GetCapacity(); }

        RangeTable& operator=(RangeTable&& other) noexcept { Move(PK::Forward<RangeTable>(other)); return *this; }
        RangeTable& operator=(const RangeTable& other) noexcept { Copy(other); return *this; }

        inline void Copy(const RangeTable& other)
        {
            Resize(other.GetCapacity());
            ranges.Copy(other.ranges);
            baseIndex = other.baseIndex;
        }

        inline void Move(RangeTable&& other)
        {
            if (this != &other)
            {
                ranges.Move(other.ranges);
                baseIndex = PK::Exchange(other.baseIndex, 0u);
            }
        }

        void Resize(size_t newCapacity) 
        {
            ranges.Reserve(newCapacity); 
        }

        void Reserve(size_t start, size_t end)
        {
            auto base = baseIndex ? ranges[baseIndex - 1u] : nullptr;
            auto next = &base;
    
            for (auto curr = *next; curr && curr->start <= end; curr = *next)
            {
                if (curr->end < start)
                {
                    next = &curr->next;
                    continue;
                }
    
                start = start < curr->start ? start : curr->start;
                end = end > curr->end ? end : curr->end;
                *next = curr->next;
                ranges.Delete(curr);
            }
    
            *next = ranges.New(*next, start, end);
            baseIndex = ranges.GetIndex(base) + 1u;
        }
    
        void Unreserve(size_t start, size_t end)
        {
            auto base = baseIndex ? ranges[baseIndex - 1u] : nullptr;
            auto next = &base;

            for (auto curr = *next; curr && curr->start < end; curr = *next)
            {
                if (curr->end < start)
                {
                    next = &curr->next;
                    continue;
                }
    
                // encapsulated
                if (curr->start >= start && curr->end <= end)
                {
                    *next = curr->next;
                    ranges.Delete(curr);
                    continue;
                }
    
                // bottom clip
                if (curr->start >= start && curr->end > end)
                {
                    curr->start = end;
                    next = &curr->next;
                    continue;
                }
    
                // top clip
                if (curr->start < start && curr->end >= end)
                {
                    curr->end = start;
                    next = &curr->next;
                    continue;
                }
    
                // splice
                *next = ranges.New(curr, curr->start, start);
                curr->start = curr->end;
                curr->end = end;
                break;
            }

            baseIndex = base ? ranges.GetIndex(base) + 1u : 0u;
        }
    
        size_t FindFreeOffset(size_t size)
        {
            auto base = baseIndex ? ranges[baseIndex - 1u] : nullptr;
            auto head = 0ull;
    
            for (auto curr = base; curr && (curr->start - head) < size; curr = curr->next)
            {
                head = curr->end;
            }
    
            return head;
        }
    
        bool IsReservedAny(size_t start, size_t end)
        {
            auto base = baseIndex ? ranges[baseIndex - 1u] : nullptr;

            for (auto curr = base; curr; curr = curr->next)
            {
                if (curr->start > end) return false;
                if (curr->end > start) return true;
            }
    
            return false;
        }
    
        Pool<Range, TAllocation, TMask> ranges;
        uint32_t baseIndex = 0u;
    };

    template<size_t capacity>
    using FixedRangeTable = RangeTable<AllocationFixed<capacity>, FixedMask<capacity>>;

    template<size_t inline_capacity>
    using InlineRangeTable = RangeTable<AllocationInline<inline_capacity>, InlineMask<inline_capacity>>;

    using HeapRangeTable = RangeTable<AllocationHeap, HeapMask>;
}
