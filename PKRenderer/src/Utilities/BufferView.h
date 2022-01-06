#pragma once

namespace PK::Utilities
{
    template<typename T>
    struct BufferView
    {
        T* data = nullptr;
        size_t count = 0;
    
        T& operator[](size_t);
    };
    
    template<typename T>
    T& BufferView<T>::operator[](size_t index)
    {
        if (index >= count)
        {
            throw std::invalid_argument("Out of bounds index");
        }
    
        return data[index];
    }

    template<typename T>
    struct ConstBufferView
    {
        const T* data = nullptr;
        size_t count = 0;

        const T& operator[](size_t);
    };

    template<typename T>
    const T& ConstBufferView<T>::operator[](size_t index)
    {
        if (index >= count)
        {
            throw std::invalid_argument("Out of bounds index");
        }

        return data[index];
    }
}