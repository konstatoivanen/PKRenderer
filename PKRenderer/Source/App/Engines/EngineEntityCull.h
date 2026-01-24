#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "App/Renderer/EntityCulling.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct EntityDatabase)

namespace PK::App
{
    class EngineEntityCull : 
        public IStep<IArena*, RequestEntityCullFrustum*>,
        public IStep<IArena*, RequestEntityCullCubeFaces*>,
        public IStep<IArena*, RequestEntityCullCascades*>
    {
    public:
        EngineEntityCull(EntityDatabase* entityDb) : m_entityDb(entityDb) {};
        virtual void Step(IArena* frameArena, RequestEntityCullFrustum* request) final;
        virtual void Step(IArena* frameArena, RequestEntityCullCubeFaces* request) final;
        virtual void Step(IArena* frameArena, RequestEntityCullCascades* request) final;

    private:
        EntityDatabase* m_entityDb = nullptr;
    };
}
