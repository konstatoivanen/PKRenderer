#pragma once
namespace PK
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

        constexpr ConstBufferView() = default;
        constexpr ConstBufferView(const T* data, size_t count) : data(data), count(count) {}

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


    template<typename T>
    struct InterleavedBufferView
    {
        const uint8_t* data = nullptr;
        size_t count = 0;
        size_t stride = 0;
        size_t offset = 0;

        const T& operator[](size_t);
    };

    template<typename T>
    const T& InterleavedBufferView<T>::operator[](size_t index)
    {
        if (index >= count)
        {
            throw std::invalid_argument("Out of bounds index");
        }

        return *reinterpret_cast<T*>(data + index * stride + offset);  
    }
}
