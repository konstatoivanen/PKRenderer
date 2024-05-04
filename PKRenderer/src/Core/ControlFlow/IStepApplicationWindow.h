#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/ApplicationStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct Window)

namespace PK::Core::ControlFlow
{
    class IStepApplicationOpenFrameWindow : public IStep<ApplicationStep::OpenFrame, PK::Rendering::RHI::Window*>
    {
    protected:
        virtual ~IStepApplicationOpenFrameWindow() = default;
    public:
        virtual void OnApplicationOpenFrame(PK::Rendering::RHI::Window* window) = 0;
        void Step(ApplicationStep::OpenFrame token, PK::Rendering::RHI::Window* window) final { OnApplicationOpenFrame(window); }
    };

    class IStepApplicationUpdateInputWindow : public IStep<ApplicationStep::UpdateInput, PK::Rendering::RHI::Window*>
    {
    protected:
        virtual ~IStepApplicationUpdateInputWindow() = default;
    public:
        virtual void OnApplicationUpdateInput(PK::Rendering::RHI::Window* window) = 0;
        void Step(ApplicationStep::UpdateInput token, PK::Rendering::RHI::Window* window) final { OnApplicationUpdateInput(window); }
    };

    class IStepApplicationUpdateEnginesWindow : public IStep<ApplicationStep::UpdateEngines, PK::Rendering::RHI::Window*>
    {
    protected:
        virtual ~IStepApplicationUpdateEnginesWindow() = default;
    public:
        virtual void OnApplicationUpdateEngines(PK::Rendering::RHI::Window* window) = 0;
        void Step(ApplicationStep::UpdateEngines token, PK::Rendering::RHI::Window* window) final { OnApplicationUpdateEngines(window); }
    };

    class IStepApplicationRenderWindow : public IStep<ApplicationStep::Render, PK::Rendering::RHI::Window*>
    {
    protected:
        virtual ~IStepApplicationRenderWindow() = default;
    public:
        virtual void OnApplicationRender(PK::Rendering::RHI::Window* window) = 0;
        void Step(ApplicationStep::Render token, PK::Rendering::RHI::Window* window) final { OnApplicationRender(window); }
    };

    class IStepApplicationCloseFrameWindow : public IStep<ApplicationStep::CloseFrame, PK::Rendering::RHI::Window*>
    {
    protected:
        virtual ~IStepApplicationCloseFrameWindow() = default;
    public:
        virtual void OnApplicationCloseFrame(PK::Rendering::RHI::Window* window) = 0;
        void Step(ApplicationStep::CloseFrame token, PK::Rendering::RHI::Window* window) final { OnApplicationCloseFrame(window); }
    };
}