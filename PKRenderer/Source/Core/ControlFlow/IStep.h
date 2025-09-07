#pragma once
#include "Core/Utilities/FastTypeIndex.h"

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
        friend struct Sequencer;
        friend struct Step;

    protected:
        virtual ~IStep() = default;
        static uint64_t GetStepTypeId() { return (uint64_t)pk_base_type_index<IStep<Args...>>(); }
    public:
        virtual void Step(Args ... args) = 0;
    };
}
