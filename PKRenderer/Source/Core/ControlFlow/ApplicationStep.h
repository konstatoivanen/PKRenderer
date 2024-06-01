#pragma once
namespace PK
{
    struct ApplicationStep
    {
        struct OpenFrame {};
        struct UpdateInput {};
        struct UpdateEngines {};
        struct Render {};
        struct CloseFrame {};
    };
}