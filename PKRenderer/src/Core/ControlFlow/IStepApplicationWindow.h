#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/ApplicationStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Graphics::RHI, struct RHIWindow)

namespace PK::Graphics
{
    typedef RHI::RHIWindow Window;
}

namespace PK::Core::ControlFlow
{
    class IStepApplicationOpenFrameWindow : public IStep<ApplicationStep::OpenFrame, PK::Graphics::Window*>
    {
    protected:
        virtual ~IStepApplicationOpenFrameWindow() = default;
    public:
        virtual void OnApplicationOpenFrame(PK::Graphics::Window* window) = 0;
        void Step(ApplicationStep::OpenFrame token, PK::Graphics::Window* window) final { OnApplicationOpenFrame(window); }
    };

    class IStepApplicationUpdateInputWindow : public IStep<ApplicationStep::UpdateInput, PK::Graphics::Window*>
    {
    protected:
        virtual ~IStepApplicationUpdateInputWindow() = default;
    public:
        virtual void OnApplicationUpdateInput(PK::Graphics::Window* window) = 0;
        void Step(ApplicationStep::UpdateInput token, PK::Graphics::Window* window) final { OnApplicationUpdateInput(window); }
    };

    class IStepApplicationUpdateEnginesWindow : public IStep<ApplicationStep::UpdateEngines, PK::Graphics::Window*>
    {
    protected:
        virtual ~IStepApplicationUpdateEnginesWindow() = default;
    public:
        virtual void OnApplicationUpdateEngines(PK::Graphics::Window* window) = 0;
        void Step(ApplicationStep::UpdateEngines token, PK::Graphics::Window* window) final { OnApplicationUpdateEngines(window); }
    };

    class IStepApplicationRenderWindow : public IStep<ApplicationStep::Render, PK::Graphics::Window*>
    {
    protected:
        virtual ~IStepApplicationRenderWindow() = default;
    public:
        virtual void OnApplicationRender(PK::Graphics::Window* window) = 0;
        void Step(ApplicationStep::Render token, PK::Graphics::Window* window) final { OnApplicationRender(window); }
    };

    class IStepApplicationCloseFrameWindow : public IStep<ApplicationStep::CloseFrame, PK::Graphics::Window*>
    {
    protected:
        virtual ~IStepApplicationCloseFrameWindow() = default;
    public:
        virtual void OnApplicationCloseFrame(PK::Graphics::Window* window) = 0;
        void Step(ApplicationStep::CloseFrame token, PK::Graphics::Window* window) final { OnApplicationCloseFrame(window); }
    };
}