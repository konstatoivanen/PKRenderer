#pragma once

namespace PK::ECS::Tokens
{
    struct TimeToken
    {
        double timeScale = 0.0;
        double time = 0.0;
        double unscaledTime = 0.0;
        double deltaTime = 0.0;
        double unscaledDeltaTime = 0.0;
        double smoothDeltaTime = 0.0;
        double unscaledDeltaTimeFixed = 0.0;
    
        bool logFrameRate = false;
    };
}