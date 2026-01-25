#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FastBuffer.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/CLI/CVariable.h"
#include "App/Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    struct EntityViewLight;
    struct RequestEntityCullResults;
    struct RenderPipelineContext;
    struct RenderView;

    typedef std::array<float, 5> ShadowCascades;

    class PassLights : public NoCopy
    {
        public:
            constexpr static const uint32_t MaxLightsPerTile = 32;
            constexpr static const uint32_t LightGridSizeZ = 32;
            constexpr static const uint32_t LightGridTileSizePx = 64;
            constexpr static const uint32_t ShadowCascadeCount = 4;
 
           struct ShadowbatchInfo
            {
                uint32_t baseLightIndex = 0u;
                uint32_t count = 0u;
                uint32_t batchGroup = 0u;
                LightType type = LightType::TypeCount;
            };

            struct ViewResources
            {
                BufferView<EntityViewLight*> lightViews;
                BufferView<ShadowbatchInfo> shadowBatches;
                RHIBufferRef  lightsLists;
                RHITextureRef lightTiles;
                RHITextureRef screenSpaceShadowmapDownsampled;
                RHITextureRef screenSpaceShadowmap;
            };

            PassLights(AssetDatabase* assetDatabase);
            void SetViewConstants(RenderView* view);
            void BuildLights(RenderPipelineContext* context);
            void RenderShadows(CommandBufferExt cmd, RenderPipelineContext* context);
            void RenderScreenSpaceShadows(CommandBufferExt cmd, RenderPipelineContext* context);
            void ComputeClusters(CommandBufferExt cmd, RenderPipelineContext* context);

        private:
            ShaderAsset* m_computeLightAssignment = nullptr;
            ShaderAsset* m_computeCopyCubeShadow = nullptr;
            ShaderAsset* m_computeScreenSpaceShadow = nullptr;
            RHIBufferRef m_lightsBuffer;
            RHIBufferRef m_lightMatricesBuffer;
            RHITextureRef m_shadowmaps;
            RHITextureRef m_depthTarget2D;
            RHITextureRef m_depthTargetCube;
            RHITextureRef m_shadowTargetCube;
            
            CVariableField<float> m_cascadeDistribution = { "Renderer.Lights.CascadeDistribution", 0.5f };
            CVariableField<float> m_tileZDistribution = { "Renderer.Lights.TileZDistribution", 10.0f };
            CVariableField<uint32_t> m_shadowmapSize = { "Renderer.Lights.ShadowmapSize", 1024u };
    };
}
