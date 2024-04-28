#pragma once
namespace PK::Core::ControlFlow
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