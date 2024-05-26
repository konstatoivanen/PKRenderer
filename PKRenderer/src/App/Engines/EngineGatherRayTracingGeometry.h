#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ECS/EGID.h"
#include "Core/ControlFlow/IStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    struct RequestEntityCullRayTracingGeometry;

    class EngineGatherRayTracingGeometry : public IStep<RequestEntityCullRayTracingGeometry*>
    {
    public:
        EngineGatherRayTracingGeometry(EntityDatabase* entityDb);
        virtual void Step(RequestEntityCullRayTracingGeometry* token) final;

    private:
        EntityDatabase* m_entityDb = nullptr;
    };
}