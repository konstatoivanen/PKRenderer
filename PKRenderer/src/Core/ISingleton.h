#pragma once
#include "Core/NoCopy.h"

namespace PK::Core
{
    template<typename T>
    class ISingleton : public NoCopy
    {
        public: 
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
    
        private: inline static T* s_Instance = nullptr;
    };
}