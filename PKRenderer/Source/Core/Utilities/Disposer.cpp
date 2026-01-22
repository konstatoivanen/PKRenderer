#include "PrecompiledHeader.h"
#include "Disposer.h"

namespace PK
{
    Disposer::~Disposer()
    {
        for (auto i = (int)m_disposables.size() - 1; i >= 0; --i)
        {
            m_disposables[i].destructor(m_disposables[i].context, m_disposables[i].disposable);
        }
    }

    void Disposer::Dispose(void* context, void* disposable, Destructor destructor, const FenceRef& releaseFence)
    {
        if (disposable != nullptr)
        {
            m_disposables.push_back({ context, disposable, destructor, releaseFence });
        }
    }

    void Disposer::Prune()
    {
        for (auto i = (int32_t)m_disposables.size() - 1; i >= 0; --i)
        {
            if (m_disposables.at(i).fence.IsComplete())
            {
                auto n = (int32_t)m_disposables.size() - 1;
                m_disposables[i].destructor(m_disposables[i].context, m_disposables[i].disposable);

                if (i != n)
                {
                    m_disposables[i] = std::move(m_disposables[n]);
                }

                m_disposables.pop_back();
            }
        }
    }
}
