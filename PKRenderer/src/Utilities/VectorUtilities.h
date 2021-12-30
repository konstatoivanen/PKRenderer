#pragma once
#include <vector>

namespace PK::Utilities::Vector
{
    template<typename  T>
    struct Comparer
    {
        int operator()(T& a, T& b);
    };

    template<typename T>
    struct Bound
    {
        size_t operator()(T& a);
    };

    template<typename T>
    void UnorderedRemoveAt(std::vector<T>& v, uint32_t i)
    {
        auto n = (int32_t)v.size() - 1;

        if (i != n)
        {
            v[i] = std::move(v[n]);
        }

        v.pop_back();
    }

    // Unsafe for vectors with elements that have dealloc in destructors
    template<typename T>
    void OrderedRemoveAt(std::vector<T>& v, uint32_t i)
    {
        auto n = (int32_t)v.size() - 1;
        auto arr = v.data();

        if (i != n)
        {
            memmove(arr + i, arr + i + 1, sizeof(T) * (n - i));
        }

        v.pop_back();
    }

    template<typename T>
    void ValidateSize(std::vector<T>& v, size_t minSize)
    {
        if (v.size() >= minSize)
        {
            return;
        }

        auto newSize = v.size();

        if (newSize == 0)
        {
            newSize = 1;
        }

        while (newSize < minSize)
        {
            newSize <<= 1;
        }

        v.resize(newSize);
    }

    template<typename T>
    void QuickSort(T* arr, Comparer<T>& c, int32_t low, int32_t high)
    {
        int32_t i = low;
        int32_t j = high;
        auto& pivot = arr[(i + j) / 2];

        while (i <= j)
        {
            while (c(arr[i], pivot) < 0)
            {
                i++;
            }

            while (c(arr[j], pivot) > 0)
            {
                j--;
            }

            if (i <= j)
            {
                if (i < j)
                {
                    std::swap(arr[i], arr[j]);
                }

                i++;
                j--;
            }
        }

        if (j > low)
        {
            QuickSort(arr, c, low, j);
        }

        if (i < high)
        {
            QuickSort(arr, c, i, high);
        }
    }

    template<typename T>
    void QuickSort(std::vector<T>& v, uint32_t count)
    {
        if (count > 1)
        {
            Comparer<T> c{};
            QuickSort(v.data(), c, 0, (int32_t)count - 1);
        }
    }

    template<typename T>
    void QuickSort(std::vector<T>& v)
    {
        if (v.size() > 1)
        {
            Comparer<T> c{};
            QuickSort(v.data(), c, 0, (int32_t)v.size() - 1);
        }
    }

    template<typename T>
    static int LowerBound(std::vector<T>& v, uint32_t size)
    {
        Bound<T> b{};

        auto arr = v.data();
        int32_t n = (int32_t)v.size();
        int32_t l = 0;
        int32_t r = n - 1;

        while (l < r)
        {
            auto m = (l + r) / 2;
            auto c = b(arr[m]);

            if (size <= c)
            {
                r = m;
            }
            else
            {
                l = m + 1;
            }
        }

        if (l < n && b(arr[l]) < size)
        {
            ++l;
        }

        return l < 0 || l >= n ? -1 : l;
    }
};
