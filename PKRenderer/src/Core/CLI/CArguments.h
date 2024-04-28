#pragma once
namespace PK::Core::CLI
{
    struct CArguments
    {
        int count;
        char** args;
    };

    struct CArgument
    {
        char* arg;
    };

    struct CArgumentsConst
    {
        const char** args;
        int count;
    };

    struct CArgumentConst
    {
        const char* arg;
    };
}