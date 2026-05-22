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
    void InsertionSort(T* beg, T* end, const TPred& pred = TPred())
    {
        if (beg != end) 
        {
            for (auto mid = beg; ++mid != end;) 
            { 
                auto hole = mid;
                auto cur = PK::MoveTemp(*mid);

                if (pred(cur, *beg))
                { 
                    auto mend = mid;
                    auto mdst = ++hole;

                    while (beg != mend)
                    {
                        *--mdst = PK::MoveTemp(*--mend);
                    }

                    *beg = PK::MoveTemp(cur);
                }
                else 
                {
                    for (auto pre = hole; pred(cur, *(--pre)); hole = pre)
                    {
                        *hole = PK::MoveTemp(*pre); 
                    }

                    *hole = PK::MoveTemp(cur); 
                }
            }
        }
    }

    template<typename T, typename TPred>
    void Median3(T* beg, T* mid, T* end, const TPred& pred) 
    {
        if (pred(*mid,*beg))
        {
            PK::Swap(*mid, *beg);
        }

        if (pred(*end,*mid)) 
        { 
            PK::Swap(*end, *mid);

            if (pred(*mid,*beg)) 
            {
                PK::Swap(*mid, *beg);
            }
        }
    }

    template<typename T, typename TPred>
    void QuicksortIteration(T* beg, T* end, const TPred& pred, T** outbeg, T** outend)
    {
        auto mid = beg + ((end - beg) >> 1ull);
        const auto last = end - 1ull;
        const uint64_t count = last - beg;

        if (40ull < count)
        {
            const auto one = (count + 1ull) >> 3ull;
            const auto two = one << 1ull;
            Median3(beg, beg + one, beg + two, pred);
            Median3(mid - one, mid, mid + one, pred);
            Median3(last - two, last - one, last, pred);
            Median3(beg + one, mid, last - one, pred);
        }
        else
        {
            Median3(beg, mid, last, pred);
        }

        auto pbeg = mid;
        auto pend = pbeg + 1ull;

        while (beg < pbeg && !pred(*(pbeg - 1ull), *pbeg) && !pred(*pbeg, *(pbeg - 1ull)))
        {
            --pbeg;
        }

        while (pend < end && !pred(*pend, *pbeg) && !pred(*pbeg, *pend))
        {
            ++pend;
        }

        auto gbeg = pend;
        auto gend = pbeg;

        for (;;)
        {
            for (; gbeg < end; ++gbeg)
            {
                if (pred(*pbeg, *gbeg))
                {
                    continue;
                }
                else if (pred(*gbeg, *pbeg))
                {
                    break;
                }
                else if (pend != gbeg)
                {
                    PK::Swap(*pend, *gbeg);
                }
                
                ++pend;
            }

            for (; beg < gend; --gend)
            {
                if (pred(*(gend - 1ull),*pbeg))
                {
                    continue;
                }
                else if (pred(*pbeg,*(gend - 1ull)))
                {
                    break;
                }
                else if (--pbeg != (gend - 1ull))
                {
                    PK::Swap(*pbeg, *(gend - 1ull));
                }
            }

            if (gend == beg && gbeg == end)
            {
                break;
            }

            if (gend == beg)
            {
                if (pend != gbeg)
                {
                    PK::Swap(*pbeg, *pend);
                }

                ++pend;
                PK::Swap(*pbeg, *gbeg);
                ++pbeg;
                ++gbeg;
            }
            else if (gbeg == end)
            {
                if (--gend != --pbeg)
                {
                    PK::Swap(*gend, *pbeg);
                }

                PK::Swap(*pbeg, *(--pend));
            }
            else
            {
                PK::Swap(*gbeg, *(--gend));
                ++gbeg;
            }
        }

        *outbeg = pbeg;
        *outend = pend;
    }

    template<typename T, typename TPred = TLess<T>>
    void QuickSort(T* beg, T* end, const TPred& pred = TPred())
    {
        if (beg < end)
        {
            while (true)
            {
                T* pbeg = nullptr;
                T* pend = nullptr;
                QuicksortIteration(beg, end, pred, &pbeg, &pend);

                if (pbeg - beg < end - pend)
                {
                    QuickSort(beg, pbeg, pred);
                    beg = pend;
                }
                else
                {
                    QuickSort(pend, end, pred);
                    end = pbeg;
                }
            }
        }
    }

    template<typename T, typename TPred = TLess<T>>
    void IntroSort(T* beg, T* end, const TPred& pred = TPred())
    {
        if (beg < end)
        {
            while(true)
            {
                if (end - beg <= 16ll) 
                {
                    InsertionSort(beg, end, pred);
                    return;
                }

                T* pbeg = nullptr;
                T* pend = nullptr;
                QuicksortIteration(beg, end, pred, &pbeg, &pend);

                if (pbeg - beg < end - pend)
                { 
                    IntroSort(beg, pbeg, pred);
                    beg = pend;
                }
                else 
                { 
                    IntroSort(pend, end, pred);
                    end = pbeg;
                }
            }
        }
    }
}
