#include "PrecompiledHeader.h"
#include <bend/bend_sss_cpu.h>
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/ApplicationConfig.h"
#include "ECS/EntityDatabase.h"
#include "ECS/EntityViewStaticMesh.h"
#include "ECS/EntityViewLight.h"
#include "Rendering/HashCache.h"
#include "Rendering/Objects/RenderView.h"
#include "Rendering/Geometry/IBatcher.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/EntityCulling.h"
#include "Rendering/IRenderPipeline.h"
#include "PassLights.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::ECS;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Geometry;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    // Packed into float4, float4, uint4
    struct alignas(16) LightPacked
    {
        Math::float3 position = Math::PK_FLOAT3_ZERO;
        float radius = 0.0f;
        Math::float3 color = Math::PK_FLOAT3_ZERO;
        float angle = 0.0f;
        Math::ushort indexShadow = 0xFFFFu;
        Math::ushort indexMatrix = 0u;
        Math::ushort type = 0xFFFFu;
        Math::ushort cookie = 0xFFFFu;
        Math::uint direction = 0u;
        float sourceRadius = 0.0f;
    };

    static int EntityViewLightPtrCompare(const void* a, const void* b)
    {
        auto lightA = *reinterpret_cast<EntityViewLight* const*>(a);
        auto lightB = *reinterpret_cast<EntityViewLight* const*>(b);

        auto shadowsA = (lightA->primitive->flags & ScenePrimitiveFlags::CastShadows) != 0;
        auto shadowsB = (lightB->primitive->flags & ScenePrimitiveFlags::CastShadows) != 0;

        if (shadowsA < shadowsB)
        {
            return 1;
        }

        if (shadowsA > shadowsB)
        {
            return -1;
        }

        if (lightA->light->type < lightB->light->type)
        {
            return -1;
        }

        if (lightA->light->type > lightB->light->type)
        {
            return 1;
        }

        return 0;
    }

    PassLights::PassLights(AssetDatabase* assetDatabase, const ApplicationConfig* config) : m_lights(1024)
    {
        PK_LOG_VERBOSE("PassLights.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeLightAssignment = assetDatabase->Find<Shader>("CS_LightAssignment");
        m_computeCopyCubeShadow = assetDatabase->Find<Shader>("CS_CopyCubeShadow");
        m_computeScreenSpaceShadow = assetDatabase->Find<Shader>("CS_ScreenspaceShadow");

        m_cascadeLinearity = config->CascadeLinearity;

        m_shadowTypeData[(int)LightType::Point].MatrixCount = 0u;
        m_shadowTypeData[(int)LightType::Point].TileCount = 1u;
        m_shadowTypeData[(int)LightType::Point].MaxBatchSize = PK_SHADOW_CASCADE_COUNT;
        m_shadowTypeData[(int)LightType::Point].LayerStride = 6u;

        m_shadowTypeData[(int)LightType::Spot].MatrixCount = 1u;
        m_shadowTypeData[(int)LightType::Spot].TileCount = 1u;
        m_shadowTypeData[(int)LightType::Spot].MaxBatchSize = PK_SHADOW_CASCADE_COUNT;
        m_shadowTypeData[(int)LightType::Spot].LayerStride = 1u;

        m_shadowTypeData[(int)LightType::Directional].MatrixCount = PK_SHADOW_CASCADE_COUNT;
        m_shadowTypeData[(int)LightType::Directional].TileCount = PK_SHADOW_CASCADE_COUNT;
        m_shadowTypeData[(int)LightType::Directional].MaxBatchSize = 1u;
        m_shadowTypeData[(int)LightType::Directional].LayerStride = PK_SHADOW_CASCADE_COUNT;

        auto shadowCubeFaceSize = (uint)sqrt((config->ShadowmapTileSize * config->ShadowmapTileSize) / 6);
        TextureDescriptor depthDesc;
        depthDesc.samplerType = SamplerType::CubemapArray;
        depthDesc.resolution = { shadowCubeFaceSize , shadowCubeFaceSize , 1u };
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
        depthDesc.resolution = { (int)config->ShadowmapTileSize, (int)config->ShadowmapTileSize, 1u };
        depthDesc.layers = PK_SHADOW_CASCADE_COUNT;
        depthDesc.usage = TextureUsage::RTDepth;
        m_depthTarget2D = Texture::Create(depthDesc, "Lights.DepthTarget.2D");

        TextureDescriptor atlasDesc;
        atlasDesc.samplerType = SamplerType::Sampler2DArray;
        atlasDesc.format = TextureFormat::R32F;
        atlasDesc.usage = TextureUsage::Sample | TextureUsage::Storage | TextureUsage::RTColor;
        atlasDesc.layers = PK_SHADOW_CASCADE_COUNT * 2; // initial size assume 1 active directional light.
        atlasDesc.resolution = { (int)config->ShadowmapTileSize, (int)config->ShadowmapTileSize, 1u };
        atlasDesc.sampler.wrap[0] = WrapMode::Clamp;
        atlasDesc.sampler.wrap[1] = WrapMode::Clamp;
        atlasDesc.sampler.wrap[2] = WrapMode::Clamp;
        atlasDesc.sampler.filterMin = FilterMode::Bilinear;
        atlasDesc.sampler.filterMag = FilterMode::Bilinear;
        m_shadowmaps = Texture::Create(atlasDesc, "Lights.Shadowmap.Atlas");

        TextureDescriptor screenSpaceDesc;
        screenSpaceDesc.samplerType = SamplerType::Sampler2D;
        screenSpaceDesc.format = TextureFormat::R8;
        screenSpaceDesc.usage = TextureUsage::Sample | TextureUsage::Storage;
        screenSpaceDesc.layers = 1;
        screenSpaceDesc.resolution = { config->InitialWidth, config->InitialHeight, 1u };
        screenSpaceDesc.sampler.wrap[0] = WrapMode::Clamp;
        screenSpaceDesc.sampler.wrap[1] = WrapMode::Clamp;
        screenSpaceDesc.sampler.wrap[2] = WrapMode::Clamp;
        screenSpaceDesc.sampler.filterMin = FilterMode::Bilinear;
        screenSpaceDesc.sampler.filterMag = FilterMode::Bilinear;
        m_screenSpaceShadowmap = Texture::Create(screenSpaceDesc, "Lights.Shadowmap.ScreenSpace");

        screenSpaceDesc.resolution = { config->InitialWidth >> 1u, config->InitialHeight >> 1u, 1u };
        m_screenSpaceShadowmapDownsampled = Texture::Create(screenSpaceDesc, "Lights.Shadowmap.ScreenSpaceQuareterRes");

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

        auto lightIndexCount = imageDescriptor.resolution.x *
            imageDescriptor.resolution.y *
            imageDescriptor.resolution.z *
            MaxLightsPerTile;

        m_lightsBuffer = Buffer::Create<LightPacked>(1024ull, BufferUsage::PersistentStorage, "Lights");
        m_lightMatricesBuffer = Buffer::Create<float4x4>(32ull, BufferUsage::PersistentStorage, "Lights.Matrices");
        m_lightsLists = Buffer::Create<ushort>(lightIndexCount, BufferUsage::DefaultStorage, "Lights.List");

        auto hash = HashCache::Get();

        GraphicsAPI::SetBuffer(hash->pk_LightLists, m_lightsLists.get());
        GraphicsAPI::SetImage(hash->pk_LightTiles, m_lightTiles.get());

        auto lightCookies = assetDatabase->Load<Texture>("res/textures/default/T_LightCookies.ktx2");

        auto sampler = lightCookies->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Clamp;
        sampler.wrap[1] = WrapMode::Clamp;
        sampler.wrap[2] = WrapMode::Clamp;
        lightCookies->SetSampler(sampler);
        GraphicsAPI::SetTexture(hash->pk_LightCookies, lightCookies);
    }

    void PassLights::RenderShadows(CommandBuffer* cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto atlasIndex = 0u;

        uint32_t passKeywords[(uint32_t)LightType::TypeCount]
        {
            hash->PK_LIGHT_PASS_DIRECTIONAL,
            hash->PK_LIGHT_PASS_SPOT,
            hash->PK_LIGHT_PASS_POINT,
        };

        for (const auto& batch : m_shadowBatches)
        {
            auto& shadow = m_shadowTypeData[(int)batch.type];
            auto tileCount = shadow.TileCount * batch.count;
            auto keyword = passKeywords[(uint32_t)batch.type];

            cmd->BeginDebugScope("ShadowBatch", PK_COLOR_RED);

            auto range0 = TextureViewRange(0, 0, 0, shadow.LayerStride * batch.count);
            auto range1 = TextureViewRange(0u, atlasIndex, 1u, tileCount);

            if (batch.type == LightType::Point)
            {
                cmd->SetRenderTarget({ m_depthTargetCube.get(), m_shadowTargetCube.get() }, { range0, range0 }, true);
                cmd->ClearDepth(0.0f, 0u);
                cmd->ClearColor(color(PK_HALF_MAX), 0u);
                context->batcher->RenderGroup(cmd, batch.batchGroup, nullptr, keyword);
                GraphicsAPI::SetTexture(hash->pk_Texture, m_shadowTargetCube.get());
                GraphicsAPI::SetImage(hash->pk_Image, m_shadowmaps.get(), range1);
                cmd->DispatchWithCounter(m_computeCopyCubeShadow, 0, { m_shadowmaps->GetResolution().xy, tileCount });
            }
            else
            {
                cmd->SetRenderTarget({ m_depthTarget2D.get(), m_shadowmaps.get() }, { range0, range1 }, true);
                cmd->ClearColor(color(PK_HALF_MAX), 0u);
                cmd->ClearDepth(0.0f, 0u);
                context->batcher->RenderGroup(cmd, batch.batchGroup, nullptr, keyword);
            }

            cmd->EndDebugScope();

            atlasIndex += tileCount;
        }
    }

    void PassLights::RenderScreenSpaceShadows(CommandBuffer* cmd, RenderPipelineContext* context)
    {
        if (m_shadowBatches.size() == 0u || m_shadowBatches.at(0).type != LightType::Directional)
        {
            return;
        }

        cmd->BeginDebugScope("ScreenSpaceShadows", PK_COLOR_RED);

        auto renderView = context->views[0];
        auto hash = HashCache::Get();
        auto resolution = renderView->GetResolution();
        auto quarterResolution = uint3(resolution.x >> 1u, resolution.y >> 1u, 1u);

        m_screenSpaceShadowmap->Validate(resolution);
        m_screenSpaceShadowmapDownsampled->Validate(quarterResolution);

        GraphicsAPI::SetTexture(hash->pk_ShadowmapScreenSpace, m_screenSpaceShadowmap.get());

        GraphicsAPI::SetImage(hash->pk_Image, m_screenSpaceShadowmapDownsampled.get());
        cmd->Dispatch(m_computeScreenSpaceShadow, 0, quarterResolution);

        GraphicsAPI::SetTexture(hash->pk_Texture, m_screenSpaceShadowmapDownsampled.get());
        GraphicsAPI::SetImage(hash->pk_Image, m_screenSpaceShadowmap.get());
        cmd->Dispatch(m_computeScreenSpaceShadow, 1, resolution);

        // Bend screen space shadows.
        // https://www.bendstudio.com/blog/inside-bend-screen-space-shadows/
        auto lightView = m_lights[m_shadowBatches.at(0).baseLightIndex];
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
            GraphicsAPI::SetConstant<Bend::DispatchDataGPU>(hash->pk_BendShadowDispatchData, dispatchData);

            uint3 dim;
            dim.x = 64 * dispatch.WaveCount[0];
            dim.y = 1 * dispatch.WaveCount[1];
            dim.z = 1 * dispatch.WaveCount[2];
            cmd->Dispatch(m_computeScreenSpaceShadow, 2, dim);
        }

        cmd->EndDebugScope();
    }

    void PassLights::ComputeClusters(CommandBuffer* cmd, RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto renderView = context->views[0];
        auto resolution = renderView->GetResolution();
        resolution.x /= LightGridTileSizePx;
        resolution.y /= LightGridTileSizePx;
        resolution.z = LightGridSizeZ;

        auto lightIndexCount = resolution.x *
            resolution.y *
            resolution.z *
            MaxLightsPerTile;

        if (m_lightsLists->Validate<ushort>(lightIndexCount))
        {
            GraphicsAPI::SetBuffer(hash->pk_LightLists, m_lightsLists.get());
        }

        if (m_lightTiles->Validate(resolution))
        {
            GraphicsAPI::SetImage(hash->pk_LightTiles, m_lightTiles.get());
        }

        cmd->DispatchWithCounter(m_computeLightAssignment, resolution);
    }

    void PassLights::BuildLights(RenderPipelineContext* context)
    {
        auto renderView = context->views[0];
        auto culledLights = context->cullingProxy->CullFrustum(ScenePrimitiveFlags::Light, renderView->worldToClip);

        if (culledLights.GetCount() == 0)
        {
            return;
        }

        uint lightCount = (uint)culledLights.GetCount();
        uint matrixCount = 0u;
        uint matrixIndex = 0u;
        uint shadowCount = 0u;

        for (auto i = 0U; i < culledLights.GetCount(); ++i)
        {
            auto view = context->entityDb->Query<EntityViewLight>(EGID(culledLights[i].entityId, (uint)ENTITY_GROUPS::ACTIVE));
            matrixCount += m_shadowTypeData[(int)view->light->type].MatrixCount;
            m_lights[i] = view;
        }

        // Sort could be faster but whatever.
        qsort(m_lights.GetData(), lightCount, sizeof(EntityViewLight*), EntityViewLightPtrCompare);

        m_shadowBatches.clear();
        m_lightsBuffer->Validate<LightPacked>(lightCount + 1);
        m_lightMatricesBuffer->Validate<float4x4>(matrixCount);

        auto cmd = GraphicsAPI::GetCommandBuffer(QueueType::Transfer);
        auto lightsView = cmd->BeginBufferWrite<LightPacked>(m_lightsBuffer.get(), 0u, lightCount + 1);
        auto matricesView = matrixCount > 0 ? cmd->BeginBufferWrite<float4x4>(m_lightMatricesBuffer.get(), 0u, matrixCount) : BufferView<float4x4>();

        auto clipToWorld = glm::inverse(renderView->worldToClip);
        auto cascadeZSplits = GetCascadeZSplits(renderView->znear, renderView->zfar);
        auto shadowCasterMask = ScenePrimitiveFlags::Mesh | ScenePrimitiveFlags::CastShadows;

        for (auto i = 0u; i < lightCount; ++i)
        {
            auto& view = m_lights[i];
            auto& light = lightsView[i];
            auto& transform = view->transform;
            auto& worldToLocal = transform->worldToLocal;
            light.indexShadow = 0xFFFFu;
            light.indexMatrix = matrixIndex;
            light.color = view->light->color;
            light.cookie = (ushort)view->light->cookie;
            light.type = (ushort)view->light->type;
            light.position = transform->position;
            light.radius = view->light->radius;
            light.angle = view->light->angle * PK_FLOAT_DEG2RAD;
            light.sourceRadius = view->light->sourceRadius;
            light.direction = 0u;

            auto castShadows = (view->primitive->flags & ScenePrimitiveFlags::CastShadows) != 0;

            switch (view->light->type)
            {
                case LightType::Directional:
                {
                    ShadowCascadeCreateInfo cascadeInfo{};
                    cascadeInfo.worldToLocal = worldToLocal;
                    cascadeInfo.clipToWorld = clipToWorld;
                    cascadeInfo.nearPlaneOffset = 1.0f;
                    cascadeInfo.splitPlanes = cascadeZSplits.data();
                    cascadeInfo.resolution = m_shadowmaps->GetResolution().x;
                    cascadeInfo.count = PK_SHADOW_CASCADE_COUNT;
                    Functions::GetShadowCascadeMatrices(cascadeInfo, matricesView.data + matrixIndex);

                    // @TODO Something is very wrong here. position radius output stays invariant & lighting flickers when nothing is rendered.
                    // Fix this shit.
                    // Only shadow casting lights need matrices.
                    if (castShadows)
                    {
                        auto shadowCasters = context->cullingProxy->CullCascades(shadowCasterMask, matricesView.data + matrixIndex, renderView->forwardPlane, cascadeZSplits.data(), PK_SHADOW_CASCADE_COUNT);
                        light.indexShadow = BuildShadowBatch(context, shadowCasters, view, i, &shadowCount);

                        // Regenerate cascades as the depth range might change based on culling. 
                        cascadeInfo.nearPlaneOffset = shadowCasters.outMinDepth;
                        Functions::GetShadowCascadeMatrices(cascadeInfo, matricesView.data + matrixIndex);
                    }

                    const auto nearPlane = Functions::GetNearPlane(matricesView.data[matrixIndex]);
                    light.position = float3(nearPlane.xyz);
                    light.radius = nearPlane.w;
                }
                break;

                case LightType::Spot:
                {
                    matricesView[matrixIndex] = Functions::GetPerspective(view->light->angle, 1.0f, 0.1f, view->light->radius) * worldToLocal;
                    light.direction = Math::Functions::OctaEncodeUint(transform->rotation * PK_FLOAT3_FORWARD);

                    if (castShadows)
                    {
                        auto shadowCasters = context->cullingProxy->CullFrustum(shadowCasterMask, matricesView[matrixIndex]);
                        light.indexShadow = BuildShadowBatch(context, shadowCasters, view, i, &shadowCount);
                    }
                }
                break;

                case LightType::Point:
                {
                    if (castShadows)
                    {
                        auto shadowCasters = context->cullingProxy->CullCubeFaces(shadowCasterMask, view->bounds->worldAABB);
                        light.indexShadow = BuildShadowBatch(context, shadowCasters, view, i, &shadowCount);
                    }
                }
                break;
            }

            matrixIndex += m_shadowTypeData[(uint32_t)view->light->type].MatrixCount;
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

    ShadowCascades PassLights::GetCascadeZSplits(float znear, float zfar) const
    {
        ShadowCascades cascadeSplits;
        Functions::GetCascadeDepths(znear, zfar, m_cascadeLinearity, cascadeSplits.data(), LightGridSizeZ, 5);
        return cascadeSplits;
    }

    uint32_t PassLights::BuildShadowBatch(RenderPipelineContext* context, const RequestEntityCullResults& shadowCasters, EntityViewLight* lightView, uint32_t index, uint32_t* outShadowCount)
    {
        if (shadowCasters.GetCount() == 0)
        {
            return 0xFFFFu;
        }

        auto& shadow = m_shadowTypeData[(int)lightView->light->type];

        if (m_shadowBatches.size() == 0 ||
            m_shadowBatches.back().count >= shadow.MaxBatchSize ||
            m_shadowBatches.back().type != lightView->light->type)
        {
            auto& newBatch = m_shadowBatches.emplace_back();
            newBatch.batchGroup = context->batcher->BeginNewGroup();
            newBatch.type = lightView->light->type;
            newBatch.baseLightIndex = index;
        }

        auto& batch = m_shadowBatches.back();

        uint32_t shadowmapIndex = *outShadowCount;
        *outShadowCount += m_shadowTypeData[(int)lightView->light->type].TileCount;

        for (auto i = 0u; i < shadowCasters.GetCount(); ++i)
        {
            auto& info = shadowCasters[i];
            auto entity = context->entityDb->Query<EntityViewStaticMesh>(EGID(info.entityId, (uint32_t)ENTITY_GROUPS::ACTIVE));

            for (auto& kv : entity->materials->materials)
            {
                auto transform = entity->transform;
                auto shader = kv.material->GetShadowShader();

                if (shader != nullptr)
                {
                    auto layerOffset = batch.count * shadow.LayerStride + info.clipId;
                    context->batcher->SubmitStaticMeshDraw(transform, shader, nullptr, entity->staticMesh->sharedMesh, kv.submesh, (index & 0xFFFF) | (layerOffset << 16), info.depth);
                }
            }
        }

        batch.count++;
        return shadowmapIndex;
    }
}
