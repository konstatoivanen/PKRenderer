#include "PrecompiledHeader.h"
#include "Math/FunctionsIntersect.h"
#include "Core/ControlFlow/Sequencer.h"
#include "ECS/EntityDatabase.h"
#include "ECS/EntityViewStaticMesh.h"
#include "Rendering/Geometry/IBatcher.h"
#include "Rendering/Objects/RenderView.h"
#include "Rendering/HashCache.h"
#include "Rendering/EntityCulling.h"
#include "Rendering/IRenderPipeline.h"
#include "EngineDrawGeometry.h"

namespace PK::Engines
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::ControlFlow;
    using namespace PK::ECS;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;

    EngineDrawGeometry::EngineDrawGeometry(EntityDatabase* entityDb, Sequencer* sequencer) :
        m_entityDb(entityDb),
        m_sequencer(sequencer)
    {
        m_gbufferAttribs.depthStencil.depthCompareOp = Comparison::GreaterEqual;
        m_gbufferAttribs.depthStencil.depthWriteEnable = true;
        m_gbufferAttribs.rasterization.cullMode = CullMode::Back;
        m_gbufferAttribs.blending.colorMask = ColorMask::RGBA;
    }

    void EngineDrawGeometry::Step(RenderPipelineEvent* renderEvent)
    {
        auto view = renderEvent->context->views[0];

        switch (renderEvent->type)
        {
            case RenderPipelineEvent::CollectDraws:
            {
                RequestEntityCullFrustum cullRequest;
                cullRequest.mask = ScenePrimitiveFlags::Mesh;
                cullRequest.matrix = view->worldToClip;
                m_sequencer->Next(this, &cullRequest);
                view->primaryPassGroup = 0xFFFFFFFF;

                if (cullRequest.GetCount() > 0)
                {
                    view->primaryPassGroup = renderEvent->context->batcher->BeginNewGroup();

                    for (auto i = 0u; i < cullRequest.GetCount(); ++i)
                    {
                        auto& info = cullRequest[i];
                        auto entity = m_entityDb->Query<EntityViewStaticMesh>(EGID(info.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));

                        for (auto& kv : entity->materials->materials)
                        {
                            auto transform = entity->transform;
                            auto shader = kv.material->GetShader();
                            renderEvent->context->batcher->SubmitStaticMeshDraw(transform, shader, kv.material, entity->staticMesh->sharedMesh, kv.submesh, 0u, info.depth);
                        }
                    }
                }
            }
            return;
            case RenderPipelineEvent::Depth:
            {
                renderEvent->context->batcher->RenderGroup(renderEvent->cmd, view->primaryPassGroup, &m_gbufferAttribs, HashCache::Get()->PK_META_PASS_GBUFFER);
            }
            return;
            case RenderPipelineEvent::GBuffer:
            {
                renderEvent->context->batcher->RenderGroup(renderEvent->cmd, view->primaryPassGroup, nullptr, HashCache::Get()->PK_META_PASS_GBUFFER);
            }
            return;
            case RenderPipelineEvent::ForwardOpaque:
            {
                renderEvent->context->batcher->RenderGroup(renderEvent->cmd, view->primaryPassGroup);
            }
            return;
            default: return;
        }
    }
}