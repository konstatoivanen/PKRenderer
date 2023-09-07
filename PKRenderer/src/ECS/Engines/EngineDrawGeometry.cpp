#include "PrecompiledHeader.h"
#include "EngineDrawGeometry.h"
#include "ECS/EntityViews/MeshRenderableView.h"
#include "Math/FunctionsIntersect.h"
#include "Rendering/HashCache.h"

namespace PK::ECS::Engines
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace ECS;
    using namespace ECS::Tokens;
    using namespace ECS::EntityViews;
    using namespace Rendering;
    using namespace Rendering::Objects;
    using namespace Rendering::Structs;

    EngineDrawGeometry::EngineDrawGeometry(EntityDatabase* entityDb, Sequencer* sequencer) :
        m_entityDb(entityDb), 
        m_sequencer(sequencer)
    {
        m_gbufferAttribs.depthStencil.depthCompareOp = Comparison::LessEqual;
        m_gbufferAttribs.depthStencil.depthWriteEnable = true;
        m_gbufferAttribs.rasterization.cullMode = CullMode::Back;
        m_gbufferAttribs.blending.colorMask = ColorMask::RGBA;
    }

    void Engines::EngineDrawGeometry::Step(TokenRenderEvent* token, int condition)
    {
        switch ((RenderEvent)condition)
        {
            case RenderEvent::CollectDraws:
            {
                token->visibilityList->Clear();

                TokenCullFrustum tokenFrustum{};
                tokenFrustum.results = token->visibilityList;
                tokenFrustum.mask = RenderableFlags::Mesh;
                tokenFrustum.matrix = token->viewProjection;
                m_sequencer->Next(this, &tokenFrustum);
                m_passGroup = token->outPassGroup = 0xFFFFFFFF;

                if (token->visibilityList->count == 0)
                {
                    return;
                }

                m_passGroup = token->outPassGroup = token->batcher->BeginNewGroup();

                for (auto i = 0u; i < token->visibilityList->count; ++i)
                {
                    auto& item = (*token->visibilityList)[i];
                    auto entity = m_entityDb->Query<MeshRenderableView>(EGID(item.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));

                    for (auto& kv : entity->materials->materials)
                    {
                        auto transform = entity->transform;
                        auto shader = kv.material->GetShader();
                        token->batcher->SubmitDraw(transform, shader, kv.material, entity->mesh->sharedMesh, kv.submesh, 0u);
                    }
                }
            }
            return;
            case RenderEvent::GBuffer:
            {
                token->batcher->Render(token->cmd, m_passGroup, &m_gbufferAttribs, HashCache::Get()->PK_META_PASS_GBUFFER);
            }
            return;
            case RenderEvent::ForwardOpaque:
            {
                token->batcher->Render(token->cmd, m_passGroup);
            }
            return;
        }

    }
}