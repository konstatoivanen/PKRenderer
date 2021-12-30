#pragma once
#include "Core/Services/IService.h"
#include "Utilities/Ref.h"

namespace PK::Core::Services
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
            void Step(void* token, int condition) override final { Step(condition); }
    };

    struct Step
    {
        std::type_index type = std::type_index(typeid(IBaseStep));
        void* step = nullptr;

        template<typename T>
        static Step Token(IStep<T>* s) { return { std::type_index(typeid(IStep<T>*)), s }; }

        template<typename T>
        static Step Conditional(IConditionalStep<T>* s) { return { std::type_index(typeid(IConditionalStep<T>*)), s }; }

        inline static Step Simple(ISimpleStep* s) { return { std::type_index(typeid(IConditionalStep<void>*)), static_cast<IConditionalStep<void>*>(s) }; }
    };

    typedef std::unordered_map<int, std::vector<Step>> BranchSteps;

    class To
    {
        public:
            To(std::initializer_list<BranchSteps::value_type> branchSteps);
            To(std::initializer_list<Step> commonSteps);
            To(std::initializer_list<BranchSteps::value_type> branchSteps, std::initializer_list<Step> commonSteps);
            To(std::initializer_list<Step> commonSteps, std::initializer_list<BranchSteps::value_type> steps);

            const std::vector<Step>* GetSteps(int condition);
            constexpr const std::vector<Step>* GetCommonSteps() const { return &m_commonSteps; }
 
        private:
            BranchSteps m_branchSteps;
            std::vector<Step> m_commonSteps;
    };

    typedef std::unordered_map<const void*, To> Steps;

    class Sequencer : public IService
    {
        public:
            void SetSteps(std::initializer_list<Steps::value_type> steps);

            inline const void* GetRoot() { return this; }

            template<typename T>
            void Next(const void* engine, T* token, int condition = 0)
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
            void Next(T* token, int condition) { Next<T>(this, token, condition); }

            inline void Next(int condition) { Next<void>(this, nullptr, condition); }

            inline void Release() { m_steps.clear(); }

        private:
            template<typename T>
            void InvokeSteps(const std::vector<Step>* branchSteps, T* token, int condition)
            {
                using TToken = IStep<T>*;
                using TConditional = IConditionalStep<T>*;
                auto typeToken = std::type_index(typeid(TToken));
                auto typeConditional = std::type_index(typeid(TConditional));

                auto& steps = *branchSteps;

                for (auto& i : steps)
                {
                    if (i.type == typeConditional)
                    {
                        reinterpret_cast<TConditional>(i.step)->Step(token, condition);
                        continue;
                    }
                    
                    if (i.type == typeToken)
                    {
                        reinterpret_cast<TToken>(i.step)->Step(token);
                        continue;
                    }
                }
            }

            Steps m_steps;
    };
}