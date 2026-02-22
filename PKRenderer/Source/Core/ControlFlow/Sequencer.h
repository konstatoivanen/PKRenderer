#pragma once
#include <utility>
#include "Core/Utilities/FastBuffer.h"
#include "Core/Utilities/FastMap.h"
#include "Core/ControlFlow/IStep.h"

namespace PK
{
    struct Sequencer
    {
        struct Step
        {
            uint64_t type = 0u;
            IBaseStep* step = nullptr;

            template<typename ... Args>
            static Step Create(IStep<Args...>* s) { return { IStep<Args...>::GetStepTypeId(), static_cast<IStep<Args...>*>(s) }; }
        };

        struct StepsKey
        {
            const void* caller;
            uint64_t type;

            inline bool operator == (const StepsKey& r) const noexcept
            {
                return caller == r.caller && type == r.type;
            }
        };

        struct StepsView
        {
            Step* steps = nullptr;
            size_t count = 0ull;
        };

        void SetSteps(const std::initializer_list<std::tuple<const void*, std::initializer_list<Step>>>& initializer);
        void Release();

        inline const void* GetRoot() const { return this; }

        template<typename ... Args>
        void Next(const void* engine, Args ... args)
        {
            using TStep = IStep<Args...>;
            auto viewPtr = m_map.GetValuePtr({ engine, TStep::GetStepTypeId() });
            auto view = viewPtr ? *viewPtr : StepsView();

            for (auto i = 0u; i < view.count; ++i)
            {
                static_cast<TStep*>(view.steps[i].step)->Step(std::forward<Args>(args)...);
            }
        }

        template<typename ... Args>
        void NextRoot(Args ... args) { Next(this, std::forward<Args>(args)...); }

        template<typename T, typename ... Args>
        void NextEmplace(const void* engine, Args&& ... args)
        {
            auto token = T(std::forward<Args>(args)...);
            Next<T*>(engine, &token);
        }

    private:
        FastBuffer<Step> m_steps;
        FastMap16<StepsKey, StepsView, Hash::TFNV1AHash<StepsKey>> m_map;
    };
}
