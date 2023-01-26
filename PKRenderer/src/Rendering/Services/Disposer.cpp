#include "PrecompiledHeader.h"
#include "Disposer.h"

namespace PK::Rendering::Services
{
    void Disposer::Prune()
    {
        for (auto i = (int)m_disposables.size() - 1; i >= 0; --i)
        {
            if (m_disposables.at(i).gate.IsComplete())
            {
                auto n = m_disposables.size() - 1;

                if (i != n)
                {
                    m_disposables[i] = std::move(m_disposables[n]);
                }

                m_disposables.pop_back();
            }
        }
    }
}