#include "PrecompiledHeader.h"
#include "EnginePKAssetBuilder.h"
#include "Core/Services/Log.h"

namespace PK::ECS::Engines
{
    EnginePKAssetBuilder::EnginePKAssetBuilder(const ApplicationArguments& arguments) : m_executablePath(L"")
    {
        if (arguments.count < 4)
        {
            return;
        }

        auto executableDir = std::filesystem::path(arguments.args[1]);
        auto sourcedirectory = std::filesystem::path(arguments.args[2]).wstring();
        auto targetdirectory = std::filesystem::path(arguments.args[3]).wstring();
        m_executablePath = executableDir.wstring();
        m_executablePath = m_executablePath.substr(1, m_executablePath.size() - 2);
        auto args = L"\"" + sourcedirectory + L"\" \"" + targetdirectory + L"";

        m_executableArguments.resize(args.size() + 1);
        memcpy(m_executableArguments.data(), args.c_str(), sizeof(wchar_t) * args.size());
    }
    
    void EnginePKAssetBuilder::Step(ConsoleCommandToken* token)
    {
        if (m_executablePath.empty() || m_executableArguments.empty() || token->isConsumed || token->argument != "recompile_pkassets")
        {
            return;
        }
        
        token->isConsumed = true;

        #if defined(WIN32)
            PK_LOG_INFO("Executing Command!");

            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            PK_THROW_ASSERT(CreateProcess(m_executablePath.c_str(), m_executableArguments.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi), "CreateProcess failed (%d).", GetLastError());
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
        #endif
    }
}