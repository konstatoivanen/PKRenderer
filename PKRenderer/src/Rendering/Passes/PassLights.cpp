#include "PrecompiledHeader.h"
#include "PassLights.h"
#include "Utilities/VectorUtilities.h"
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"

using namespace PK::Core;
using namespace PK::ECS::Tokens;
using namespace PK::ECS::EntityViews;
using namespace PK::Rendering::Objects;

template<>
struct PK::Utilities::Vector::Comparer<LightRenderableView*>
{
    int operator()(LightRenderableView*& a, LightRenderableView*& b)
    {
        auto shadowsA = (a->renderable->flags & RenderableFlags::CastShadows) != 0;
        auto shadowsB = (a->renderable->flags & RenderableFlags::CastShadows) != 0;

        if (shadowsA < shadowsB)
        {
            return -1;
        }

        if (shadowsA > shadowsB)
        {
            return 1;
        }

        if (a->light->type < b->light->type)
        {
            return -1;
        }

        if (a->light->type > b->light->type)
        {
            return 1;
        }

        return 0;
    }
};


namespace PK::Rendering::Passes
{
    PassLights::PassLights(EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher, float cascadeLinearity) : 
        m_entityDb(entityDb), 
        m_sequencer(sequencer), 
        m_batcher(batcher),
        m_cascadeLinearity(cascadeLinearity)
    {
        m_lights = Buffer::CreateStorage(
        {
            { ElementType::Float4, "COLOR"},
            { ElementType::Float4, "DIRECTION"},
            { ElementType::Uint, "SHADOWMAP_INDEX"},
            { ElementType::Uint, "PROJECTION_INDEX"},
            { ElementType::Uint, "COOKIE_INDEX"},
            { ElementType::Uint, "TYPE"},
        },
        1024);

        m_visibleLights.reserve(1024);
    }

    void PassLights::Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float znear, float zfar)
    {
        TokenCullFrustum tokenFrustum{};
        TokenCullCubeFaces tokenCubeFaces{};
        TokenCullCascades tokenCascades{};

        visibilityList->Clear();

        tokenFrustum.results = visibilityList;
        tokenCubeFaces.results = visibilityList;
        tokenCubeFaces.results = visibilityList;
        tokenFrustum.depthRange = zfar - znear;
        tokenFrustum.mask = RenderableFlags::Light;
        Functions::ExtractFrustrumPlanes(viewProjection, &tokenFrustum.planes, true);
        m_sequencer->Next(engineRoot, &tokenFrustum);

        m_visibleLights.resize(visibilityList->count);

        if (visibilityList->count == 0)
        {
            return;
        }

        for (auto i = 0U; i < visibilityList->count; ++i)
        {
            m_visibleLights[i] = m_entityDb->Query<LightRenderableView>(EGID((*visibilityList)[i].entityId, (uint)ENTITY_GROUPS::ACTIVE));
        }

        Vector::QuickSort(m_visibleLights);

        auto cascadeSplits = GetCascadeZSplits(znear, zfar);

        for (auto i = 0u; i < m_visibleLights.size(); ++i)
        {
            auto& view = m_visibleLights[i];
            view->light->batchGroup = i + 1;

            if ((view->renderable->flags & RenderableFlags::CastShadows) == 0)
            {
                break;
            }

            switch (view->light->type)
            {
                case LightType::Point:
                {
                    tokenCubeFaces.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
                    tokenCubeFaces.depthRange = view->light->radius - 0.1f;
                    tokenCubeFaces.aabb = view->bounds->worldAABB;
                    m_sequencer->Next(engineRoot, &tokenCubeFaces);
                }
                break;

                case LightType::Spot:
                {
                    tokenFrustum.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
                    tokenFrustum.depthRange = view->light->radius - 0.1f;
                    auto projection = Functions::GetPerspective(view->light->angle, 1.0f, 0.1f, view->light->radius) * view->transform->worldToLocal;
                    Functions::ExtractFrustrumPlanes(projection, &tokenFrustum.planes, true);
                    m_sequencer->Next(engineRoot, &tokenFrustum);
                }
                break;

                case LightType::Directional:
                {
                    float4x4 cascades[PK_SHADOW_CASCADE_COUNT];
                    FrustumPlanes planes[PK_SHADOW_CASCADE_COUNT];

                    auto lightRange = Functions::GetShadowCascadeMatrices(
                        view->transform->worldToLocal,
                        glm::inverse(viewProjection),
                        cascadeSplits.planes,
                        -view->light->radius,
                        PK_SHADOW_CASCADE_COUNT,
                        cascades);

                    for (auto j = 0u; j < PK_SHADOW_CASCADE_COUNT; ++j)
                    {
                        Functions::ExtractFrustrumPlanes(cascades[j], &planes[j], true);
                    }

                    tokenCascades.depthRange = lightRange;
                    tokenCascades.count = PK_SHADOW_CASCADE_COUNT;
                    tokenCascades.cascades = planes;
                    tokenCascades.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
                    m_sequencer->Next(engineRoot, &tokenCascades);
                }
                break;
            }
        }
    }

    void PassLights::RenderShadowmaps(CommandBuffer* cmd)
    {
    }

    void PassLights::RenderTiles(CommandBuffer* cmd)
    {
    }

    ShadowCascades PassLights::GetCascadeZSplits(float znear, float zfar) const
    {
        ShadowCascades cascadeSplits;
        Functions::GetCascadeDepths(znear, zfar, m_cascadeLinearity, cascadeSplits.planes, 5);

        // Snap z ranges to tile indices to make shader branching more coherent
        auto scale = GridSizeZ / glm::log2(zfar / znear);
        auto bias = GridSizeZ * -log2(znear) / log2(zfar / znear);

        for (auto i = 1; i < 4; ++i)
        {
            float zTile = round(log2(cascadeSplits.planes[i]) * scale + bias);
            cascadeSplits.planes[i] = znear * pow(zfar / znear, zTile / GridSizeZ);
        }

        return cascadeSplits;
    }

}
