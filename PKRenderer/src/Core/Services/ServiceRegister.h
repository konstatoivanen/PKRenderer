#pragma once
#include "IService.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Core/Services/Log.h"

namespace PK::Core::Services
{
    class ServiceRegister : public PK::Utilities::NoCopy
    {
        public:
            template <typename T>
            T* Get() { return static_cast<T*>(m_services.at(std::type_index(typeid(T))).get()); }
    
            template<typename T, typename ... Args>
            T* Create(Args&& ... args)
            {
                static_assert(std::is_base_of<IService, T>::value, "Template argument type does not derive from IService!");

                printf("Creating Service: %s \n", typeid(T).name());
                PK_LOG_SCOPE_INDENT(service);

                auto idx = std::type_index(typeid(T));
                PK_THROW_ASSERT(m_services.count(idx) == 0, "Service of type (%s) is already registered", typeid(T).name());
                auto service = new T(std::forward<Args>(args)...);
                m_services[idx] = PK::Utilities::Scope<IService>(service);
                return service;
            }
    
            inline void Clear() { m_services.clear(); }
    
        private:
            std::unordered_map<std::type_index, PK::Utilities::Scope<IService>> m_services;
    };
}