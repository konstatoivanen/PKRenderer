#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/IService.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, class EntityDatabase)

namespace PK::Engines
{
    class EngineUpdateTransforms : public Core::IService,
        public Core::ControlFlow::IStepApplicationUpdateEngines
    {
    public:
        EngineUpdateTransforms(ECS::EntityDatabase* entityDb);
        virtual void OnApplicationUpdateEngines() final;

    private:
        ECS::EntityDatabase* m_entityDb = nullptr;
    };
}