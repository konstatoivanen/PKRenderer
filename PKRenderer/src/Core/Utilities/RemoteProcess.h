#pragma once
#include <string>

namespace PK::RemoteProcess
{
    bool Execute(const char* executable, const char* arguments, std::string& outError);
    bool Execute(const std::string& executable, const std::string& arguments, std::string& outError);
}