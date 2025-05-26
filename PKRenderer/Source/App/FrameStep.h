#pragma once
#include "Core/ControlFlow/IStep.h"

namespace PK::App
{
    struct FrameContext;

    struct FrameStep
    {
        struct Initialize {};
        struct Update {};
        struct Render {};
        struct Finalize {};
    };

    // Note: no perfect forwarding as we assume user knows which types to declare as forwarded, etc.
    template<typename... Args>
    class IStepFrameInitialize : public IStep<FrameStep::Initialize, FrameContext*, Args...>
    {
    protected:
        virtual ~IStepFrameInitialize() = default;
    public:
        virtual void OnStepFrameInitialize([[maybe_unused]] FrameContext* ctx, Args ... args) = 0;
        virtual void Step([[maybe_unused]] FrameStep::Initialize token, [[maybe_unused]] FrameContext* ctx, Args ... args) final { OnStepFrameInitialize(ctx, args...); }
    };

    template<typename... Args>
    class IStepFrameUpdate : public IStep<FrameStep::Update, FrameContext*, Args...>
    {
    protected:
        virtual ~IStepFrameUpdate() = default;
    public:
        virtual void OnStepFrameUpdate([[maybe_unused]] FrameContext* ctx, Args ... args) = 0;
        virtual void Step([[maybe_unused]] FrameStep::Update token, [[maybe_unused]] FrameContext* ctx, Args ... args) final { OnStepFrameUpdate(ctx, args...); }
    };

    template<typename... Args>
    class IStepFrameRender : public IStep<FrameStep::Render, FrameContext*, Args...>
    {
    protected:
        virtual ~IStepFrameRender() = default;
    public:
        virtual void OnStepFrameRender([[maybe_unused]] FrameContext* ctx, Args ... args) = 0;
        virtual void Step([[maybe_unused]] FrameStep::Render token, [[maybe_unused]] FrameContext* ctx, Args ... args) final { OnStepFrameRender(ctx, args...); }
    };

    template<typename... Args>
    class IStepFrameFinalize : public IStep<FrameStep::Finalize, FrameContext*, Args...>
    {
    protected:
        virtual ~IStepFrameFinalize() = default;
    public:
        virtual void OnStepFrameFinalize([[maybe_unused]] FrameContext* ctx, Args ... args) = 0;
        virtual void Step([[maybe_unused]] FrameStep::Finalize token, [[maybe_unused]] FrameContext* ctx, Args ... args) final { OnStepFrameFinalize(ctx, args...); }
    };
}
