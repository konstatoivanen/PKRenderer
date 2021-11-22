#pragma once
#include "PrecompiledHeader.h"
#include "Core/IService.h"
#include "Utilities/Ref.h"

// @TODO Replace this nastyness with templates or smth.
#define PK_STEP_T(S, D) static_cast<PK::ECS::IStep<D>*>(S)
#define PK_STEP_C(S, D) static_cast<PK::ECS::IConditionalStep<D>*>(S)
#define PK_STEP_S(S) static_cast<PK::ECS::ISimpleStep*>(S)

namespace PK::ECS
{
    class IBaseStep
    {
        protected: virtual ~IBaseStep() = default;
    };

    template <typename T>
    class IStep : public IBaseStep
    {
        protected: virtual ~IStep() = default;
        public: virtual void Step(T* token) = 0;
    };

    template <typename T>
    class IConditionalStep : public IBaseStep
    {
        protected: virtual ~IConditionalStep() = default;
        public: virtual void Step(T* token, int condition) = 0;
    };

    class ISimpleStep : public IConditionalStep<void>
    {
        protected:
            virtual ~ISimpleStep() = default;
        public:
            virtual void Step(int condition) = 0;
            void Step(void* token, int condition) { Step(condition); }
    };

    typedef IBaseStep* StepPtr;
    typedef std::unordered_map<int, std::vector<StepPtr>> BranchSteps;

    class To
    {
        public:
            To(std::initializer_list<BranchSteps::value_type> branchSteps);
            To(std::initializer_list<StepPtr> commonSteps);
            To(std::initializer_list<BranchSteps::value_type> branchSteps, std::initializer_list<StepPtr> commonSteps);
            To(std::initializer_list<StepPtr> commonSteps, std::initializer_list<BranchSteps::value_type> steps);

            const std::vector<StepPtr>* GetSteps(int condition);
            constexpr const std::vector<StepPtr>* GetCommonSteps() const { return &m_commonSteps; }
 
        private:
            BranchSteps m_branchSteps;
            std::vector<StepPtr> m_commonSteps;
    };

    typedef std::unordered_map<const void*, To> Steps;

    class Sequencer : public Core::IService
    {
        public:
            void SetSteps(std::initializer_list<Steps::value_type> steps);

            inline const void* GetRoot() { return this; }

            template<typename T>
            void Next(const void* engine, T* token, int condition)
            {
                if (m_steps.count(engine) < 1)
                {
                    return;
                }

                auto& target = m_steps.at(engine);
                const auto* branchSteps = target.GetSteps(condition);
                
                if (branchSteps != nullptr)
                {
                    InvokeSteps(branchSteps, token, condition);
                }

                InvokeSteps(target.GetCommonSteps(), token, condition);
            }

            template<typename T>
            void InvokeRootStep(T* token, int condition) { Next<T>(this, token, condition); }

            inline void InvokeRootStep(int condition) { Next<void>(this, nullptr, condition); }

            inline void Release() { m_steps.clear(); }

        private:
            template<typename T>
            void InvokeSteps(const std::vector<StepPtr>* branchSteps, T* token, int condition)
            {
                auto& steps = *branchSteps;

                for (auto& i : steps)
                {
                    if (auto* conditionalStep = dynamic_cast<IConditionalStep<T>*>(i))
                    {
                        conditionalStep->Step(token, condition);
                    }
                    
                    if (auto* step = dynamic_cast<IStep<T>*>(i))
                    {
                        step->Step(token);
                    }
                }
            }

            Steps m_steps;
    };
}