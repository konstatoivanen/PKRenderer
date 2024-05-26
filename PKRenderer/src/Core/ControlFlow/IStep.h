#pragma once
#include <typeindex>

// Note clang complains about hidden virtuals. we dont care about that. lets ignore it.
#ifdef __clang__
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

namespace PK
{
    class IBaseStep
    {
    protected: virtual ~IBaseStep() = default;
    };

    template <typename ... Args>
    class IStep : public IBaseStep
    {
        friend class Sequencer;
        friend struct Step;
        friend struct To;

    protected:
        virtual ~IStep() = default;
        static std::type_index GetStepTypeId() { return std::type_index(typeid(IStep<Args...>*)); }
    public:
        virtual void Step(Args ... args) = 0;
    };
}
