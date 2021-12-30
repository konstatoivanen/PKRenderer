#include "PrecompiledHeader.h"
#include "Sequencer.h"

namespace PK::Core::Services
{
    To::To(std::initializer_list<BranchSteps::value_type> branchSteps) : m_branchSteps(branchSteps) {}
    To::To(std::initializer_list<Step> commonSteps) : m_commonSteps(commonSteps) {}
    To::To(std::initializer_list<BranchSteps::value_type> branchSteps, std::initializer_list<Step> commonSteps) : m_branchSteps(branchSteps), m_commonSteps(commonSteps) {}
    To::To(std::initializer_list<Step> commonSteps, std::initializer_list<BranchSteps::value_type> branchSteps) : m_branchSteps(branchSteps), m_commonSteps(commonSteps) {}

    const std::vector<Step>* To::GetSteps(int condition)
    {
        if (m_branchSteps.count(condition) < 1)
        {
            return nullptr;
        }

        return &m_branchSteps.at(condition);
    }

    void Sequencer::SetSteps(std::initializer_list<Steps::value_type> steps)
    {
        m_steps = Steps(steps);
    }
}