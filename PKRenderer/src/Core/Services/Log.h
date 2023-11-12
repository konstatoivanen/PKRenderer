#pragma once
#include "Utilities/ISingleton.h"
#include "Core/Services/IService.h"
#include <conio.h>
#include <ctime>
#include <chrono>

namespace PK::Core::Services::Debug
{
    constexpr uint16_t ComposeConsoleColor(uint16_t fore, uint16_t back)
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
        LOG_RHI = ComposeConsoleColor(8, 0),
        LOG_INFO = ComposeConsoleColor(15, 0),
        LOG_VERBOSE = ComposeConsoleColor(8, 0),
        LOG_HEADER = ComposeConsoleColor(0, 15),
        LOG_ERROR = ComposeConsoleColor(0, 4),
        LOG_WARNING = ComposeConsoleColor(0, 14),
        LOG_INPUT = ComposeConsoleColor(11, 7)
    };

    typedef enum 
    {
        PK_LOG_LVL_RHI = 1 << 0,
        PK_LOG_LVL_VERBOSE = 1 << 1,
        PK_LOG_LVL_INFO = 1 << 2,
        PK_LOG_LVL_WARNING = 1 << 3,
        PK_LOG_LVL_ERROR = 1 << 4,
        PK_LOG_LVL_ALL_FLAGS = 0xFF,
    } PKLogSeverityFlags;

    class Logger : public IService, public PK::Utilities::ISingleton<Logger>
    {
        public:
            Logger(uint32_t filterFlags) : m_filterFlags(filterFlags) {}

            void ClearLineRemainder(int32_t length);
            void InsertIndentation();
            void InsertNewLine();
            inline void SetFilter(uint32_t flags) { m_filterFlags = flags; }
            inline void SetConsoleColor(int32_t color) 
            {
                #if defined(WIN32)
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); 
                #endif
            }

            template<typename T, typename... Args>
            void Log(PKLogSeverityFlags flags, int32_t color, const T* message, const Args&...args)
            {
                if ((flags & m_filterFlags) != 0)
                {
                    SetConsoleColor(color);
                    InsertIndentation();
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
            std::exception Exception(PKLogSeverityFlags flags, int32_t color, const T* message, const Args&...args)
            {
                if ((flags & m_filterFlags) != 0)
                {
                    SetConsoleColor(color);
                    InsertIndentation();
                    ClearLineRemainder(printf(message, args...));
                    InsertNewLine();
                }

                #if defined(WIN32) && defined(PK_DEBUG)
                DebugBreak();
                #endif

                _getch();
                return std::runtime_error(message);
            }

            static void AddIndent();
            static void SubIndent();

        private:
            uint32_t m_filterFlags = PK_LOG_LVL_ALL_FLAGS;
            int32_t m_lineClearLength = 0;
    };

    struct ScopeTimer : public Utilities::NoCopy
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> start;
        const char* name;
        ScopeTimer(const char* name);
        ~ScopeTimer();
    };


    struct IndentScope : public Utilities::NoCopy
    {
        IndentScope();
        ~IndentScope();
    };
}

#define PK_LOG_NEWLINE() PK::Core::Services::Debug::Logger::Get()->InsertNewLine()
#define PK_LOG_HEADER(...) PK::Core::Services::Debug::Logger::Get()->Log(PK::Core::Services::Debug::PK_LOG_LVL_INFO, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_HEADER, __VA_ARGS__)
#define PK_LOG_INFO(...) PK::Core::Services::Debug::Logger::Get()->Log(PK::Core::Services::Debug::PK_LOG_LVL_INFO, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_INFO, __VA_ARGS__)
#define PK_LOG_VERBOSE(...) PK::Core::Services::Debug::Logger::Get()->Log(PK::Core::Services::Debug::PK_LOG_LVL_VERBOSE, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_VERBOSE, __VA_ARGS__)
#define PK_LOG_RHI(...) PK::Core::Services::Debug::Logger::Get()->Log(PK::Core::Services::Debug::PK_LOG_LVL_RHI, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_RHI, __VA_ARGS__)
#define PK_LOG_WARNING(...) PK::Core::Services::Debug::Logger::Get()->Log(PK::Core::Services::Debug::PK_LOG_LVL_WARNING, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_WARNING, __VA_ARGS__)
#define PK_LOG_ERROR(...) PK::Core::Services::Debug::Debug::Get()->Log(PK::Core::Services::Debug::PK_LOG_LVL_ERROR, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_WARNING, __VA_ARGS__)
#define PK_LOG_OVERWRITE(...) PK::Core::Services::Debug::Logger::Get()->LogOverwrite((unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_INFO, __VA_ARGS__)
#define PK_GET_EXCEPTION(...) PK::Core::Services::Debug::Logger::Get()->Exception(PK::Core::Services::Debug::PK_LOG_LVL_ERROR, (unsigned short)PK::Core::Services::Debug::ConsoleColor::LOG_ERROR, __VA_ARGS__)
#define PK_LOG_ADD_INDENT() PK::Core::Services::Debug::Logger::AddIndent()
#define PK_LOG_SUB_INDENT() PK::Core::Services::Debug::Logger::SubIndent()
#define PK_THROW_ERROR(...) throw PK_GET_EXCEPTION(__VA_ARGS__)
#define PK_THROW_ASSERT(value, ...) { if(!(value)) { PK_THROW_ERROR(__VA_ARGS__); } }
#define PK_WARNING_ASSERT(value, ...) { if(!(value)) { PK_LOG_WARNING(__VA_ARGS__); } }
#define PK_LOG_SCOPE_TIMER(name) auto PK_LOG_TIMER_##name = PK::Core::Services::Debug::ScopeTimer(#name)
#define PK_LOG_SCOPE_INDENT(name) auto PK_LOG_SCOPE_INDENT_##name = PK::Core::Services::Debug::IndentScope()
