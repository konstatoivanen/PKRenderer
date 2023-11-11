#pragma once
#include "FixedPool.h"

namespace PK::Utilities
{
	template<size_t capacity>
	struct RangeTable : NoCopy
	{
		struct Range
		{
			Range* next;
			size_t start;
			size_t end;
			Range(Range* next, size_t start, size_t end) : next(next), start(start), end(end) {}
		};
	
		void Reserve(size_t start, size_t end)
		{
			auto next = &firstRange;
	
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
		}
	
		void Unreserve(size_t start, size_t end)
		{
			auto next = &firstRange;

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
		}
	
		size_t FindFreeOffset(size_t size)
		{
			auto head = 0ull;
	
			for (auto curr = firstRange; curr && (curr->start - head) < size; curr = curr->next)
			{
				head = curr->end;
			}
	
			return head;
		}
	
		bool IsReservedAny(size_t start, size_t end)
		{
			for (auto curr = firstRange; curr; curr = curr->next)
			{
				if (curr->start > end) return false;
				if (curr->end > start) return true;
			}
	
			return false;
		}
	
		Range* firstRange = nullptr;
		FixedPool<Range, capacity> ranges;
	};
}