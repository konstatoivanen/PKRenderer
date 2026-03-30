#pragma once
#include "NoCopy.h"
#include "Memory.h"

namespace PK
{
    template<typename T>
    struct Singleton : public NoCopy
    {
        Singleton() 
        {
            Memory::Assert(s_Instance == nullptr, "Singleton instance already exists!");
            s_Instance = static_cast<T*>(this); 
        }

        virtual ~Singleton() = default;
        inline static T* Get() { return s_Instance; }
    
        private: 
            inline static T* s_Instance = nullptr;
    };
}
