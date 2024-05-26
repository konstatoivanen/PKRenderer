#pragma once
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/ApplicationStep.h"

namespace PK
{
    class IStepApplicationOpenFrame : public IStep<ApplicationStep::OpenFrame>
    {
    protected:
        virtual ~IStepApplicationOpenFrame() = default;
    public:
        virtual void OnApplicationOpenFrame() = 0;
        virtual void Step(ApplicationStep::OpenFrame token) final { OnApplicationOpenFrame(); }
    };

    class IStepApplicationUpdateInput : public IStep<ApplicationStep::UpdateInput>
    {
    protected:
        virtual ~IStepApplicationUpdateInput() = default;
    public:
        virtual void OnApplicationUpdateInput() = 0;
        virtual void Step(ApplicationStep::UpdateInput token) final { OnApplicationUpdateInput(); }
    };

    class IStepApplicationUpdateEngines : public IStep<ApplicationStep::UpdateEngines>
    {
    protected:
        virtual ~IStepApplicationUpdateEngines() = default;
    public:
        virtual void OnApplicationUpdateEngines() = 0;
        virtual void Step(ApplicationStep::UpdateEngines token) final { OnApplicationUpdateEngines(); }
    };

    class IStepApplicationRender : public IStep<ApplicationStep::Render>
    {
    protected:
        virtual ~IStepApplicationRender() = default;
    public:
        virtual void OnApplicationRender() = 0;
        virtual void Step(ApplicationStep::Render token) final { OnApplicationRender(); }
    };

    class IStepApplicationCloseFrame : public IStep<ApplicationStep::CloseFrame>
    {
    protected:
        virtual ~IStepApplicationCloseFrame() = default;
    public:
        virtual void OnApplicationCloseFrame() = 0;
        virtual void Step(ApplicationStep::CloseFrame token) final { OnApplicationCloseFrame(); }
    };
}