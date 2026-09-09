#include "PrecompiledHeader.h"
#include "GUIHashStack.h"

namespace PK
{
    GUIHashStack::GUIHashStack()
    {
        m_hashStack[0] = 14695981039346656037ull; 
    }

    void GUIHashStack::Push(const char* name)
    {
        if (m_hashHead < HASH_STACK_MAX)
        {
            auto value = m_hashStack[m_hashHead];

            for (auto i = 0u; name && name[i]; ++i)
            {
                value ^= static_cast<uint64_t>(name[i]);
                value *= 1099511628211ull;
            }

            m_hashStack[++m_hashHead] = value;
        }
    }

    void GUIHashStack::Pop()
    {
        if (m_hashHead)
        {
            --m_hashHead;
        }
    }

    uint64_t GUIHashStack::Get() const
    {
        return m_hashStack[m_hashHead];
    }
}
