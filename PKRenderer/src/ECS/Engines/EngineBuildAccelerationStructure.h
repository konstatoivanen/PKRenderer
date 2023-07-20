#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Tokens/AccelerationStructureBuildToken.h"

namespace PK::ECS::Engines
{
    class EngineBuildAccelerationStructure : public Core::Services::IService,
        public Core::Services::IStep<Tokens::TokenAccelerationStructureBuild>
    {
        public:
            EngineBuildAccelerationStructure(EntityDatabase* entityDb);
            void Step(Tokens::TokenAccelerationStructureBuild* token) final;

        private:
            EntityDatabase* m_entityDb = nullptr;
            std::vector<EGID> m_renderableEgids;
    };
}