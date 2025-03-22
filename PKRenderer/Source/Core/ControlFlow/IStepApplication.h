#pragma once
#include "Core/ControlFlow/IStep.h"
#include "Core/ControlFlow/ApplicationStep.h"

namespace PK
{
    // Note: no perfect forwarding as we assume user knows which types to declare as forwarded, etc.
    template<typename... Args>
    class IStepApplicationOpenFrame : public IStep<ApplicationStep::OpenFrame, Args...>
    {
    protected:
        virtual ~IStepApplicationOpenFrame() = default;
    public:
        virtual void OnApplicationOpenFrame(Args ... args) = 0;
        virtual void Step([[maybe_unused]] ApplicationStep::OpenFrame token, Args ... args) final { OnApplicationOpenFrame(args...); }
    };

    template<typename... Args>
    class IStepApplicationUpdateInput : public IStep<ApplicationStep::UpdateInput, Args...>
    {
    protected:
        virtual ~IStepApplicationUpdateInput() = default;
    public:
        virtual void OnApplicationUpdateInput(Args ... args) = 0;
        virtual void Step([[maybe_unused]] ApplicationStep::UpdateInput token, Args ... args) final { OnApplicationUpdateInput(args...); }
    };

    template<typename... Args>
    class IStepApplicationUpdateEngines : public IStep<ApplicationStep::UpdateEngines, Args...>
    {
    protected:
        virtual ~IStepApplicationUpdateEngines() = default;
    public:
        virtual void OnApplicationUpdateEngines(Args ... args) = 0;
        virtual void Step([[maybe_unused]] ApplicationStep::UpdateEngines token, Args ... args) final { OnApplicationUpdateEngines(args...); }
    };

    template<typename... Args>
    class IStepApplicationRender : public IStep<ApplicationStep::Render, Args...>
    {
    protected:
        virtual ~IStepApplicationRender() = default;
    public:
        virtual void OnApplicationRender(Args ... args) = 0;
        virtual void Step([[maybe_unused]] ApplicationStep::Render token, Args ... args) final { OnApplicationRender(args...); }
    };

    template<typename... Args>
    class IStepApplicationCloseFrame : public IStep<ApplicationStep::CloseFrame, Args...>
    {
    protected:
        virtual ~IStepApplicationCloseFrame() = default;
    public:
        virtual void OnApplicationCloseFrame(Args ... args) = 0;
        virtual void Step([[maybe_unused]] ApplicationStep::CloseFrame token, Args ... args) final { OnApplicationCloseFrame(args...); }
    };
}