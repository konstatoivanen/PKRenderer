#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    class VulkanDisposer : public PK::Utilities::NoCopy
    {
        struct DisposeHandle
        {
            PK::Utilities::Scope<IVulkanDisposable> disposable = nullptr;
            Structs::ExecutionGate gate{};
        };

        public:
            VulkanDisposer() {}

            template<typename T>
            void Dispose(T* disposable, const Structs::ExecutionGate& releaseGate)
            {
                static_assert(std::is_base_of<IVulkanDisposable, T>::value, "Template argument type does not derive from IService!");
                m_disposables.push_back({ PK::Utilities::Scope<IVulkanDisposable>(disposable), releaseGate });
            }

            void Prune();

        private:
            std::vector<DisposeHandle> m_disposables;
    };
}