#pragma once
#include "NoCopy.h"

namespace PK
{
    template<typename T>
    struct Singleton : public NoCopy
    {
        Singleton() 
        {
            if (s_Instance != nullptr)
            {
                throw std::exception("Singleton instance already exists!");
            }

            s_Instance = static_cast<T*>(this); 
        }

        virtual ~Singleton() = default;
        inline static T* Get() { return s_Instance; }
    
        private: 
            inline static T* s_Instance = nullptr;
    };
}
