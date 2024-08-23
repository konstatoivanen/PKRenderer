#pragma once
#include <exception>

namespace PK::ContainerHelpers
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

    #define PK_CONTAINER_RANGE_CHECK(index, min, max) if (index >= max || index < min) throw std::exception("Index/Count outside of container bounds!")

    template<typename T>
    void MoveArray(T* dst, T* src, size_t count)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy(dst, src, sizeof(T) * count);
        }
        else
        {
            for (auto i = 0u; i < count; ++i)
            {
                dst[i] = std::move(src[i]);
            }
        }
    }

    template<typename TValue, typename TNode> 
    void ReallocNodeValues(void** buffer, TValue** values, TNode** nodes, size_t size, size_t newSize)
    {
        auto offsetNode = (sizeof(TValue) * newSize + sizeof(TNode) - 1u) & ~(sizeof(TNode) - 1u);
        auto newBuffer = calloc(offsetNode + sizeof(TNode) * newSize, 1u);
        auto newValues = reinterpret_cast<TValue*>(newBuffer);
        auto newNodes = reinterpret_cast<TNode*>(reinterpret_cast<char*>(newBuffer) + offsetNode);

        if (*buffer)
        {
            ContainerHelpers::MoveArray(newValues, *values, size);
            ContainerHelpers::MoveArray(newNodes, *nodes, size);
            free(*buffer);
        }

        *buffer = newBuffer;
        *values = newValues;
        *nodes = newNodes;
    }

    template<typename T>
    void UnorderedRemoveAt(T* v, uint32_t i, int32_t size)
    {
        auto n = (int32_t)size - 1;

        if ((int32_t)i != n)
        {
            v[i] = std::move(v[n]);
        }
    }

    // Unsafe for vectors with elements that have dealloc in destructors
    template<typename T>
    void OrderedRemoveAt(T* arr, uint32_t i, int32_t size)
    {
        auto n = (int32_t)size - 1;

        if ((int32_t)i != n)
        {
            memmove(arr + i, arr + i + 1, sizeof(T) * (n - i));
        }
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
    void QuickSort(T* v, size_t count)
    {
        if (count > 1)
        {
            Comparer<T> c{};
            QuickSort(v, c, 0, (int32_t)count - 1);
        }
    }

    template<typename T>
    static int32_t LowerBound(T* arr, int32_t size, uint32_t value)
    {
        Bound<T> b{};
        int32_t n = size;
        int32_t l = 0;
        int32_t r = n - 1;

        while (l < r)
        {
            auto m = (l + r) / 2;
            auto c = b(arr[m]);

            if (value <= c)
            {
                r = m;
            }
            else
            {
                l = m + 1;
            }
        }

        if (l < n && b(arr[l]) < value)
        {
            ++l;
        }

        return l < 0 || l >= n ? -1 : l;
    }
}
