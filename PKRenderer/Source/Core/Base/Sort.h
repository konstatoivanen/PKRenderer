#pragma once
#include "Templates.h"

namespace PK
{

    template<typename T> 
    struct TLess 
    { 
        constexpr bool operator()(const T& a, const T& b) const 
        { 
            return a < b; 
        } 
    };

    template<typename T>
    struct TLessFunc
    {
        typedef bool (*TFunc)(const T& a, const T& b);
        TFunc function;

        constexpr TLessFunc(TFunc _function) : function(_function) {}

        constexpr bool operator()(const T& a, const T& b) const
        {
            return function(a, b);
        }
    };

    template<typename T, typename TPred = TLess<T>>
    void InsertionSort(T* begin, T* end, const TPred& predicate = TPred())
    {
        if (begin != end) 
        {
            for (auto middle = begin; ++middle != end;) 
            { 
                auto hole = middle;
                auto current = PK::MoveTemp(*middle);

                if (predicate(current, *begin))
                { 
                    auto moveEnd = middle;
                    auto moveDst = ++hole;

                    while (begin != moveEnd)
                    {
                        *--moveDst = PK::MoveTemp(*--moveEnd);
                    }

                    *begin = PK::MoveTemp(current);
                }
                else 
                {
                    for (auto previous = hole; predicate(current, *(--previous)); hole = previous)
                    {
                        *hole = PK::MoveTemp(*previous);
                    }

                    *hole = PK::MoveTemp(current);
                }
            }
        }
    }

    template<typename T, typename TPred>
    void Median3(T* begin, T* middle, T* end, const TPred& predicate) 
    {
        if (predicate(*middle,*begin))
        {
            PK::Swap(*middle, *begin);
        }

        if (predicate(*end,*middle)) 
        { 
            PK::Swap(*end, *middle);

            if (predicate(*middle,*begin)) 
            {
                PK::Swap(*middle, *begin);
            }
        }
    }

    template<typename T, typename TPred>
    void QuicksortIteration(T* begin, T* end, const TPred& predicate, T** outbegin, T** outend)
    {
        auto middle = begin + ((end - begin) >> 1ull);
        const auto last = end - 1ull;
        const uint64_t count = last - begin;

        if (40ull < count)
        {
            const auto one = (count + 1ull) >> 3ull;
            const auto two = one << 1ull;
            Median3(begin, begin + one, begin + two, predicate);
            Median3(middle - one, middle, middle + one, predicate);
            Median3(last - two, last - one, last, predicate);
            Median3(begin + one, middle, last - one, predicate);
        }
        else
        {
            Median3(begin, middle, last, predicate);
        }

        auto partitionBegin = middle;
        auto partitionEnd = partitionBegin + 1ull;

        while (begin < partitionBegin && 
            !predicate(*(partitionBegin - 1ull), *partitionBegin) && 
            !predicate(*partitionBegin, *(partitionBegin - 1ull)))
        {
            --partitionBegin;
        }

        while (partitionEnd < end && 
            !predicate(*partitionEnd, *partitionBegin) && 
            !predicate(*partitionBegin, *partitionEnd))
        {
            ++partitionEnd;
        }

        auto iteratorBegin = partitionEnd;
        auto iteratorEnd = partitionBegin;

        for (;;)
        {
            for (; iteratorBegin < end; ++iteratorBegin)
            {
                if (predicate(*partitionBegin, *iteratorBegin))
                {
                    continue;
                }
                else if (predicate(*iteratorBegin, *partitionBegin))
                {
                    break;
                }
                else if (partitionEnd != iteratorBegin)
                {
                    PK::Swap(*partitionEnd, *iteratorBegin);
                }
                
                ++partitionEnd;
            }

            for (; begin < iteratorEnd; --iteratorEnd)
            {
                if (predicate(*(iteratorEnd - 1ull),*partitionBegin))
                {
                    continue;
                }
                else if (predicate(*partitionBegin,*(iteratorEnd - 1ull)))
                {
                    break;
                }
                else if (--partitionBegin != (iteratorEnd - 1ull))
                {
                    PK::Swap(*partitionBegin, *(iteratorEnd - 1ull));
                }
            }

            if (iteratorEnd == begin && iteratorBegin == end)
            {
                break;
            }

            if (iteratorEnd == begin)
            {
                if (partitionEnd != iteratorBegin)
                {
                    PK::Swap(*partitionBegin, *partitionEnd);
                }

                PK::Swap(*partitionBegin, *iteratorBegin);
                ++partitionEnd;
                ++partitionBegin;
                ++iteratorBegin;
            }
            else if (iteratorBegin == end)
            {
                if (--iteratorEnd != --partitionBegin)
                {
                    PK::Swap(*iteratorEnd, *partitionBegin);
                }

                PK::Swap(*partitionBegin, *(--partitionEnd));
            }
            else
            {
                PK::Swap(*iteratorBegin, *(--iteratorEnd));
                ++iteratorBegin;
            }
        }

        *outbegin = partitionBegin;
        *outend = partitionEnd;
    }

    template<typename T, typename TPred = TLess<T>>
    void QuickSort(T* begin, T* end, const TPred& predicate = TPred())
    {
        if (begin < end)
        {
            while (true)
            {
                T* partitionBegin = nullptr;
                T* partitionEnd = nullptr;
                QuicksortIteration(begin, end, predicate, &partitionBegin, &partitionEnd);

                if (partitionBegin - begin < end - partitionEnd)
                {
                    QuickSort(begin, partitionBegin, predicate);
                    begin = partitionEnd;
                }
                else
                {
                    QuickSort(partitionEnd, end, predicate);
                    end = partitionBegin;
                }
            }
        }
    }

    template<typename T, typename TPred = TLess<T>>
    void IntroSort(T* begin, T* end, const TPred& predicate = TPred())
    {
        if (begin < end)
        {
            while(true)
            {
                if (end - begin <= 16ll) 
                {
                    InsertionSort(begin, end, predicate);
                    return;
                }

                T* partitionBegin = nullptr;
                T* partitionEnd = nullptr;
                QuicksortIteration(begin, end, predicate, &partitionBegin, &partitionEnd);

                if (partitionBegin - begin < end - partitionEnd)
                { 
                    IntroSort(begin, partitionBegin, predicate);
                    begin = partitionEnd;
                }
                else 
                { 
                    IntroSort(partitionEnd, end, predicate);
                    end = partitionBegin;
                }
            }
        }
    }
}
