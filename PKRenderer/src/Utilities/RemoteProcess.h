#pragma once
#include <string>

namespace PK::Utilities
{
    bool ExecuteRemoteProcess(const char* executable, const char* arguments, std::string& outError);
    bool ExecuteRemoteProcess(const std::string& executable, const std::string& arguments, std::string& outError);
}