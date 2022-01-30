#include "PrecompiledHeader.h"
#include "PassLights.h"
#include "Utilities/VectorUtilities.h"
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
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
    struct ShadowmapRenderData
    {
        uint pk_ShadowmapLightIndex;
        uint pk_ShadowmapBaseLayer;
        float4 pk_ShadowmapBlurAmount;
    };

    PassLights::PassLights(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher, const ApplicationConfig* config) :
        m_entityDb(entityDb), 
        m_sequencer(sequencer), 
        m_batcher(batcher),
        m_lights(1024)
    {
        m_computeLightAssignment = assetDatabase->Find<Shader>("LightAssignment");
        m_shadowmapBlur = assetDatabase->Find<Shader>("ShadowmapBlur");
        
        auto hash = HashCache::Get();

        m_cascadeLinearity = config->CascadeLinearity;
        m_shadowmapTileSize = config->ShadowmapTileSize;
        m_shadowmapTileCount = config->ShadowmapTileCount;
        m_shadowmapCubeFaceSize = (uint)sqrt((m_shadowmapTileSize * m_shadowmapTileSize) / 6);

        auto descriptor = RenderTextureDescriptor();
        descriptor.samplerType = SamplerType::CubemapArray;
        descriptor.resolution = { m_shadowmapCubeFaceSize, m_shadowmapCubeFaceSize , 1u };
        descriptor.colorFormats[0] =  { TextureFormat::RG32F };
        descriptor.depthFormat = TextureFormat::Depth16;
        descriptor.layers = 6 * PK_SHADOW_CASCADE_COUNT;
        descriptor.sampler.wrap[0] = WrapMode::Clamp;
        descriptor.sampler.wrap[1] = WrapMode::Clamp;
        descriptor.sampler.wrap[2] = WrapMode::Clamp;
        descriptor.sampler.filter = FilterMode::Bilinear;
        descriptor.usage = TextureUsage::Sample;
        m_shadowmapTypeData[(int)LightType::Point].SceneRenderTarget = CreateRef<RenderTexture>(descriptor);
        m_shadowmapTypeData[(int)LightType::Point].BlurPass0 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_CUBE, hash->SHADOW_BLUR_PASS0 });
        m_shadowmapTypeData[(int)LightType::Point].BlurPass1 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_CUBE, hash->SHADOW_BLUR_PASS1 });
        m_shadowmapTypeData[(int)LightType::Point].TileCount = 1u;
        m_shadowmapTypeData[(int)LightType::Point].LayerStride = 6u;

        descriptor.samplerType = SamplerType::Sampler2DArray;
        descriptor.resolution = { m_shadowmapTileSize, m_shadowmapTileSize, 1u };
        descriptor.layers = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Spot].SceneRenderTarget = CreateRef<RenderTexture>(descriptor);
        m_shadowmapTypeData[(int)LightType::Spot].BlurPass0 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D, hash->SHADOW_BLUR_PASS0 });
        m_shadowmapTypeData[(int)LightType::Spot].BlurPass1 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D, hash->SHADOW_BLUR_PASS1 });
        m_shadowmapTypeData[(int)LightType::Spot].TileCount = 1u;
        m_shadowmapTypeData[(int)LightType::Spot].LayerStride = 1u;

        m_shadowmapTypeData[(int)LightType::Directional].SceneRenderTarget = m_shadowmapTypeData[(int)LightType::Spot].SceneRenderTarget;
        m_shadowmapTypeData[(int)LightType::Directional].BlurPass0 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D, hash->SHADOW_BLUR_PASS0 });
        m_shadowmapTypeData[(int)LightType::Directional].BlurPass1 = m_shadowmapBlur->GetVariantIndex({ hash->SHADOW_SOURCE_2D, hash->SHADOW_BLUR_PASS1 });
        m_shadowmapTypeData[(int)LightType::Directional].TileCount = PK_SHADOW_CASCADE_COUNT;
        m_shadowmapTypeData[(int)LightType::Directional].LayerStride = PK_SHADOW_CASCADE_COUNT;
        
        TextureDescriptor atlasDescriptor;
        atlasDescriptor.samplerType = SamplerType::Sampler2DArray;
        atlasDescriptor.format = TextureFormat::RG32F;
        atlasDescriptor.usage = TextureUsage::RTColorSample;
        atlasDescriptor.layers = 32;
        atlasDescriptor.resolution = { m_shadowmapTileSize, m_shadowmapTileSize, 1u };
        atlasDescriptor.sampler.wrap[0] = WrapMode::Clamp;
        atlasDescriptor.sampler.wrap[1] = WrapMode::Clamp;
        atlasDescriptor.sampler.wrap[2] = WrapMode::Clamp;
        atlasDescriptor.sampler.filter = FilterMode::Bilinear;
        m_shadowmaps = Texture::Create(atlasDescriptor);

        TextureDescriptor imageDescriptor;
        imageDescriptor.samplerType = SamplerType::Sampler3D;
        imageDescriptor.format = TextureFormat::R32UI;
        imageDescriptor.usage = TextureUsage::Storage;
        imageDescriptor.resolution = { GridSizeX, GridSizeY, GridSizeZ };
        imageDescriptor.sampler.filter = FilterMode::Point;
        imageDescriptor.sampler.wrap[0] = WrapMode::Clamp;
        imageDescriptor.sampler.wrap[1] = WrapMode::Clamp;
        imageDescriptor.sampler.wrap[2] = WrapMode::Clamp;
        m_lightTiles = Texture::Create(imageDescriptor);

        m_lightsBuffer = Buffer::CreateStorage(
        {
            { ElementType::Float4, "POSITION"},
            { ElementType::Float4, "COLOR"},
            { ElementType::Uint4, "INDICES"},
        },
        1024, BufferUsage::PersistentStage);

        m_lightMatricesBuffer = Buffer::CreateStorage({ { ElementType::Float4x4, "MATRIX" } }, 32, BufferUsage::PersistentStage);
        m_lightDirectionsBuffer = Buffer::CreateStorage({ { ElementType::Float4, "DIRECTION"} }, 32, BufferUsage::PersistentStage);
        m_globalLightsList = Buffer::CreateStorage({ {ElementType::Int, "INDEX"} }, ClusterCount * MaxLightsPerTile);
        m_globalLightIndex = Buffer::CreateStorage({ {ElementType::Uint, "INDEX"} }, 1);
        
        auto cmd = GraphicsAPI::GetCommandBuffer();
        cmd->SetBuffer(hash->pk_GlobalLightsList, m_globalLightsList.get());
        cmd->SetBuffer(hash->pk_GlobalListListIndex, m_globalLightIndex.get());
        cmd->SetImage(hash->pk_LightTiles, m_lightTiles.get());
    }

    void PassLights::Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float znear, float zfar)
    {
        m_shadowmapCount = 0u;
        m_projectionCount = 0u;
        m_cascadeSplits = GetCascadeZSplits(znear, zfar);

        CullTokens tokens{};
        tokens.frustum.results = visibilityList;
        tokens.cube.results = visibilityList;
        tokens.cascades.results = visibilityList;

        tokens.frustum.depthRange = zfar - znear;
        tokens.frustum.mask = RenderableFlags::Light;
        Functions::ExtractFrustrumPlanes(viewProjection, &tokens.frustum.planes, true);

        visibilityList->Clear();
        m_sequencer->Next(engineRoot, &tokens.frustum);
        m_lightCount = (uint32_t)visibilityList->count;

        if (visibilityList->count == 0)
        {
            return;
        }

        auto inverseViewProjection = glm::inverse(viewProjection);

        for (auto i = 0U; i < visibilityList->count; ++i)
        {
            m_lights[i] = m_entityDb->Query<LightRenderableView>(EGID((*visibilityList)[i].entityId, (uint)ENTITY_GROUPS::ACTIVE));
        }

        Vector::QuickSort(m_lights.GetData(), m_lightCount);

        for (auto i = 0u; i < m_lightCount; ++i)
        {
            auto& view = m_lights[i];
            auto info = view->lightFrameInfo;
            info->batchGroup = 0xFFFF;
            info->shadowmapIndex = 0xFFFF;
            info->projectionIndex = view->light->type == LightType::Directional || view->light->type == LightType::Spot ? m_projectionCount : 0xFFFF;
            m_projectionCount += view->light->type == LightType::Directional ? PK_SHADOW_CASCADE_COUNT : view->light->type == LightType::Spot ? 1 : 0;

            if ((view->renderable->flags & RenderableFlags::CastShadows) != 0)
            {
                BuildShadowmapBatches(engineRoot, &tokens, view, inverseViewProjection);
            }
        }

        m_lightsBuffer->Validate(m_lightCount + 1);
        m_lightMatricesBuffer->Validate(m_projectionCount);
        m_lightDirectionsBuffer->Validate(m_projectionCount);

        auto lightsView = m_lightsBuffer->BeginWrite<PK_Light>(0, m_lightCount + 1);
        auto matricesView = m_projectionCount > 0 ? m_lightMatricesBuffer->BeginWrite<float4x4>(0, m_projectionCount) : BufferView<float4x4>();
        auto directionsView = m_projectionCount > 0 ? m_lightDirectionsBuffer->BeginWrite<float4>(0, m_projectionCount) : BufferView<float4>();

        for (auto i = 0u; i < m_lightCount; ++i)
        {
            auto& view = m_lights[i];
            auto info = view->lightFrameInfo;
            auto position = PK_FLOAT4_ZERO;

            switch (view->light->type)
			{
				case LightType::Point:
					position = float4(view->transform->position, view->light->radius);
					break;

				case LightType::Spot:
					position = float4(view->transform->position, view->light->radius);
                    matricesView[info->projectionIndex] = Functions::GetPerspective(view->light->angle, 1.0f, 0.1f, view->light->radius) * view->transform->worldToLocal;
                    directionsView[info->projectionIndex] = float4(view->transform->rotation * PK_FLOAT3_FORWARD, view->light->angle * PK_FLOAT_DEG2RAD);
					break;

                case LightType::Directional:
					position = float4(view->transform->rotation * PK_FLOAT3_FORWARD, 0.0f);
					position.w = Functions::GetShadowCascadeMatrices(
						view->transform->worldToLocal, 
						inverseViewProjection, 
						m_cascadeSplits.planes, 
						-view->light->radius + info->minShadowDepth, 
						PK_SHADOW_CASCADE_COUNT, 
                        matricesView.data + info->projectionIndex);
					break;
			}

            lightsView[i] =
            {
                position,
                view->light->color,
                {
                    info->shadowmapIndex, 
                    info->projectionIndex, 
                    (ushort)view->light->cookie,
                    (ushort)view->light->type
                }
            };
        }

        lightsView[m_lightCount] = { PK_FLOAT4_ZERO, PK_COLOR_CLEAR, { 0xFFFF, 0u, 0xFFFF, 0xFFFF }};
        m_lightsBuffer->EndWrite();

        if (m_projectionCount > 0)
        {
            m_lightMatricesBuffer->EndWrite();
            m_lightDirectionsBuffer->EndWrite();
        }
    }

    void PassLights::Render(CommandBuffer* cmd)
    {
        auto hash = HashCache::Get();
        cmd->Clear(m_globalLightIndex.get(), 0, sizeof(uint32_t), 0u);
        
        if (m_shadowmaps->GetLevels() <= m_shadowmapCount + PK_SHADOW_CASCADE_COUNT)
        {
            m_shadowmaps->Validate(1u, m_shadowmapCount + PK_SHADOW_CASCADE_COUNT);
        }

        cmd->SetConstant<uint32_t>(hash->pk_LightCount, m_lightCount);
        cmd->SetBuffer(hash->pk_Lights, m_lightsBuffer.get());
        cmd->SetBuffer(hash->pk_LightMatrices, m_lightMatricesBuffer.get());
        cmd->SetBuffer(hash->pk_LightDirections, m_lightDirectionsBuffer.get());
        cmd->SetTexture(hash->pk_ShadowmapAtlas, m_shadowmaps.get());

        ShadowmapRenderData renderData{};

        auto atlasIndex = 0u;

        for (auto i = 0u; i < m_lightCount;)
        {
            auto view = m_lights[i];

            if (view->lightFrameInfo->batchGroup == 0xFFFF)
            {
                ++i;
                continue;
            }

            auto batchSize = 1u;
            auto batchType = view->light->type;
            auto maxDepth = view->lightFrameInfo->maxShadowDepth - view->lightFrameInfo->minShadowDepth;
            auto& shadow = m_shadowmapTypeData[(int)batchType];
            auto maxBatchSize = PK_SHADOW_CASCADE_COUNT - shadow.TileCount + 1;
            maxBatchSize = glm::min(maxBatchSize, m_lightCount - i);
            renderData.pk_ShadowmapBlurAmount[0] = view->light->shadowBlur;

            for (; batchSize < maxBatchSize; ++batchSize)
            {
                view = m_lights[i + batchSize];
            
                if (view->lightFrameInfo->batchGroup == 0xFFFF || view->light->type != batchType)
                {
                    break;
                }

                maxDepth = glm::max(view->lightFrameInfo->maxShadowDepth - view->lightFrameInfo->minShadowDepth, maxDepth);
            }

            auto tileCount = shadow.TileCount * batchSize;

            cmd->SetRenderTarget(shadow.SceneRenderTarget.get(), true);
            cmd->ClearColor(color(maxDepth, maxDepth * maxDepth, 0.0f, 0.0f), 0u);
            cmd->ClearDepth(1.0f, 0u);

            for (auto j = 0u; j < batchSize; ++j)
            {
                auto group = m_lights[i]->lightFrameInfo->batchGroup;
                renderData.pk_ShadowmapBlurAmount[j] = m_lights[i]->light->shadowBlur;
                renderData.pk_ShadowmapBaseLayer = j * shadow.LayerStride;
                renderData.pk_ShadowmapLightIndex = i++;
                cmd->SetConstant(hash->pk_ShadowmapData, renderData);
                m_batcher->Render(cmd, group);
            }

            cmd->SetViewPort({ 0, 0, m_shadowmapTileSize, m_shadowmapTileSize });
            cmd->SetScissor({ 0, 0, m_shadowmapTileSize, m_shadowmapTileSize });

            auto range0 = TextureViewRange(0, atlasIndex + tileCount, 1, tileCount);
            cmd->SetRenderTarget(m_shadowmaps.get(), range0);
            cmd->SetTexture(hash->pk_ShadowmapSource, shadow.SceneRenderTarget->GetColor(0));
            cmd->Blit(m_shadowmapBlur, tileCount, 0u, shadow.BlurPass0);
            
            auto range1 = TextureViewRange(0, atlasIndex, 1, tileCount);
            cmd->SetRenderTarget(m_shadowmaps.get(), range1);
            cmd->SetTexture(hash->pk_ShadowmapSource, m_shadowmaps.get(), range0);
            cmd->Blit(m_shadowmapBlur, tileCount, 0u, shadow.BlurPass1);

            atlasIndex += tileCount;
        }

        cmd->Dispatch(m_computeLightAssignment, { 1,1, GridSizeZ / 4 });
        cmd->Barrier(m_globalLightsList.get(), MemoryAccessFlags::ComputeReadWrite, MemoryAccessFlags::FragmentBuffer);
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

    void PassLights::BuildShadowmapBatches(void* engineRoot, CullTokens* tokens, LightRenderableView* view, const float4x4& inverseViewProjection)
    {
        auto info = view->lightFrameInfo;

        auto visibilityList = tokens->frustum.results;

        visibilityList->Clear();

        switch (view->light->type)
        {
            case LightType::Point:
            {
                tokens->cube.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
                tokens->cube.depthRange = info->maxShadowDepth = view->light->radius - 0.1f;
                tokens->cube.aabb = view->bounds->worldAABB;
                m_sequencer->Next(engineRoot, &tokens->cube);
            }
            break;

            case LightType::Spot:
            {
                tokens->frustum.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
                tokens->frustum.depthRange = info->maxShadowDepth = view->light->radius - 0.1f;
                auto projection = Functions::GetPerspective(view->light->angle, 1.0f, 0.1f, view->light->radius) * view->transform->worldToLocal;
                Functions::ExtractFrustrumPlanes(projection, &tokens->frustum.planes, true);
                m_sequencer->Next(engineRoot, &tokens->frustum);
            }
            break;

            case LightType::Directional:
            {
                float4x4 cascades[PK_SHADOW_CASCADE_COUNT];
                FrustumPlanes planes[PK_SHADOW_CASCADE_COUNT];

                auto lightRange = Functions::GetShadowCascadeMatrices(
                    view->transform->worldToLocal,
                    inverseViewProjection,
                    m_cascadeSplits.planes,
                    -view->light->radius,
                    PK_SHADOW_CASCADE_COUNT,
                    cascades);

                for (auto j = 0u; j < PK_SHADOW_CASCADE_COUNT; ++j)
                {
                    Functions::ExtractFrustrumPlanes(cascades[j], &planes[j], true);
                }

                tokens->cascades.depthRange = info->maxShadowDepth = lightRange;
                tokens->cascades.count = PK_SHADOW_CASCADE_COUNT;
                tokens->cascades.cascades = planes;
                tokens->cascades.mask = RenderableFlags::Mesh | RenderableFlags::CastShadows;
                m_sequencer->Next(engineRoot, &tokens->cascades);
            }
            break;
        }

        if (visibilityList->count == 0)
        {
            return;
        }

        uint32_t minDepth = 0xFFFFFFFF;
        info->shadowmapIndex = m_shadowmapCount;
        info->batchGroup = m_batcher->BeginNewGroup();
        m_shadowmapCount += view->light->type == LightType::Directional ? PK_SHADOW_CASCADE_COUNT : 1;

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
                    m_batcher->SubmitDraw(transform, shader, nullptr, entity->mesh->sharedMesh, kv.submesh, item.clipId);
                }
            }
        }

        info->minShadowDepth = (minDepth * info->maxShadowDepth) / (float)0xFFFF;
    }
}
