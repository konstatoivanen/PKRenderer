#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

    class VulkanDisposer : public PK::Core::NoCopy
    {
        public:
            template<typename T>
            void Dispose(T* disposable, const VulkanExecutionGate& releaseGate)
            {
                static_assert(std::is_base_of<IVulkanDisposable, T>::value, "Template argument type does not derive from IService!");
                m_disposables.push_back({ Scope<IVulkanDisposable>(disposable), releaseGate });
            }

            void Prune();

        private:
            struct DisposeHandle
            {
                Scope<IVulkanDisposable> disposable;
                VulkanExecutionGate gate;
            };

            std::vector<DisposeHandle> m_disposables;
    };
}