#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

    class VulkanDisposer : public PK::Core::NoCopy
    {
        public:
            void Dispose(Ref<VulkanDisposable> disposable, const VulkanExecutionGate& releaseGate);
            void Prune();

        private:
            struct DisposeHandle
            {
                Ref<VulkanDisposable> disposable;
                VulkanExecutionGate gate;
            };

            std::vector<DisposeHandle> m_disposables;
    };
}