#pragma once
#include <vector>
#include <typeindex>
#include <unordered_map>
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/CLI/LogScopeIndent.h"

namespace PK
{
    class ServiceRegister : public NoCopy
    {
        struct Service : public NoCopy
        {
            virtual ~Service() = 0;
        };

        template<typename T>
        struct ServiceContainer : Service
        {
            T Instance;

            template<typename ... Args>
            ServiceContainer(Args&& ... args) : Instance(std::forward<Args>(args)...) {}
        };
        

    public:
        template <typename T>
        T* Get() { return &static_cast<ServiceContainer<T>*>(m_services.at(std::type_index(typeid(T))).get())->Instance; }

        template<typename T, typename ... Args>
        T* Create(Args&& ... args)
        {
            auto idx = std::type_index(typeid(T));
            AssertTypeExists(idx);
            auto logIndent = PK::LogScopeIndent();
            auto service = new ServiceContainer<T>(std::forward<Args>(args)...);
            m_services[idx] = Scope<Service>(service);
            m_releaseOrder.push_back(idx);
            return &service->Instance;
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
        std::unordered_map<std::type_index, Scope<Service>> m_services;
    };
}