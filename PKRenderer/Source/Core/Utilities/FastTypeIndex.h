#pragma once
#include <cstdint>

namespace PK
{
    // Note do not use inside a dynamic library. 
    inline uint32_t pk_type_index_counter = 0u;
    
    template<typename T> 
    inline const uint32_t pk_type_index = pk_type_index_counter++;

    template<typename T>
    constexpr uint32_t pk_base_type_index()
    {
        using TBase = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
        return pk_type_index<TBase>;
    }
}
