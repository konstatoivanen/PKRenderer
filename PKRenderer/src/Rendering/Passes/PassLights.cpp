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
        auto shadowsB = (b->renderable->flags & RenderableFlags::CastShadows) != 0;

        if (shadowsA < shadowsB)
        {
            return 1;
        }

        if (shadowsA > shadowsB)
        {
            return -1;
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
        m_computeCopyCubeShadow = assetDatabase->Find<Shader>("CS_CopyCubeShadow");

        auto hash = HashCache::Get();

        m_cascadeLinearity = config->CascadeLinearity;
        m_shadowmapTileSize = config->ShadowmapTileSize;
        m_shadowmapCubeFaceSize = (uint)sqrt((m_shadowmapTileSize * m_shadowmapTileSize) / 6);

        m_shadowmapTypeData[(int)LightType::Point].TileCount = 1u;
        m_shadowmapTypeData[(int)LightType::Point].MaxBatchSize = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Point].LayerStride = 6u;
        m_shadowmapTypeData[(int)LightType::Spot].TileCount = 1u;
        m_shadowmapTypeData[(int)LightType::Spot].MaxBatchSize = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Spot].LayerStride = 1u;
        m_shadowmapTypeData[(int)LightType::Directional].TileCount = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Directional].MaxBatchSize = 1u;
        m_shadowmapTypeData[(int)LightType::Directional].LayerStride = PK_SHADOW_CASCADE_COUNT;

        TextureDescriptor depthDesc;
        depthDesc.samplerType = SamplerType::CubemapArray;
        depthDesc.resolution = { m_shadowmapCubeFaceSize, m_shadowmapCubeFaceSize , 1u };
        depthDesc.format = TextureFormat::Depth16;
        depthDesc.layers = 6 * PK_SHADOW_CASCADE_COUNT;
        depthDesc.sampler.wrap[0] = WrapMode::Mirror;
        depthDesc.sampler.wrap[1] = WrapMode::Mirror;
        depthDesc.sampler.wrap[2] = WrapMode::Mirror;
        depthDesc.sampler.filterMin = FilterMode::Bilinear;
        depthDesc.sampler.filterMag = FilterMode::Bilinear;
        depthDesc.usage = TextureUsage::RTDepthSample;
        m_depthTargetCube = Texture::Create(depthDesc, "Lights.DepthTarget.Cube");

        depthDesc.usage = TextureUsage::RTColorSample;
        depthDesc.format = TextureFormat::R32F;
        m_shadowTargetCube = Texture::Create(depthDesc, "Lights.ShadowTarget.Cube");

        depthDesc.samplerType = SamplerType::Sampler2DArray;
        depthDesc.format = TextureFormat::Depth16;
        depthDesc.resolution = { m_shadowmapTileSize, m_shadowmapTileSize, 1u };
        depthDesc.layers = PK_SHADOW_CASCADE_COUNT;
        depthDesc.usage = TextureUsage::RTDepth;
        m_depthTarget2D = Texture::Create(depthDesc, "Lights.DepthTarget.2D");

        TextureDescriptor atlasDescriptor;
        atlasDescriptor.samplerType = SamplerType::Sampler2DArray;
        atlasDescriptor.format = TextureFormat::R32F;
        atlasDescriptor.usage = TextureUsage::Sample | TextureUsage::Storage | TextureUsage::RTColor;
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
        imageDescriptor.resolution = { config->InitialWidth / LightGridTileSizePx, config->InitialHeight / LightGridTileSizePx, LightGridSizeZ };
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

        auto lightIndexCount = imageDescriptor.resolution.x *
                               imageDescriptor.resolution.y *
                               imageDescriptor.resolution.z *
                               MaxLightsPerTile;

        m_lightMatricesBuffer = Buffer::Create(ElementType::Float4x4, 32, BufferUsage::PersistentStorage, "Lights.Matrices");
        m_lightsLists = Buffer::Create(ElementType::Ushort, lightIndexCount, BufferUsage::DefaultStorage, "Lights.List");
        GraphicsAPI::SetBuffer(hash->pk_LightLists, m_lightsLists.get());
        GraphicsAPI::SetImage(hash->pk_LightTiles, m_lightTiles.get());
    }

    void PassLights::Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float znear, float zfar)
    {
        TokenCullFrustum cullFrustum;
        cullFrustum.results = visibilityList;
        cullFrustum.depthRange = zfar - znear;
        cullFrustum.mask = RenderableFlags::Light;
        cullFrustum.matrix = viewProjection;

        visibilityList->Clear();
        m_sequencer->Next(engineRoot, &cullFrustum);

        if (visibilityList->count == 0)
        {
            return;
        }

        uint lightCount = (uint)visibilityList->count;
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
        auto matricesView = matrixCount > 0 ? cmd->BeginBufferWrite<float4x4>(m_lightMatricesBuffer.get(), 0u, matrixCount) : BufferView<float4x4>();

        auto ivp = glm::inverse(viewProjection);
        auto zsplits = GetCascadeZSplits(znear, zfar);
        TokenCullCubeFaces cullCube;
        TokenCullCascades cullCascades;
        cullCube.results = visibilityList;
        cullCascades.results = visibilityList;
        cullCube.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
        cullFrustum.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
        cullCascades.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
        cullCascades.count = PK_SHADOW_CASCADE_COUNT;

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
            light.sourceRadius = view->light->sourceRadius;
            light.direction = 0u;

            auto castShadows = (view->renderable->flags & RenderableFlags::CastShadows) != 0;
            visibilityList->Clear();

            switch (view->light->type)
            {
                case LightType::Directional:
                {
                    light.position = transform->rotation * PK_FLOAT3_FORWARD;

                    Functions::ShadowCascadeCreateInfo cascadeInfo{};
                    cascadeInfo.worldToLocal = worldToLocal;
                    cascadeInfo.projToWorld = ivp;
                    cascadeInfo.splitPlanes = zsplits.data();
                    cascadeInfo.zPadding = -view->light->radius;
                    cascadeInfo.resolution = m_shadowmapTileSize;
                    cascadeInfo.count = PK_SHADOW_CASCADE_COUNT;
                    Functions::GetShadowCascadeMatrices(cascadeInfo, matricesView.data + matrixIndex, &light.radius);

                    if (castShadows)
                    {
                        float minDepth = 0.0f;
                        cullCascades.cascades = matricesView.data + matrixIndex;
                        cullCascades.depthRange = light.radius;
                        m_sequencer->Next(engineRoot, &cullCascades);
                        light.shadowIndex = BuildShadowBatch(visibilityList, view, i, cullCascades.depthRange, &shadowCount, &minDepth);

                        // Regenerate cascades as the depth range might change based on culling
                        cascadeInfo.zPadding = -view->light->radius + minDepth;
                        Functions::GetShadowCascadeMatrices(cascadeInfo, matricesView.data + matrixIndex, &light.radius);
                    }
                }
                break;

                case LightType::Point:
                {
                    if (castShadows)
                    {
                        cullCube.depthRange = view->light->radius - 0.1f;
                        cullCube.aabb = view->bounds->worldAABB;
                        m_sequencer->Next(engineRoot, &cullCube);
                        light.shadowIndex = BuildShadowBatch(visibilityList, view, i, cullCube.depthRange, &shadowCount);
                    }
                }
                break;

                case LightType::Spot:
                {
                    matricesView[matrixIndex] = Functions::GetPerspectiveInvZ(view->light->angle, 1.0f, 0.1f, view->light->radius) * worldToLocal;
                    light.direction = Math::Functions::OctaEncodeUint(transform->rotation * PK_FLOAT3_FORWARD);
                    
                    if (castShadows)
                    {
                        cullFrustum.depthRange = view->light->radius - 0.1f;
                        cullFrustum.matrix = matricesView[matrixIndex];
                        m_sequencer->Next(engineRoot, &cullFrustum);
                        light.shadowIndex = BuildShadowBatch(visibilityList, view, i, cullFrustum.depthRange, &shadowCount);
                    }
                }
                break;
            }

            matrixIndex += m_shadowmapTypeData[(uint32_t)view->light->type].TileCount;
        }

        // Empty last one for clustering
        lightsView[lightCount] = LightPacked();
            
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

        Texture* renderTargets[2]{ nullptr, nullptr };
        TextureViewRange viewRanges[2]{};

        for (const auto& batch : m_shadowBatches)
        {
            auto batchType = batch.batchType;
            auto& shadow = m_shadowmapTypeData[(int)batchType];
            auto tileCount = shadow.TileCount * batch.count;

            cmd->BeginDebugScope("ShadowBatch", PK_COLOR_RED);

            if (batchType == LightType::Point)
            { 
                auto range = TextureViewRange(0, 0, 0, shadow.LayerStride * batch.count);
                cmd->SetRenderTarget({ m_depthTargetCube.get(), m_shadowTargetCube.get() }, { range, range }, true);
                cmd->ClearDepth(0.0f, 0u);
                cmd->ClearColor(color(batch.maxDepthRange), 0u);
                m_batcher->Render(cmd, batch.batchGroup);
                GraphicsAPI::SetTexture(hash->pk_Texture, m_shadowTargetCube.get());
                GraphicsAPI::SetImage(hash->pk_Image, m_shadowmaps.get(), TextureViewRange(0, atlasIndex, 1, tileCount));
                cmd->DispatchWithCounter(m_computeCopyCubeShadow, 0, { m_shadowmapTileSize, m_shadowmapTileSize, tileCount });
            }
            else
            {
                cmd->SetRenderTarget({ m_depthTarget2D.get(), m_shadowmaps.get() }, { { 0u, 0u, 0u, (uint16_t)tileCount }, { 0u, (uint16_t)atlasIndex, 1u, (uint16_t)tileCount } }, true);
                cmd->ClearColor(color(batch.maxDepthRange), 0u);
                cmd->ClearDepth(0.0f, 0u);
                m_batcher->Render(cmd, batch.batchGroup);
            }

            cmd->EndDebugScope();

            atlasIndex += tileCount;
        }
    }

    void PassLights::ComputeClusters(CommandBuffer* cmd, Math::uint3 resolution)
    {
        auto hash = HashCache::Get();

        resolution.x /= LightGridTileSizePx;
        resolution.y /= LightGridTileSizePx;
        resolution.z = LightGridSizeZ;

        auto lightIndexCount = resolution.x *
                               resolution.y *
                               resolution.z *
                               MaxLightsPerTile;

        if (m_lightsLists->Validate(lightIndexCount))
        {
            GraphicsAPI::SetBuffer(hash->pk_LightLists, m_lightsLists.get());
        }

        if (m_lightTiles->Validate(resolution))
        {
            GraphicsAPI::SetImage(hash->pk_LightTiles, m_lightTiles.get());
        }

        cmd->DispatchWithCounter(m_computeLightAssignment, m_lightTiles->GetResolution());
    }

    ShadowCascades PassLights::GetCascadeZSplits(float znear, float zfar) const
    {
        ShadowCascades cascadeSplits;
        Functions::GetCascadeDepths(znear, zfar, m_cascadeLinearity, cascadeSplits.data(), 5);

        // Snap z ranges to tile indices to make shader branching more coherent
        auto scale = LightGridSizeZ / glm::log2(zfar / znear);
        auto bias = LightGridSizeZ * -log2(znear) / log2(zfar / znear);

        for (auto i = 1; i < 4; ++i)
        {
            float zTile = round(log2(cascadeSplits[i]) * scale + bias);
            cascadeSplits[i] = znear * pow(zfar / znear, zTile / LightGridSizeZ);
        }

        return cascadeSplits;
    }

    uint32_t PassLights::BuildShadowBatch(VisibilityList* visibilityList,
        LightRenderableView* view, 
        uint32_t index, 
        float maxDepth, 
        uint32_t* outShadowCount,
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
        uint32_t shadowmapIndex = *outShadowCount;
        *outShadowCount += m_shadowmapTypeData[(int)view->light->type].TileCount;

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
