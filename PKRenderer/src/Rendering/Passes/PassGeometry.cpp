#include "PrecompiledHeader.h"
#include "PassGeometry.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
#include "Math/FunctionsIntersect.h"
#include "Rendering/HashCache.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace ECS;
    using namespace ECS::Tokens;
    using namespace ECS::EntityViews;
    using namespace Objects;
    using namespace Structs;

    PassGeometry::PassGeometry(EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher) : m_entityDb(entityDb), m_sequencer(sequencer), m_batcher(batcher)
    {
        m_gbufferAttribs.depthStencil.depthCompareOp = Comparison::LessEqual;
        m_gbufferAttribs.depthStencil.depthWriteEnable = true;
        m_gbufferAttribs.rasterization.cullMode = CullMode::Back;
        m_gbufferAttribs.blending.colorMask = ColorMask::RGBA;
    }
    
    void PassGeometry::Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float depthRange)
    {
        visibilityList->Clear();
        
        TokenCullFrustum tokenFrustum{};
        tokenFrustum.results = visibilityList;
        tokenFrustum.mask = RenderableFlags::Mesh;
        Functions::ExtractFrustrumPlanes(viewProjection, &tokenFrustum.planes, true);
        m_sequencer->Next(engineRoot, &tokenFrustum);
        m_passGroup = 0xFFFFFFFF;

        if (visibilityList->count == 0)
        {
            return;
        }

        m_passGroup = m_batcher->BeginNewGroup();

        for (auto i = 0u; i < visibilityList->count; ++i)
        {
            auto& item = (*visibilityList)[i];
            auto entity = m_entityDb->Query<MeshRenderableView>(EGID(item.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));

            for (auto& kv : entity->materials->materials)
            {
                auto transform = entity->transform;
                auto shader = kv.material->GetShader();
                m_batcher->SubmitDraw(transform, shader, kv.material, entity->mesh->sharedMesh, kv.submesh, 0u);
            }
        }
    }

    void PassGeometry::RenderForward(CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("Forward Opaque", PK_COLOR_BLUE);
        m_batcher->Render(cmd, m_passGroup);
        cmd->EndDebugScope();
    }

    void PassGeometry::RenderGBuffer(CommandBuffer* cmd)
    {
        cmd->BeginDebugScope("GBuffer", PK_COLOR_RED);
        m_batcher->Render(cmd, m_passGroup, &m_gbufferAttribs, HashCache::Get()->PK_META_PASS_GBUFFER);
        cmd->EndDebugScope();
    }
}