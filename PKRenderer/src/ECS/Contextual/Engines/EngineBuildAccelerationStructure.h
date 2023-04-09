#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Tokens/AccelerationStructureBuildToken.h"

namespace PK::ECS::Engines
{
    class EngineBuildAccelerationStructure : public Core::Services::IService,
        public Core::Services::IStep<Tokens::AccelerationStructureBuildToken>
    {
    public:
        EngineBuildAccelerationStructure(EntityDatabase* entityDb);
        void Step(Tokens::AccelerationStructureBuildToken* token) override final;

    private:
        EntityDatabase* m_entityDb = nullptr;
        std::vector<EGID> m_renderableEgids;
    };
}