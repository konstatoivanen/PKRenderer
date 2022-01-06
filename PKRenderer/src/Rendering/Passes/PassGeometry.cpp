#include "PrecompiledHeader.h"
#include "PassGeometry.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
#include "Math/FunctionsIntersect.h"

namespace PK::Rendering::Passes
{
    using namespace PK::ECS::Tokens;
    using namespace PK::ECS::EntityViews;

    PassGeometry::PassGeometry(EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher) : m_entityDb(entityDb), m_sequencer(sequencer), m_batcher(batcher)
    {
    }
    
    void PassGeometry::Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float depthRange)
    {
        visibilityList->Clear();
        
        TokenCullFrustum tokenFrustum{};
        tokenFrustum.results = visibilityList;
        tokenFrustum.mask = RenderableFlags::Mesh;
        Functions::ExtractFrustrumPlanes(viewProjection, &tokenFrustum.planes, true);
        m_sequencer->Next(engineRoot, &tokenFrustum);

        if (visibilityList->count == 0)
        {
            return;
        }

        m_passGroup = m_batcher->BeginNewGroup();

        for (auto i = 0u; i < visibilityList->count; ++i)
        {
            auto& item = (*visibilityList)[i];
            auto entity = m_entityDb->Query<MeshRenderableView>(EGID(item.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));
            auto submesh = 0u;

            for (auto& material : entity->materials->sharedMaterials)
            {
                auto transform = entity->transform;
                auto shader = material->GetShader();
                m_batcher->SubmitDraw(transform, shader, material, entity->mesh->sharedMesh, submesh++, 0u);
            }
        }
    }

    void PassGeometry::Render(CommandBuffer* cmd) { m_batcher->Render(cmd, m_passGroup); }
}