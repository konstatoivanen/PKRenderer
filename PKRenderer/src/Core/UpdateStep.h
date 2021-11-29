#pragma once

namespace PK::Core
{
    enum class UpdateStep
    {
        OpenFrame,
        UpdateInput,
        UpdateEngines,
        Render,
        CloseFrame,
    };
}