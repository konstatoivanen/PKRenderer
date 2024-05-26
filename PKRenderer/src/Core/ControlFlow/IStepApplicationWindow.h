#pragma once
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/ApplicationStep.h"

namespace PK
{
    struct RHIWindow;

    class IStepApplicationOpenFrameWindow : 
        public IStep<ApplicationStep::OpenFrame, RHIWindow*>
    {
    protected:
        virtual ~IStepApplicationOpenFrameWindow() = default;
    public:
        virtual void OnApplicationOpenFrame(RHIWindow* window) = 0;
        void Step(ApplicationStep::OpenFrame token, RHIWindow* window) final { OnApplicationOpenFrame(window); }
    };

    class IStepApplicationUpdateInputWindow : 
        public IStep<ApplicationStep::UpdateInput, RHIWindow*>
    {
    protected:
        virtual ~IStepApplicationUpdateInputWindow() = default;
    public:
        virtual void OnApplicationUpdateInput(RHIWindow* window) = 0;
        void Step(ApplicationStep::UpdateInput token, RHIWindow* window) final { OnApplicationUpdateInput(window); }
    };

    class IStepApplicationUpdateEnginesWindow : 
        public IStep<ApplicationStep::UpdateEngines, RHIWindow*>
    {
    protected:
        virtual ~IStepApplicationUpdateEnginesWindow() = default;
    public:
        virtual void OnApplicationUpdateEngines(RHIWindow* window) = 0;
        void Step(ApplicationStep::UpdateEngines token, RHIWindow* window) final { OnApplicationUpdateEngines(window); }
    };

    class IStepApplicationRenderWindow : 
        public IStep<ApplicationStep::Render, RHIWindow*>
    {
    protected:
        virtual ~IStepApplicationRenderWindow() = default;
    public:
        virtual void OnApplicationRender(RHIWindow* window) = 0;
        void Step(ApplicationStep::Render token, RHIWindow* window) final { OnApplicationRender(window); }
    };

    class IStepApplicationCloseFrameWindow : 
        public IStep<ApplicationStep::CloseFrame, RHIWindow*>
    {
    protected:
        virtual ~IStepApplicationCloseFrameWindow() = default;
    public:
        virtual void OnApplicationCloseFrame(RHIWindow* window) = 0;
        void Step(ApplicationStep::CloseFrame token, RHIWindow* window) final { OnApplicationCloseFrame(window); }
    };
}