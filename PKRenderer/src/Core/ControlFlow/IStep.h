#pragma once
#include <typeindex>

namespace PK::Core::ControlFlow
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