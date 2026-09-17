#pragma once
#include "Core/Base/Allocation.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputKeyCommand
    {
        char* command;
        InputTriplet key;
    };

    struct InputKeyCommands
    {
        using TData = AllocationHeap::Data<char>;

        constexpr InputKeyCommands() = default;
        ~InputKeyCommands() { TData::Free(m_data); }

        InputKeyCommand& operator [](size_t i) { return GetCommands()[i]; }
        InputKeyCommand const& operator [](size_t i) const { return GetCommands()[i]; }
        operator InputKeyCommand* () { return GetCommands(); }
        operator InputKeyCommand const* () const { return GetCommands(); }
        InputKeyCommand* GetCommands() { return reinterpret_cast<InputKeyCommand*>(TData::GetPtr(m_data)); }
        InputKeyCommand const* GetCommands() const { return reinterpret_cast<InputKeyCommand const*>(TData::GetPtr(m_data)); }
        char* GetString() { return TData::GetPtr(m_data) + sizeof(InputKeyCommand) * m_commandCapacity; }
        const char* GetString() const { return TData::GetPtr(m_data) + sizeof(InputKeyCommand) * m_commandCapacity; }
        constexpr size_t GetCount() const { return m_count; }

        InputKeyCommand* begin() { return GetCommands(); }
        InputKeyCommand* end() { return GetCommands() + GetCount(); }
        InputKeyCommand const* begin() const { return GetCommands(); }
        InputKeyCommand const* end() const { return GetCommands() + GetCount(); }

        void Reserve(size_t commandCapacity, size_t stringCapacity);
        void Add(const InputTriplet& key, const char* command, size_t commandLength = 0ull);
        void Clear();
    
    private:
        TData m_data{};
        size_t m_commandCapacity = 0ull;
        size_t m_stringCapacity = 0ull;
        size_t m_count = 0ull;
        size_t m_head = 0ull;
    };
}
