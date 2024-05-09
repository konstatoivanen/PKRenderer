#pragma once
#include "IService.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Core/CLI/LogScopeIndent.h"

namespace PK::Core
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
            auto idx = std::type_index(typeid(T));
            AssertTypeExists(idx);
            auto logIndent = PK::Core::CLI::LogScopeIndent();
            auto service = new T(std::forward<Args>(args)...);
            m_services[idx] = PK::Utilities::Scope<IService>(service);
            m_releaseOrder.push_back(idx);
            return service;
        }

        inline void Clear()
        {
            // Release services in reverse insertion order to ensure correct order of memory release
            for (auto i = (int32_t)m_releaseOrder.size() - 1; i >= 0; --i)
            {
                m_services[m_releaseOrder.at(i)] = nullptr;
            }

            m_releaseOrder.clear();
            m_services.clear();
        }

    private:
        void AssertTypeExists(std::type_index index);

        std::vector<std::type_index> m_releaseOrder;
        std::unordered_map<std::type_index, PK::Utilities::Scope<IService>> m_services;
    };
}