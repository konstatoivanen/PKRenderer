#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

    class VulkanDisposer : public NoCopy
    {
        struct DisposeHandle
        {
            Scope<IVulkanDisposable> disposable = nullptr;
            ExecutionGate gate{};
        };

        public:
            VulkanDisposer() {}

            template<typename T>
            void Dispose(T* disposable, const ExecutionGate& releaseGate)
            {
                static_assert(std::is_base_of<IVulkanDisposable, T>::value, "Template argument type does not derive from IService!");
                m_disposables.push_back({ Scope<IVulkanDisposable>(disposable), releaseGate });
            }

            void Prune();

        private:
            std::vector<DisposeHandle> m_disposables;
    };
}