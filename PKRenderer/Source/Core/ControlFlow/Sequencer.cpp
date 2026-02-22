#include "PrecompiledHeader.h"
#include "Sequencer.h"

namespace PK
{
    void Sequencer::SetSteps(const std::initializer_list<std::tuple<const void*, std::initializer_list<Step>>>& initializer)
    {
        auto count = 0u;

        for (auto& pair : initializer)
        {
            count += std::get<1>(pair).size();
        }

        // Allocate for worst case where all are unique.
        m_steps.Reserve(count);
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

    void PK::Sequencer::Release()
    {
        m_steps.Clear();
        m_map.Clear();
    }
}
