#include "PrecompiledHeader.h"
#include "InputKeyCommands.h"

namespace PK
{
    void InputKeyCommands::Reserve(size_t commandCapacity, size_t stringCapacity)
    {
        if (commandCapacity > m_commandCapacity || stringCapacity > m_stringCapacity)
        {
            commandCapacity = commandCapacity < m_commandCapacity ? m_commandCapacity : commandCapacity;
            stringCapacity = stringCapacity < m_stringCapacity ? m_stringCapacity : stringCapacity;
            const auto newSize = commandCapacity * sizeof(InputKeyCommand) + stringCapacity;
            auto newData = TData::Allocate(newSize);
            auto newPtr = TData::GetPtr(newData);
            auto newString = newPtr + commandCapacity * sizeof(InputKeyCommand);
            auto newCommands = reinterpret_cast<InputKeyCommand*>(newPtr);
            auto oldCommands = GetCommands();
            m_commandCapacity = commandCapacity;
            m_stringCapacity = stringCapacity;

            for (auto i = 0u; i < m_count; ++i)
            {
                newCommands[i].key = oldCommands[i].key;
                newCommands[i].command = newString;
                while ((*newString++ = *oldCommands[i].command++) != '\0') {}
            }

            TData::Free(m_data);
            m_data = newData;
        }
    }

    void InputKeyCommands::Add(const InputTriplet& key, const char* command, size_t commandLength)
    {
        if (command && command[0])
        {
            const auto length = commandLength ? commandLength : strlen(command);
            Reserve(m_count + 1ull, m_head + length);

            auto& destination = GetCommands()[m_count++];
            destination.command = GetString() + m_head;
            destination.key = key;
            
            strncpy(destination.command, command, length);
            destination.command[length] = '\0';
            m_head += length + 1ull;
        }
    }

    void InputKeyCommands::Clear() { m_count = 0ull; m_head = 0ull; }
}
