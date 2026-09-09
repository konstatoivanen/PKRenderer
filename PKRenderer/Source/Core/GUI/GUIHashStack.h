#pragma once
#include <stdint.h>

namespace PK
{
    struct GUIHashStack
    {
        constexpr const static uint32_t HASH_STACK_MAX = 15u;

        GUIHashStack();

        void Push(const char* name);
        void Pop();
        uint64_t Get() const;

    private:
        uint64_t m_hashStack[HASH_STACK_MAX + 1u]{};
        uint32_t m_hashHead = 0u;
    };
}
