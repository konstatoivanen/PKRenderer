#include "PrecompiledHeader.h"
#include "PassLights.h"
#include "Utilities/VectorUtilities.h"
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "ECS/EntityViews/MeshRenderableView.h"
#include "Rendering/HashCache.h"

using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Math;
using namespace PK::Utilities;
using namespace PK::ECS;
using namespace PK::ECS::Tokens;
using namespace PK::ECS::EntityViews;
using namespace PK::Rendering::Objects;
using namespace PK::Rendering::Structs;

template<>
struct Vector::Comparer<LightRenderableView*>
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
    PassLights::PassLights(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher, const ApplicationConfig* config) :
        m_entityDb(entityDb),
        m_sequencer(sequencer),
        m_batcher(batcher),
        m_lights(1024)
    {
        m_computeLightAssignment = assetDatabase->Find<Shader>("CS_LightAssignment");
        m_shadowmapBlur = assetDatabase->Find<Shader>("CS_ShadowmapBlur");

        auto hash = HashCache::Get();

        m_cascadeLinearity = config->CascadeLinearity;
        m_shadowmapTileSize = config->ShadowmapTileSize;
        m_shadowmapCubeFaceSize = (uint)sqrt((m_shadowmapTileSize * m_shadowmapTileSize) / 6);

        auto descriptor = RenderTextureDescriptor();
        descriptor.samplerType = SamplerType::CubemapArray;
        descriptor.resolution = { m_shadowmapCubeFaceSize, m_shadowmapCubeFaceSize , 1u };
        descriptor.colorFormats[0] = { TextureFormat::RG32F };
        descriptor.depthFormat = TextureFormat::Depth16;
        descriptor.layers = 6 * PK_SHADOW_CASCADE_COUNT;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.usage = TextureUsage::Sample;
        m_shadowmapTypeData[(int)LightType::Point].SceneRenderTarget = CreateRef<RenderTexture>(descriptor, "Lights.Shadowmap.PointRenderTarget");
        m_shadowmapTypeData[(int)LightType::Point].BlurPass0 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_CUBE });
        m_shadowmapTypeData[(int)LightType::Point].BlurPass1 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D });
        m_shadowmapTypeData[(int)LightType::Point].TileCount = 1u;
        m_shadowmapTypeData[(int)LightType::Point].MaxBatchSize = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Point].LayerStride = 6u;

        descriptor.samplerType = SamplerType::Sampler2DArray;
        descriptor.resolution = { m_shadowmapTileSize, m_shadowmapTileSize, 1u };
        descriptor.layers = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Spot].SceneRenderTarget = CreateRef<RenderTexture>(descriptor, "Lights.Shadowmap.SpotRenderTarget");
        m_shadowmapTypeData[(int)LightType::Spot].BlurPass0 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D });
        m_shadowmapTypeData[(int)LightType::Spot].BlurPass1 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D });
        m_shadowmapTypeData[(int)LightType::Spot].TileCount = 1u;
        m_shadowmapTypeData[(int)LightType::Spot].MaxBatchSize = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Spot].LayerStride = 1u;

        m_shadowmapTypeData[(int)LightType::Directional].SceneRenderTarget = m_shadowmapTypeData[(int)LightType::Spot].SceneRenderTarget;
        m_shadowmapTypeData[(int)LightType::Directional].BlurPass0 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D });
        m_shadowmapTypeData[(int)LightType::Directional].BlurPass1 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D });
        m_shadowmapTypeData[(int)LightType::Directional].TileCount = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Directional].MaxBatchSize = 1u;
        m_shadowmapTypeData[(int)LightType::Directional].LayerStride = PK_SHADOW_CASCADE_COUNT;

        TextureDescriptor atlasDescriptor;
        atlasDescriptor.samplerType = SamplerType::Sampler2DArray;
        atlasDescriptor.format = TextureFormat::RG32F;
        atlasDescriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        atlasDescriptor.layers = 32;
        atlasDescriptor.resolution = { m_shadowmapTileSize, m_shadowmapTileSize, 1u };
        atlasDescriptor.sampler.wrap[0] = WrapMode::Clamp;
        atlasDescriptor.sampler.wrap[1] = WrapMode::Clamp;
        atlasDescriptor.sampler.wrap[2] = WrapMode::Clamp;
        atlasDescriptor.sampler.filterMin = FilterMode::Bilinear;
        atlasDescriptor.sampler.filterMag = FilterMode::Bilinear;
        m_shadowmaps = Texture::Create(atlasDescriptor, "Lights.Shadowmap.Atlas");

        TextureDescriptor imageDescriptor;
        imageDescriptor.samplerType = SamplerType::Sampler3D;
        imageDescriptor.format = TextureFormat::R32UI;
        imageDescriptor.usage = TextureUsage::Storage | TextureUsage::Concurrent;
        imageDescriptor.resolution = { GridSizeX, GridSizeY, GridSizeZ };
        imageDescriptor.sampler.filterMin = FilterMode::Point;
        imageDescriptor.sampler.filterMag = FilterMode::Point;
        imageDescriptor.sampler.wrap[0] = WrapMode::Clamp;
        imageDescriptor.sampler.wrap[1] = WrapMode::Clamp;
        imageDescriptor.sampler.wrap[2] = WrapMode::Clamp;
        m_lightTiles = Texture::Create(imageDescriptor, "Lights.Tiles");

        m_lightsBuffer = Buffer::Create(
            {
                { ElementType::Float4, "PACKED0"},
                { ElementType::Float4, "PACKED1"},
                { ElementType::Uint4, "PACKED2"},
            },
            1024, BufferUsage::PersistentStorage, "Lights");

        m_lightMatricesBuffer = Buffer::Create(ElementType::Float4x4, 32, BufferUsage::PersistentStorage, "Lights.Matrices");
        m_globalLightsList = Buffer::Create(ElementType::Int, ClusterCount * MaxLightsPerTile, BufferUsage::DefaultStorage, "Lights.List");

        GraphicsAPI::SetBuffer(hash->pk_GlobalLightsList, m_globalLightsList.get());
        GraphicsAPI::SetImage(hash->pk_LightTiles, m_lightTiles.get());
    }

    void PassLights::Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float znear, float zfar)
    {

        CullTokens tokens{};
        tokens.frustum.results = visibilityList;
        tokens.cube.results = visibilityList;
        tokens.cascades.results = visibilityList;
        tokens.frustum.depthRange = zfar - znear;
        tokens.frustum.mask = RenderableFlags::Light;
        Functions::ExtractFrustrumPlanes(viewProjection, &tokens.frustum.planes, true);

        visibilityList->Clear();
        m_sequencer->Next(engineRoot, &tokens.frustum);
        auto lightCount = (uint32_t)visibilityList->count;

        if (visibilityList->count == 0)
        {
            return;
        }

        uint matrixCount = 0u;
        uint matrixIndex = 0u;
        uint shadowCount = 0u;

        for (auto i = 0U; i < visibilityList->count; ++i)
        {
            auto view = m_entityDb->Query<LightRenderableView>(EGID((*visibilityList)[i].entityId, (uint)ENTITY_GROUPS::ACTIVE));
            matrixCount += m_shadowmapTypeData[(int)view->light->type].TileCount;
            m_lights[i] = view;
        }

        Vector::QuickSort(m_lights.GetData(), lightCount);

        m_shadowBatches.clear();
        m_lightsBuffer->Validate(lightCount + 1);
        m_lightMatricesBuffer->Validate(matrixCount);

        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
        auto lightsView = cmd->BeginBufferWrite<LightPacked>(m_lightsBuffer.get(), 0u, lightCount + 1);
        auto matricesView = matrixCount > 0 ? cmd->BeginBufferWrite<float4x4>(m_lightMatricesBuffer.get(), 0u, projectionCount) : BufferView<float4x4>();

        auto ivp = glm::inverse(viewProjection);
        auto zsplits = GetCascadeZSplits(znear, zfar);
        tokens.cube.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
        tokens.frustum.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
        tokens.cascades.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
        FrustumPlanes planes[PK_SHADOW_CASCADE_COUNT];
        tokens.cascades.cascades = planes;
        tokens.cascades.count = PK_SHADOW_CASCADE_COUNT;

        for (auto i = 0u; i < lightCount; ++i)
        {
            auto& view = m_lights[i];
            auto& light = lightsView[i];
            auto& transform = view->transform;
            auto& worldToLocal = transform->worldToLocal;
            light.matrixIndex = matrixIndex;
            light.shadowIndex = 0xFFFFu;
            light.color = view->light->color;
            light.cookie = (ushort)view->light->cookie;
            light.type = (ushort)view->light->type;
            light.position = transform->position;
            light.radius = view->light->radius;
            light.angle = view->light->angle * PK_FLOAT_DEG2RAD;
            light.direction = 0u;

            auto castShadows = (view->renderable->flags & RenderableFlags::CastShadows) != 0;
            visibilityList->Clear();

            switch (view->light->type)
            {
                case LightType::Point:
                {
                    if (castShadows)
                    {
                        auto shadowBlurAmount = 2.0f * glm::sin(view->light->shadowBlur * 0.5f * PK_FLOAT_HALF_PI);
                        tokens.cube.depthRange = view->light->radius - 0.1f;
                        tokens.cube.aabb = view->bounds->worldAABB;
                        m_sequencer->Next(engineRoot, &tokens.cube);
                        light.shadowIndex = BuildShadowBatch(visibilityList, view, i, shadowCount, shadowBlurAmount, tokens.cube.depthRange);
                    }
                }
                break;

                case LightType::Spot:
                {
                    matricesView[matrixIndex] = Functions::GetPerspective(view->light->angle, 1.0f, 0.1f, view->light->radius) * worldToLocal;
                    light.direction = Math::Functions::OctaEncodeUint(transform->rotation * PK_FLOAT3_FORWARD);
                    
                    if (castShadows)
                    {
                        auto shadowBlurAmount = view->light->shadowBlur * glm::sin(0.5f * view->light->angle * PK_FLOAT_DEG2RAD) / PK_FLOAT_INV_SQRT2;
                        tokens.frustum.depthRange = view->light->radius - 0.1f;
                        Functions::ExtractFrustrumPlanes(matricesView[matrixIndex], &tokens.frustum.planes, true);
                        m_sequencer->Next(engineRoot, &tokens.frustum);
                        light.shadowIndex = BuildShadowBatch(visibilityList, view, i, shadowCount, shadowBlurAmount, tokens.frustum.depthRange);
                    }
                }
                break;

                case LightType::Directional:
                {
                    light.position = transform->rotation * PK_FLOAT3_FORWARD;
                    Functions::GetShadowCascadeMatrices(worldToLocal, ivp, zsplits.planes, -view->light->radius, m_shadowmapTileSize, PK_SHADOW_CASCADE_COUNT, matricesView.data + matrixIndex, &light.radius);

                    if (castShadows)
                    {
                        for (auto j = 0u; j < PK_SHADOW_CASCADE_COUNT; ++j)
                        {
                            Functions::ExtractFrustrumPlanes(matricesView[matrixIndex + j], &planes[j], true);
                        }

                        tokens.cascades.depthRange = light.radius;
                        m_sequencer->Next(engineRoot, &tokens.cascades);

                        float minDepth = 0.0f;
                        light.shadowIndex = BuildShadowBatch(visibilityList, view, i, shadowCount, view->light->shadowBlur, tokens.cascades.depthRange, &minDepth);

                        // Regenerate cascades as the depth range might change based on culling
                        Functions::GetShadowCascadeMatrices(worldToLocal, ivp, zsplits.planes, -view->light->radius + minDepth, m_shadowmapTileSize, PK_SHADOW_CASCADE_COUNT, matricesView.data + matrixIndex, &light.radius);
                    }
                }
                break;
            }

            matrixIndex += m_shadowmapTypeData[(uint32_t)view->light->type].TileCount;
        }

        // Empty last one for clustering
        lightsView[lightCount] =
        {
            PK_FLOAT3_ZERO,
            0.0f,
            PK_FLOAT3_ZERO,
            0.0f,
            0xFFFFu,
            0u,
            0xFFFFu,
            0xFFFFu,
            0u
        };
            
        cmd->EndBufferWrite(m_lightsBuffer.get());

        if (matrixCount > 0)
        {
            cmd->EndBufferWrite(m_lightMatricesBuffer.get());
        }

        if (m_shadowmaps->GetLayers() < shadowCount + PK_SHADOW_CASCADE_COUNT)
        {
            m_shadowmaps->Validate(1u, shadowCount + PK_SHADOW_CASCADE_COUNT);
        }

        auto hash = HashCache::Get();
        GraphicsAPI::SetConstant<uint32_t>(hash->pk_LightCount, lightCount);
        GraphicsAPI::SetBuffer(hash->pk_Lights, m_lightsBuffer.get());
        GraphicsAPI::SetBuffer(hash->pk_LightMatrices, m_lightMatricesBuffer.get());
        GraphicsAPI::SetTexture(hash->pk_ShadowmapAtlas, m_shadowmaps.get());
    }

    void PassLights::RenderShadows(Objects::CommandBuffer* cmd)
    {
        auto hash = HashCache::Get();
        auto atlasIndex = 0u;

        for (const auto& shadowBatch : m_shadowBatches)
        {
            auto batchType = shadowBatch.batchType;
            auto& shadow = m_shadowmapTypeData[(int)batchType];
            auto tileCount = shadow.TileCount * shadowBatch.count;

            cmd->BeginDebugScope("ShadowBatch", PK_COLOR_RED);

            cmd->SetRenderTarget(shadow.SceneRenderTarget.get(), true);
            cmd->ClearColor(color(shadowBatch.maxDepthRange, shadowBatch.maxDepthRange * shadowBatch.maxDepthRange, 0.0f, 0.0f), 0u);
            cmd->ClearDepth(1.0f, 0u);

            GraphicsAPI::SetConstant(hash->pk_ShadowmapData, shadowBatch.shadowBlurAmounts);
            m_batcher->Render(cmd, shadowBatch.batchGroup);

            GraphicsAPI::SetTexture(hash->pk_ShadowmapSource, shadow.SceneRenderTarget->GetColor(0));
            GraphicsAPI::SetImage(hash->_DestinationTex, m_shadowmaps.get(), TextureViewRange(0, atlasIndex, 1, tileCount));
            cmd->DispatchWithCounter(m_shadowmapBlur, shadow.BlurPass0, { m_shadowmapTileSize, m_shadowmapTileSize, tileCount });

            cmd->EndDebugScope();

            atlasIndex += tileCount;
        }
    }

    void PassLights::ComputeClusters(CommandBuffer* cmd)
    {
        cmd->DispatchWithCounter(m_computeLightAssignment, { GridSizeX , GridSizeY, GridSizeZ });
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

    uint32_t PassLights::BuildShadowBatch(VisibilityList* visibilityList,
        LightRenderableView* view, 
        uint32_t index, 
        uint32_t& outShadowCount,
        float shadowBlurAmount, 
        float maxDepth, 
        float* outMinDepth)
    {
        if (visibilityList->count == 0)
        {
            return 0xFFFFu;
        }

        auto& shadow = m_shadowmapTypeData[(int)view->light->type];

        if (m_shadowBatches.size() == 0 ||
            m_shadowBatches.back().count >= shadow.MaxBatchSize ||
            m_shadowBatches.back().batchType != view->light->type)
        {
            auto& newBatch = m_shadowBatches.emplace_back();
            newBatch.batchGroup = m_batcher->BeginNewGroup();
            newBatch.firstIndex = index;
            newBatch.batchType = view->light->type;
        }

        auto& batch = m_shadowBatches.back();

        uint32_t minDepth = 0xFFFFFFFF;
        uint32_t shadowmapIndex = outShadowCount;
        outShadowCount += m_shadowmapTypeData[(int)view->light->type].TileCount;

        for (auto i = 0u; i < visibilityList->count; ++i)
        {
            auto& item = (*visibilityList)[i];
            auto entity = m_entityDb->Query<MeshRenderableView>(EGID(item.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));

            if (item.depth < minDepth)
            {
                minDepth = item.depth;
            }

            for (auto& kv : entity->materials->materials)
            {
                auto transform = entity->transform;
                auto shader = kv.material->GetShadowShader();

                if (shader != nullptr)
                {
                    auto layerOffset = batch.count * shadow.LayerStride + item.clipId;
                    m_batcher->SubmitDraw(transform, shader, nullptr, entity->mesh->sharedMesh, kv.submesh, (index & 0xFFFF) | (layerOffset << 16));
                }
            }
        }

        // Scale blur amount to valid range (now one wants to blur by 90 degrees).
        shadowBlurAmount *= PK_FLOAT_INV_FOUR_PI;

        // Directional lights use same blur amount for all cascades.
        // Fill the vector so that every tile gets the correct value.
        // "Correct" is debatable as this should be calculated in accordance to cascade distribution.
        // @TODO FIX ME
        for (auto i = 0u; i < shadow.TileCount; ++i)
        {
            batch.shadowBlurAmounts[batch.count + i] = shadowBlurAmount / (1.0f + i * 0.25f);
        }

        auto minDepthView = (minDepth * maxDepth) / (float)0xFFFF;
        batch.maxDepthRange = glm::max(batch.maxDepthRange, maxDepth - minDepthView);
        batch.count++;

        if (outMinDepth)
        {
            *outMinDepth = minDepthView;
        }

        return shadowmapIndex;
    }
}
