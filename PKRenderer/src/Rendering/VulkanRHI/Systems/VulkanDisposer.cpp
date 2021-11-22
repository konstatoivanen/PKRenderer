#include "PrecompiledHeader.h"
#include "VulkanDisposer.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    void VulkanDisposer::Dispose(Ref<VulkanDisposable> disposable, const VulkanExecutionGate& releaseGate)
    {
        m_disposables.push_back({ disposable, releaseGate });
    }
    
    void VulkanDisposer::Prune()
    {
        decltype(m_disposables) disposables;
        disposables.swap(m_disposables);

        for (auto& disposable : disposables)
        {
            if (!disposable.gate.IsCompleted())
            {
                m_disposables.push_back(disposable);
            }
        }
    }
}