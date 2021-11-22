#pragma once
#include "PrecompiledHeader.h"
#include "Core/IService.h"
#include "Core/ISingleton.h"
#include <conio.h>

namespace PK::Utilities::Debug
{
    using namespace PK::Core;

    constexpr unsigned short ComposeConsoleColor(unsigned short fore, unsigned short back)
    {
        return ((unsigned)back << 4) | (unsigned)fore;
    }

    enum class ConsoleColor
    {
        BLACK = 0,
        DARK_BLUE = 1,
        DARK_GREEN = 2,
        DARK_CYAN = 3,
        DARK_RED = 4,
        DARK_MAGENTA = 5,
        DARK_YELLOW = 6,
        DARK_WHITE = 7,
        GRAY = 8,
        BLUE = 9,
        GREEN = 10,
        CYAN = 11,
        RED = 12,
        MAGENTA = 13,
        YELLOW = 14,
        WHITE = 15,
        LOG_PARAMETER = ComposeConsoleColor(15, 0),
        LOG_VERBOSE = ComposeConsoleColor(8, 0),
        LOG_HEADER = ComposeConsoleColor(0, 15),
        LOG_ERROR = ComposeConsoleColor(0, 4),
        LOG_WARNING = ComposeConsoleColor(0, 14),
        LOG_INPUT = ComposeConsoleColor(11, 0)
    };

    typedef enum 
    {
        PK_LOG_VERBOSE = (1 << 0),
        PK_LOG_INFO = (1 << 1),
        PK_LOG_WARNING = (1 << 2),
        PK_LOG_ERROR = (1 << 3),
        PK_LOG_ALL_FLAGS = 0xFF,
    } PKLogSeverityFlags;

    class Logger : public IService, public ISingleton<Logger>
    {
        public:
            Logger(uint32_t filterFlags) : m_filterFlags(filterFlags) {}

            void ClearLineRemainder(int length);
            void InsertNewLine();
            inline void SetConsoleColor(int color) 
            {
                #if defined(WIN32)
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); 
                #endif
            }

            template<typename T, typename... Args>
            void Log(PKLogSeverityFlags flags, int color, const T* message, const Args&...args)
            {
                if ((flags & m_filterFlags) != 0)
                {
                    SetConsoleColor(color);
                    ClearLineRemainder(printf(message, args...));
                    InsertNewLine();
                }
            }

            template<typename T, typename... Args>
            void LogOverwrite(int color, const T* message, const Args&...args)
            {
                SetConsoleColor(color);
                ClearLineRemainder(printf(message, args...));
                printf("\r");
            }

            template<typename T, typename ... Args>
            std::exception Exception(PKLogSeverityFlags flags, int color, const T* message, const Args&...args)
            {
                if ((flags & m_filterFlags) != 0)
                {
                    SetConsoleColor(color);
                    ClearLineRemainder(printf(message, args...));
                    InsertNewLine();
                }

                _getch();
                return std::runtime_error(message);
            }

        private:
            uint32_t m_filterFlags = PK_LOG_ALL_FLAGS;
            int m_lineClearLength = 0;
    };
}

#define PK_LOG_NEWLINE() PK::Utilities::Debug::Logger::Get()->InsertNewLine()
#define PK_LOG_HEADER(...) PK::Utilities::Debug::Logger::Get()->Log(PK::Utilities::Debug::PK_LOG_INFO, (unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_HEADER, __VA_ARGS__)
#define PK_LOG_INFO(...) PK::Utilities::Debug::Logger::Get()->Log(PK::Utilities::Debug::PK_LOG_INFO, (unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_PARAMETER, __VA_ARGS__)
#define PK_LOG_VERBOSE(...) PK::Utilities::Debug::Logger::Get()->Log(PK::Utilities::Debug::PK_LOG_VERBOSE, (unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_VERBOSE, __VA_ARGS__)
#define PK_LOG_WARNING(...) PK::Utilities::Debug::Logger::Get()->Log(PK::Utilities::Debug::PK_LOG_WARNING, (unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_WARNING, __VA_ARGS__)
#define PK_LOG_ERROR(...) PK::Utilities::Debug::Logger::Get()->Log(PK::Utilities::Debug::PK_LOG_ERROR, (unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_WARNING, __VA_ARGS__)
#define PK_LOG_OVERWRITE(...) PK::Utilities::Debug::Logger::Get()->LogOverwrite((unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_PARAMETER, __VA_ARGS__)
#define PK_GET_EXCEPTION(...) PK::Utilities::Debug::Logger::Get()->Exception(PK::Utilities::Debug::PK_LOG_ERROR, (unsigned short)PK::Utilities::Debug::ConsoleColor::LOG_ERROR, __VA_ARGS__)
#define PK_THROW_ERROR(...) throw PK_GET_EXCEPTION(__VA_ARGS__)
#define PK_THROW_ASSERT(value, ...) { if(!(value)) { PK_THROW_ERROR(__VA_ARGS__); } }
#define PK_THROW_ASSERT_WARNING(value, ...) { if(!(value)) { PK_LOG_WARNING(__VA_ARGS__); } }
