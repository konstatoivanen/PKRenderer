#pragma once

namespace PK::Core
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
}