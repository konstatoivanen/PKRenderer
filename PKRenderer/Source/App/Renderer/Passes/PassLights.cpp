#include "PrecompiledHeader.h"
#include <bend/bend_sss_cpu.h>
#include "Core/Utilities/FixedArena.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/ECS/EntityViewMeshStatic.h"
#include "App/ECS/EntityViewLight.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/EntityCulling.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "App/Renderer/RenderView.h"
#include "PassLights.h"

namespace PK::App
{
    struct SceneLight
    {
        float3 position;
        float3 color;
        float3 direction;
        float2 spot_angles;
        float radius;
        float source_radius;
        uint light_type;
        uint index_mask;
        uint index_matrix;
        uint index_shadow;
    };

    struct PackedLight
    {
        uint4 packed0 = PK_UINT4_ZERO;
        uint4 packed1 = PK_UINT4_ZERO;
    };

    static PackedLight PackLight(const SceneLight& light)
    {
        PackedLight packed{};
        packed.packed0.x = Math::PackHalfToUint(light.position.xy);
        packed.packed0.y = Math::PackHalfToUint({ light.position.z, light.radius });
        packed.packed0.z = Math::PackHalfToUint(light.color.xy);

        auto colorzfp16 = (uint32_t)Math::PackHalf(light.color.z);
        auto typeAndMaskIndex = (uint32_t)light.light_type | (uint32_t)(light.index_mask << 4u);
        packed.packed0.w = (colorzfp16 & 0xFFFFu) | (typeAndMaskIndex << 16u);
        packed.packed1.x = Math::PackHalfToUint(light.direction.xy);
        packed.packed1.y = Math::PackHalfToUint({ light.direction.z, light.source_radius });
        packed.packed1.z = Math::PackHalfToUint(light.spot_angles);
        packed.packed1.w = light.index_shadow | (light.index_matrix << 16u);
        return packed;
    }

    static int EntityViewLightPtrCompare(const void* a, const void* b)
    {
        auto lightA = *reinterpret_cast<EntityViewLight* const*>(a);
        auto lightB = *reinterpret_cast<EntityViewLight* const*>(b);
        auto keyA = (int32_t)lightA->light->type | ((int32_t)((lightA->primitive->flags & ScenePrimitiveFlags::CastShadows) == 0) << 4);
        auto keyB = (int32_t)lightB->light->type | ((int32_t)((lightB->primitive->flags & ScenePrimitiveFlags::CastShadows) == 0) << 4);
        return keyA - keyB;
    }

    PassLights::PassLights(AssetDatabase* assetDatabase) 
    {
        PK_LOG_VERBOSE_FUNC("");

        m_computeLightAssignment = assetDatabase->Find<ShaderAsset>("CS_LightAssignment").get();
        m_computeCopyCubeShadow = assetDatabase->Find<ShaderAsset>("CS_CopyCubeShadow").get();
        m_computeScreenSpaceShadow = assetDatabase->Find<ShaderAsset>("CS_ScreenspaceShadow").get();

        m_shadowTypeData[(int)LightType::Point].MatrixCount = 0u;
        m_shadowTypeData[(int)LightType::Point].TileCount = 1u;
        m_shadowTypeData[(int)LightType::Point].MaxBatchSize = ShadowCascadeCount;
        m_shadowTypeData[(int)LightType::Point].LayerStride = 6u;

        m_shadowTypeData[(int)LightType::Spot].MatrixCount = 1u;
        m_shadowTypeData[(int)LightType::Spot].TileCount = 1u;
        m_shadowTypeData[(int)LightType::Spot].MaxBatchSize = ShadowCascadeCount;
        m_shadowTypeData[(int)LightType::Spot].LayerStride = 1u;

        m_shadowTypeData[(int)LightType::Directional].MatrixCount = ShadowCascadeCount;
        m_shadowTypeData[(int)LightType::Directional].TileCount = ShadowCascadeCount;
        m_shadowTypeData[(int)LightType::Directional].MaxBatchSize = 1u;
        m_shadowTypeData[(int)LightType::Directional].LayerStride = ShadowCascadeCount;

        auto shadowCubeFaceSize = (uint)sqrt((m_shadowmapSize * m_shadowmapSize) / 6);
        TextureDescriptor depthDesc;
        depthDesc.type = TextureType::CubemapArray;
        depthDesc.resolution = { shadowCubeFaceSize , shadowCubeFaceSize , 1u };
        depthDesc.format = TextureFormat::Depth16;
        depthDesc.layers = 6 * ShadowCascadeCount;
        depthDesc.sampler.wrap[0] = WrapMode::Mirror;
        depthDesc.sampler.wrap[1] = WrapMode::Mirror;
        depthDesc.sampler.wrap[2] = WrapMode::Mirror;
        depthDesc.sampler.filterMin = FilterMode::Bilinear;
        depthDesc.sampler.filterMag = FilterMode::Bilinear;
        depthDesc.usage = TextureUsage::RTDepthSample;
        m_depthTargetCube = RHI::CreateTexture(depthDesc, "Lights.DepthTarget.Cube");

        depthDesc.usage = TextureUsage::RTColorSample;
        depthDesc.format = TextureFormat::R32F;
        m_shadowTargetCube = RHI::CreateTexture(depthDesc, "Lights.ShadowTarget.Cube");

        depthDesc.type = TextureType::Texture2DArray;
        depthDesc.format = TextureFormat::Depth16;
        depthDesc.resolution = { m_shadowmapSize.Value, m_shadowmapSize.Value, 1u };
        depthDesc.layers = ShadowCascadeCount;
        depthDesc.usage = TextureUsage::RTDepth;
        m_depthTarget2D = RHI::CreateTexture(depthDesc, "Lights.DepthTarget.2D");

        TextureDescriptor atlasDesc;
        atlasDesc.type = TextureType::Texture2DArray;
        atlasDesc.format = TextureFormat::R32F;
        atlasDesc.usage = TextureUsage::Sample | TextureUsage::Storage | TextureUsage::RTColor;
        atlasDesc.layers = ShadowCascadeCount * 2; // initial size assume 1 active directional light.
        atlasDesc.resolution = { m_shadowmapSize.Value, m_shadowmapSize.Value, 1u };
        atlasDesc.sampler.wrap[0] = WrapMode::Clamp;
        atlasDesc.sampler.wrap[1] = WrapMode::Clamp;
        atlasDesc.sampler.wrap[2] = WrapMode::Clamp;
        atlasDesc.sampler.filterMin = FilterMode::Bilinear;
        atlasDesc.sampler.filterMag = FilterMode::Bilinear;
        m_shadowmaps = RHI::CreateTexture(atlasDesc, "Lights.Shadowmap.Atlas");

        m_lightsBuffer = RHI::CreateBuffer<PackedLight>(1024ull, BufferUsage::PersistentStorage, "Lights");
        m_lightMatricesBuffer = RHI::CreateBuffer<float4x4>(32ull, BufferUsage::PersistentStorage, "Lights.Matrices");

        auto hash = HashCache::Get();
        auto lightCookies = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_LightCookies.pktexture")->GetRHI();

        auto sampler = lightCookies->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Clamp;
        sampler.wrap[1] = WrapMode::Clamp;
        sampler.wrap[2] = WrapMode::Clamp;
        lightCookies->SetSampler(sampler);
        RHI::SetTexture(hash->pk_LightCookies, lightCookies);
    }

    void PassLights::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        const auto tileZParams = Math::GetExponentialZParams(view->znear, view->zfar, m_tileZDistribution, LightGridSizeZ);
        const auto shadowCascadeZSplits = Math::GetCascadeDepthsFloat4(view->znear, view->zfar, m_cascadeDistribution, tileZParams);
        view->constants->Set<float4>(hash->pk_ShadowCascadeZSplits, shadowCascadeZSplits);
        view->constants->Set<float4>(hash->pk_LightTileZParams, float4(tileZParams, 0.0f));
    }

    void PassLights::BuildLights(RenderPipelineContext* context)
    {
        auto renderView = context->views[0];
        auto resources = renderView->GetResources<ViewResources>();
        auto culledLights = context->cullingProxy->CullFrustum(ScenePrimitiveFlags::Light, renderView->worldToClip);

        if (culledLights.GetCount() == 0)
        {
            return;
        }

        const auto clipToWorld = glm::inverse(renderView->worldToClip);
        const auto tileZParams = Math::GetExponentialZParams(renderView->znear, renderView->zfar, m_tileZDistribution, LightGridSizeZ);
        const auto shadowCasterMask = ScenePrimitiveFlags::Mesh | ScenePrimitiveFlags::CastShadows;
        const auto lightCount = (uint)culledLights.GetCount();
        auto matrixCount = 0u;
        auto matrixIndex = 0u;
        auto shadowCount = 0u;

        ShadowCascades cascadeZSplits;
        Math::GetCascadeDepths(renderView->znear, renderView->zfar, m_cascadeDistribution, cascadeZSplits.data(), tileZParams, 5);

        resources->lightViews = { context->frameArena->Allocate<EntityViewLight*>(lightCount), lightCount };
        resources->shadowBatches = { context->frameArena->GetHead<ShadowbatchInfo>(), 0ull };

        for (auto i = 0U; i < lightCount; ++i)
        {
            auto view = context->entityDb->Query<EntityViewLight>(EGID(culledLights[i].entityId, (uint)ENTITY_GROUPS::ACTIVE));
            context->frameArena->Allocate<ShadowbatchInfo>((view->primitive->flags & ScenePrimitiveFlags::CastShadows) != 0 ? 1u : 0u);
            matrixCount += m_shadowTypeData[(int)view->light->type].MatrixCount;
            resources->lightViews[i] = view;
        }
    
        qsort(resources->lightViews.data, lightCount, sizeof(EntityViewLight*), EntityViewLightPtrCompare);

        RHI::ValidateBuffer<PackedLight>(m_lightsBuffer, lightCount + 1u);
        RHI::ValidateBuffer<float4x4>(m_lightMatricesBuffer, matrixCount);

        CommandBufferExt cmd = RHI::GetCommandBuffer(QueueType::Transfer);
        auto packedLights = cmd.BeginBufferWrite<PackedLight>(m_lightsBuffer.get(), 0u, lightCount + 1u);
        auto matricesView = matrixCount > 0u ? cmd.BeginBufferWrite<float4x4>(m_lightMatricesBuffer.get(), 0u, matrixCount) : BufferView<float4x4>();

        for (auto lightIndex = 0u; lightIndex < lightCount; ++lightIndex)
        {
            auto view = resources->lightViews[lightIndex];
            const auto& transform = view->transform;
            const auto& worldToLocal = transform->worldToLocal;
            const auto& shadowTypeInfo = m_shadowTypeData[(uint32_t)view->light->type];
            const auto castShadows = (view->primitive->flags & ScenePrimitiveFlags::CastShadows) != 0;
            auto* matrices = matricesView.data + matrixIndex;

            SceneLight light{};
            light.position = transform->position;
            light.color = view->light->color;
            light.direction = transform->rotation * PK_FLOAT3_FORWARD;
            light.spot_angles = float2(view->light->angle * PK_FLOAT_DEG2RAD, 0.0f); // @TODO
            light.radius = view->light->radius;
            light.source_radius = view->light->sourceRadius;
            light.light_type = (uint)view->light->type;
            light.index_mask = (uint)view->light->cookie;
            light.index_matrix = matrixIndex;
            light.index_shadow = 0xFFFFu;
            matrixIndex += shadowTypeInfo.MatrixCount;

            RequestEntityCullResults shadowCasters{};
            ShadowCascadeCreateInfo cascadeInfo{};

            if (view->light->type == LightType::Directional)
            {
                cascadeInfo.worldToLocal = worldToLocal;
                cascadeInfo.clipToWorld = clipToWorld;
                cascadeInfo.nearPlaneOffset = 1.0f;
                cascadeInfo.padding = renderView->znear;
                cascadeInfo.splitPlanes = cascadeZSplits.data();
                cascadeInfo.resolution = m_shadowmaps->GetResolution().x;
                cascadeInfo.count = ShadowCascadeCount;
                Math::GetShadowCascadeMatrices(cascadeInfo, matrices);
            }

            if (view->light->type == LightType::Spot)
            {
                *matrices = Math::GetPerspective(view->light->angle, 1.0f, 0.1f, view->light->radius) * worldToLocal;
            }

            if (castShadows && view->light->type == LightType::Directional)
            {
                // Regenerate cascades as the depth range might change based on culling. 
                shadowCasters = context->cullingProxy->CullCascades(shadowCasterMask, matrices, renderView->forwardPlane, cascadeZSplits.data(), ShadowCascadeCount);
                cascadeInfo.nearPlaneOffset = shadowCasters.outMinDepth;
                Math::GetShadowCascadeMatrices(cascadeInfo, matrices);
            }

            if (castShadows && view->light->type == LightType::Spot)
            {
                shadowCasters = context->cullingProxy->CullFrustum(shadowCasterMask, *matrices);
            }

            if (castShadows && view->light->type == LightType::Point)
            {
                shadowCasters = context->cullingProxy->CullCubeFaces(shadowCasterMask, view->bounds->worldAABB);
            }

            if (view->light->type == LightType::Directional)
            {
                const auto nearPlane = Math::GetNearPlane(*matrices);
                light.position = float3(nearPlane.xyz);
                light.radius = nearPlane.w;
            }

            if (shadowCasters.GetCount() > 0u)
            {
                light.index_shadow = shadowCount;
                shadowCount += shadowTypeInfo.TileCount;
                auto& batches = resources->shadowBatches;

                if (!batches.count || batches[batches.count - 1u].count >= shadowTypeInfo.MaxBatchSize || batches[batches.count - 1u].type != view->light->type)
                {
                    auto& newBatch = batches[batches.count++];
                    newBatch.batchGroup = context->batcher->BeginNewGroup();
                    newBatch.type = view->light->type;
                    newBatch.baseLightIndex = lightIndex;
                }

                const auto layerOffset = batches[batches.count - 1u].count * shadowTypeInfo.LayerStride;
                batches[batches.count - 1u].count++;

                for (auto casterIndex = 0u; casterIndex < shadowCasters.GetCount(); ++casterIndex)
                {
                    const auto& info = shadowCasters[casterIndex];
                    auto entity = context->entityDb->Query<EntityViewMeshStatic>(EGID(info.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));
                    auto transform = entity->transform;
                    auto mesh = entity->staticMesh->sharedMesh;
                    auto userdata = (lightIndex & 0xFFFFu) | ((layerOffset + info.clipId) << 16u);

                    for (auto& kv : entity->materials->materials)
                    {
                        if (kv->material->GetShaderShadow())
                        {
                            context->batcher->SubmitMeshStaticDraw(transform, kv->material->GetShaderShadow(), nullptr, mesh, kv->submesh, userdata, info.depth);
                        }
                    }
                }
            }

            packedLights[lightIndex] = PackLight(light);
        }

        // Empty last one for clustering
        packedLights[lightCount] = PackedLight();

        cmd->EndBufferWrite(m_lightsBuffer.get());

        if (matrixCount > 0)
        {
            cmd->EndBufferWrite(m_lightMatricesBuffer.get());
        }

        if (m_shadowmaps->GetLayers() < shadowCount + ShadowCascadeCount)
        {
            RHI::ValidateTexture(m_shadowmaps, 1u, shadowCount + ShadowCascadeCount);
        }

        auto hash = HashCache::Get();
        RHI::SetConstant<uint32_t>(hash->pk_LightCount, lightCount);
        RHI::SetBuffer(hash->pk_Lights, m_lightsBuffer.get());
        RHI::SetBuffer(hash->pk_LightMatrices, m_lightMatricesBuffer.get());
        RHI::SetTexture(hash->pk_ShadowmapAtlas, m_shadowmaps.get());
    }

    void PassLights::RenderShadows(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto renderView = context->views[0];
        auto resources = renderView->GetResources<ViewResources>();
        auto& batches = resources->shadowBatches;

        auto atlasIndex = 0u;

        uint32_t passKeywords[(uint32_t)LightType::TypeCount]
        {
            hash->PK_LIGHT_PASS_DIRECTIONAL,
            hash->PK_LIGHT_PASS_SPOT,
            hash->PK_LIGHT_PASS_POINT,
        };

        for (auto i = 0u; i < batches.count; ++i)
        {
            const auto& batch = batches[i];
            auto& shadow = m_shadowTypeData[(int)batch.type];
            auto tileCount = shadow.TileCount * batch.count;
            auto keyword = passKeywords[(uint32_t)batch.type];

            cmd->BeginDebugScope("ShadowBatch", PK_COLOR_RED);

            auto range0 = TextureViewRange(0, 0, 0, shadow.LayerStride * batch.count);
            auto range1 = TextureViewRange(0u, atlasIndex, 1u, tileCount);

            if (batch.type == LightType::Point)
            {
                auto targetDepth = RenderTargetBinding(m_depthTargetCube.get(), range0, LoadOp::Clear, StoreOp::Store, { PK_CLIPZ_FAR, 0u });
                auto targetDist = RenderTargetBinding(m_shadowTargetCube.get(), range0, LoadOp::Clear, StoreOp::Store, float4(PK_HALF_MAX) );
                cmd.SetRenderTarget({ targetDepth, targetDist }, true);
                context->batcher->RenderGroup(cmd, batch.batchGroup, nullptr, keyword);
                RHI::SetTexture(hash->pk_Texture, m_shadowTargetCube.get());
                RHI::SetImage(hash->pk_Image, m_shadowmaps.get(), range1);
                cmd.Dispatch(m_computeCopyCubeShadow, 0, { m_shadowmaps->GetResolution().xy, tileCount });
            }
            else
            {
                auto targetDepth = RenderTargetBinding(m_depthTarget2D.get(), range0, LoadOp::Clear, StoreOp::Store, { PK_CLIPZ_FAR, 0u });
                auto targetDist = RenderTargetBinding(m_shadowmaps.get(), range1, LoadOp::Clear, StoreOp::Store, float4(PK_HALF_MAX));
                cmd.SetRenderTarget({ targetDepth, targetDist }, true);
                context->batcher->RenderGroup(cmd, batch.batchGroup, nullptr, keyword);
            }

            cmd->EndDebugScope();

            atlasIndex += tileCount;
        }
    }

    void PassLights::RenderScreenSpaceShadows(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto renderView = context->views[0];
        auto resources = renderView->GetResources<ViewResources>();
        auto& batches = resources->shadowBatches;

        if (batches.count == 0u || batches[0].type != LightType::Directional)
        {
            return;
        }

        auto hash = HashCache::Get();
        auto resolution = renderView->GetResolution();
        auto quarterResolution = uint3(resolution.x >> 1u, resolution.y >> 1u, 1u);

        cmd->BeginDebugScope("ScreenSpaceShadows", PK_COLOR_RED);

        {
            TextureDescriptor screenSpaceDesc;
            screenSpaceDesc.type = TextureType::Texture2D;
            screenSpaceDesc.format = TextureFormat::R8;
            screenSpaceDesc.usage = TextureUsage::Sample | TextureUsage::Storage;
            screenSpaceDesc.layers = 1;
            screenSpaceDesc.resolution = resolution;
            screenSpaceDesc.sampler.wrap[0] = WrapMode::Clamp;
            screenSpaceDesc.sampler.wrap[1] = WrapMode::Clamp;
            screenSpaceDesc.sampler.wrap[2] = WrapMode::Clamp;
            screenSpaceDesc.sampler.filterMin = FilterMode::Bilinear;
            screenSpaceDesc.sampler.filterMag = FilterMode::Bilinear;
            RHI::ValidateTexture(resources->screenSpaceShadowmap, screenSpaceDesc, "Lights.Shadowmap.ScreenSpace");

            screenSpaceDesc.resolution = quarterResolution;
            RHI::ValidateTexture(resources->screenSpaceShadowmapDownsampled, screenSpaceDesc, "Lights.Shadowmap.ScreenSpaceQuareterRes");
        }

        RHI::SetTexture(hash->pk_ShadowmapScreenSpace, resources->screenSpaceShadowmap.get());

        RHI::SetImage(hash->pk_Image, resources->screenSpaceShadowmapDownsampled.get());
        cmd.Dispatch(m_computeScreenSpaceShadow, 0, quarterResolution);

        RHI::SetTexture(hash->pk_Texture, resources->screenSpaceShadowmapDownsampled.get());
        RHI::SetImage(hash->pk_Image, resources->screenSpaceShadowmap.get());
        cmd.Dispatch(m_computeScreenSpaceShadow, 1, resolution);

        // Bend screen space shadows.
        // https://www.bendstudio.com/blog/inside-bend-screen-space-shadows/
        auto lightView = resources->lightViews[batches[0].baseLightIndex];
        auto lightDirection = lightView->transform->rotation * PK_FLOAT3_FORWARD;
        auto lightProjection = renderView->worldToClip * float4(-lightDirection, 0.0f);
        int viewMin[2] = { 0, 0 };
        int viewMax[2] = { (int)resolution.x, (int)resolution.y };
        float projection[4] = { lightProjection.x, -lightProjection.y, lightProjection.z, lightProjection.w };
        auto dispatchList = Bend::BuildDispatchList(projection, viewMax, viewMin, viewMax, false, 64);

        for (auto i = 0; i < dispatchList.DispatchCount; ++i)
        {
            const auto& dispatch = dispatchList.Dispatch[i];
            Bend::DispatchDataGPU dispatchData;
            memcpy(dispatchData.LightCoordinate_Shader, dispatchList.LightCoordinate_Shader, sizeof(dispatchList.LightCoordinate_Shader));
            memcpy(dispatchData.WaveOffset_Shader, dispatch.WaveOffset_Shader, sizeof(dispatch.WaveOffset_Shader));
            RHI::SetConstant<Bend::DispatchDataGPU>(hash->pk_BendShadowDispatchData, dispatchData);

            uint3 dim;
            dim.x = 64 * dispatch.WaveCount[0];
            dim.y = 1 * dispatch.WaveCount[1];
            dim.z = 1 * dispatch.WaveCount[2];
            cmd.Dispatch(m_computeScreenSpaceShadow, 2, dim);
        }

        cmd->EndDebugScope();
    }

    void PassLights::ComputeClusters(CommandBufferExt cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto renderView = context->views[0];
        auto resources = renderView->GetResources<ViewResources>();
        auto resolution = renderView->GetResolution();
        resolution.x /= (resolution.x + LightGridTileSizePx - 1) / LightGridTileSizePx;
        resolution.y /= (resolution.y + LightGridTileSizePx - 1) / LightGridTileSizePx;
        resolution.z = LightGridSizeZ;

        auto lightIndexCount = resolution.x *
            resolution.y *
            resolution.z *
            MaxLightsPerTile;

        TextureDescriptor imageDescriptor;
        imageDescriptor.type = TextureType::Texture3D;
        imageDescriptor.format = TextureFormat::R32UI;
        imageDescriptor.usage = TextureUsage::Storage | TextureUsage::Concurrent;
        imageDescriptor.resolution = resolution;
        imageDescriptor.sampler.filterMin = FilterMode::Point;
        imageDescriptor.sampler.filterMag = FilterMode::Point;
        imageDescriptor.sampler.wrap[0] = WrapMode::Clamp;
        imageDescriptor.sampler.wrap[1] = WrapMode::Clamp;
        imageDescriptor.sampler.wrap[2] = WrapMode::Clamp;

        if (RHI::ValidateTexture(resources->lightTiles, imageDescriptor, "Lights.Tiles"))
        {
            RHI::SetImage(hash->pk_LightTiles, resources->lightTiles.get());
        }

        if (RHI::ValidateBuffer<ushort>(resources->lightsLists, lightIndexCount, BufferUsage::DefaultStorage, "Lights.List"))
        {
            RHI::SetBuffer(hash->pk_LightLists, resources->lightsLists.get());
        }

        cmd.DispatchWithCounter(m_computeLightAssignment, resolution);
    }
}
