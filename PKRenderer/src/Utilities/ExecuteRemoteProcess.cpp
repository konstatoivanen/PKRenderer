#include "PrecompiledHeader.h"
#include "ExecuteRemoteProcess.h"
#include <codecvt>
#include <locale>

namespace PK::Utilities
{
    bool ExecuteRemoteProcess(const char* executable, const char* arguments, std::string& outError)
    {
        const auto executableLen = strlen(executable);
        const auto argumentsLen = strlen(arguments);

        if (executableLen == 0)
        {
            outError = "Execute remote processs executable path was empty";
            return false;
        }

        if (argumentsLen == 0)
        {
            outError = "Execute remote processs arguments were empty";
            return false;
        }

#if defined(WIN32)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
    
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
    
        const auto wideExecutableLen = MultiByteToWideChar(CP_UTF8, 0, executable, (int)executableLen, nullptr, 0);
        const auto wideArgumentsLen = MultiByteToWideChar(CP_UTF8, 0, arguments, (int)argumentsLen, nullptr, 0);
        std::wstring wideExecutable(wideExecutableLen, 0);
        std::wstring wideArguments(wideArgumentsLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, executable, (int)executableLen, wideExecutable.data(), wideExecutableLen);
        MultiByteToWideChar(CP_UTF8, 0, arguments, (int)argumentsLen, wideArguments.data(), wideArgumentsLen);

        // Remove quotes from path
        if (wideExecutable[0] == L'\'')
        {
            wideExecutable = wideExecutable.substr(1);
        }

        if (wideExecutable.back() == L'\'')
        {
            wideExecutable = wideExecutable.substr(0, wideExecutable.size() - 1);
        }

        auto result = CreateProcess(wideExecutable.data(), wideArguments.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        
        if (result == 0)
        {
            outError = "Execute remote processs failed with error code:" + std::to_string(GetLastError());
            return false;
        }
    
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
#endif

        return true;
    }
    
    bool ExecuteRemoteProcess(const std::string& executablePath, const std::string& arguments, std::string& outError)
    {
        return ExecuteRemoteProcess(executablePath.c_str(), arguments.c_str(), outError);
    }
}
