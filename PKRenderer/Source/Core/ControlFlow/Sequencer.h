#pragma once
#include <vector>
#include <utility>
#include "Core/Utilities/MemoryBlock.h"
#include "Core/Utilities/FastMap.h"
#include "Core/ControlFlow/IStep.h"

namespace PK
{
    struct Sequencer
    {
        struct Step
        {
            uint64_t type = 0u;
            void* step = nullptr;

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

        void SetSteps(std::initializer_list<std::tuple<const void*, std::initializer_list<Step>>> initializer)
        {
            auto count = 0u;

            for (auto& pair : initializer)
            {
                count += std::get<1>(pair).size();
            }

            // Allocate for worst case where all are unique.
            m_steps.Validate(count);
            m_map.Reserve(count, 2ull); 
            auto head = m_steps.GetData();

            for (auto& pair : initializer)
            {
                auto caller = std::get<0>(pair);
                auto& steps = std::get<1>(pair);
                auto index = 0u; 

                for (auto& current : steps)
                {
                    if (m_map.AddKey({ caller, current.type }, &index))
                    {
                        auto& view = m_map[index].value;
                        view.steps = head;
                        view.count = 0u;

                        for (auto other = &current; other != steps.end(); other++)
                        {
                            if (current.type == other->type)
                            {
                                view.steps[view.count++] = *other;
                            }
                        }

                        head += view.count;
                    }
                }
            }
        }

        inline const void* GetRoot() { return this; }

        template<typename ... Args>
        void Next(const void* engine, Args ... args)
        {
            using TStep = IStep<Args...>;
            auto viewPtr = m_map.GetValuePtr({ engine, TStep::GetStepTypeId() });
            auto view = viewPtr ? *viewPtr : StepsView();

            for (auto i = 0u; i < view.count; ++i)
            {
                reinterpret_cast<TStep*>(view.steps[i].step)->Step(std::forward<Args>(args)...);
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

        inline void Release() 
        {
            m_steps.Clear(); 
            m_map.Clear();
        }

    private:
        MemoryBlock<Step> m_steps;
        FastMap<StepsKey, StepsView, Hash::TFNV1AHash<StepsKey>> m_map;
    };
}
