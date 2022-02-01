#include "PrecompiledHeader.h"
#include "Log.h"

namespace PK::Core::Services::Debug
{
    void Logger::ClearLineRemainder(int32_t length)
    {
        auto l = m_lineClearLength - length;
    
        if (m_lineClearLength < length)
        {
            m_lineClearLength = length;
        }
    
        if (l <= 0)
        {
            return;
        }
    
        printf(std::string(l, ' ').c_str());
    }
    
    void Logger::InsertNewLine()
    {
        SetConsoleColor((int)ConsoleColor::LOG_PARAMETER);
        printf("\n");
        m_lineClearLength = 0;
    }
}