#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStepApplication.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    class EngineUpdateTransforms : public IStepApplicationUpdateEngines<>
    {
    public:
        EngineUpdateTransforms(EntityDatabase* entityDb);
        virtual void OnApplicationUpdateEngines() final;

    private:
        EntityDatabase* m_entityDb = nullptr;
    };
}