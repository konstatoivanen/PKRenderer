#pragma once
#include <cstdint>

namespace PK::Rendering::RHI
{
    struct FenceRef
    {
        constexpr static const uint64_t INVALID_USER_DATA = 0xFFFFFFFFFFFFFFFF;
        typedef bool (*WaitFunction)(const void*, uint64_t, uint64_t);

        FenceRef() {};
        FenceRef(const void* context, WaitFunction waitFunction, uint64_t userdata) :
            m_context(context),
            m_waitFunction(waitFunction), 
            m_userdata(userdata) 
        {
        };

        inline bool Invalidate() { m_waitFunction = nullptr; m_userdata = INVALID_USER_DATA; return true; }
        inline bool IsValid() { return m_userdata != INVALID_USER_DATA && m_waitFunction != nullptr; }
        inline bool Wait(uint64_t timeout) { return !IsValid() || m_waitFunction(m_context, m_userdata, timeout); }
        inline bool WaitInvalidate(uint64_t timeout) { return Wait(timeout) && Invalidate(); }
        inline bool IsComplete() { return Wait(0ull); }
        
        private:
            const void* m_context = nullptr;
            WaitFunction m_waitFunction = nullptr;
            uint64_t m_userdata = INVALID_USER_DATA;
    };
}