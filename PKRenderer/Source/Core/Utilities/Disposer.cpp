#include "PrecompiledHeader.h"
#include "Disposer.h"

namespace PK
{
    Disposer::Disposer(size_t initialCapacity)
    {
        m_disposables.Reserve(initialCapacity);
    }

    Disposer::~Disposer()
    {
        for (auto i = (int)m_disposables.GetCount() - 1; i >= 0; --i)
        {
            m_disposables[i].destructor(m_disposables[i].context, m_disposables[i].disposable);
        }
    }

    void Disposer::Dispose(void* context, void* disposable, Destructor destructor, const FenceRef& releaseFence)
    {
        if (disposable != nullptr)
        {
            auto handle = m_disposables.Add();
            handle->context = context;
            handle->disposable = disposable;
            handle->destructor = destructor;
            handle->fence = releaseFence;
        }
    }

    void Disposer::Prune()
    {
        for (auto i = (int32_t)m_disposables.GetCount() - 1; i >= 0; --i)
        {
            if (m_disposables[i].fence.IsComplete())
            {
                m_disposables[i].destructor(m_disposables[i].context, m_disposables[i].disposable);
                m_disposables.UnorderedRemoveAt(i);
            }
        }
    }
}
