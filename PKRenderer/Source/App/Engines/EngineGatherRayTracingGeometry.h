#pragma once
#include "Core/ECS/EGID.h"
#include "Core/ControlFlow/IStep.h"

namespace PK { struct EntityDatabase; }

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
