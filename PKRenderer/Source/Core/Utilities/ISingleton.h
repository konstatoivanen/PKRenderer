#pragma once
#include "NoCopy.h"

namespace PK
{
    template<typename T>
    struct ISingleton : public NoCopy
    {
        ISingleton() 
        {
            if (s_Instance != nullptr)
            {
                throw std::exception("Singleton instance already exists!");
            }

            s_Instance = static_cast<T*>(this); 
        }

        virtual ~ISingleton() = default;
        inline static T* Get() { return s_Instance; }
    
        private: 
            inline static T* s_Instance = nullptr;
    };
}
