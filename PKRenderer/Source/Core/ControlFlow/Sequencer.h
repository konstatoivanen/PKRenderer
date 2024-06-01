#pragma once
#include <vector>
#include <unordered_map>
#include <utility>
#include "Core/ControlFlow/IStep.h"

namespace PK
{
    class Sequencer
    {
    public:
        struct Step
        {
            std::type_index type = std::type_index(typeid(IBaseStep));
            void* step = nullptr;

            template<typename ... Args>
            static Step Create(IStep<Args...>* s) { return { IStep<Args...>::GetStepTypeId(), static_cast<IStep<Args...>*>(s) }; }
        };

        struct To
        {
            To(std::initializer_list<Step> steps)
            {
                for (auto& step : steps)
                {
                    this->steps[step.type].push_back(step);
                }
            }

            std::unordered_map<std::type_index, std::vector<Step>> steps;
        };

        using Steps = std::unordered_map<const void*, To>;

        void SetSteps(std::initializer_list<Steps::value_type> steps)
        {
            m_steps = Steps(steps);
        }

        inline const void* GetRoot() { return this; }

        template<typename ... Args>
        void Next(const void* engine, Args ... args)
        {
            auto engineStepsIter = m_steps.find(engine);

            if (engineStepsIter != m_steps.end())
            {
                using TStep = IStep<Args...>;
                auto typeId = TStep::GetStepTypeId();
                auto& engineSteps = engineStepsIter->second.steps;
                auto stepsIter = engineSteps.find(typeId);

                if (stepsIter != engineSteps.end())
                {
                    auto steps = stepsIter->second;

                    for (auto& i : steps)
                    {
                        reinterpret_cast<TStep*>(i.step)->Step(std::forward<Args>(args)...);
                    }
                }
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

        inline void Release() { m_steps.clear(); }

    private:
        Steps m_steps;
    };
}