#include "PrecompiledHeader.h"
#include "Disposer.h"

namespace PK
{
    Disposer::Disposer(size_t initialCapacity)
    {
        m_disposables.Reserve(initialCapacity, false);
    }

    Disposer::~Disposer()
    {
        // Note important to go in forward order to preserve call order.
        for (auto i = 0u; i < m_disposables.GetCount(); ++i)
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
        // Do this in two loops as call order for elements in using the same fence might be important.
        for (auto i = 0u; i < m_disposables.GetCount(); ++i)
        {
            if (m_disposables[i].fence.IsComplete())
            {
                m_disposables[i].destructor(m_disposables[i].context, m_disposables[i].disposable);
                m_disposables[i].disposable = nullptr;
                m_disposables[i].context = nullptr;
            }
        }

        for (auto i = (int32_t)m_disposables.GetCount() - 1; i >= 0; --i)
        {
            if (m_disposables[i].disposable == nullptr)
            {
                m_disposables.UnorderedRemoveAt(i);
            }
        }
    }
}
