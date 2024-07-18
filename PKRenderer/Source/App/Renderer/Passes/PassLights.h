#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/MemoryBlock.h"
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

    typedef std::array<float, 5> ShadowCascades;

    class PassLights : public NoCopy
    {
        public:
            constexpr static const uint32_t MaxLightsPerTile = 32;
            constexpr static const uint32_t LightGridSizeZ = 32;
            constexpr static const uint32_t LightGridTileSizePx = 64;
            constexpr static const uint32_t ShadowCascadeCount = 4;

            PassLights(AssetDatabase* assetDatabase, const uint2& initialResolution);
            void RenderShadows(CommandBufferExt cmd, RenderPipelineContext* context);
            void RenderScreenSpaceShadows(CommandBufferExt cmd, RenderPipelineContext* context);
            void ComputeClusters(CommandBufferExt cmd, RenderPipelineContext* context);
            void BuildLights(RenderPipelineContext* context);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
            inline float4 GetCascadeZSplitsFloat4(float znear, float zfar) const { return Math::GetCascadeDepthsFloat4(znear, zfar, m_cascadeLinearity, LightGridSizeZ); }
        
        private:
            struct ShadowTypeData
            {
                uint32_t TileCount = 0u;
                uint32_t MatrixCount = 0u;
                uint32_t MaxBatchSize = 0u;
                uint32_t LayerStride = 0u;
            };

            struct ShadowbatchInfo
            {
                uint32_t baseLightIndex = 0u;
                uint32_t count = 0u;
                uint32_t batchGroup = 0u;
                LightType type = LightType::TypeCount;
            };

            uint32_t BuildShadowBatch(RenderPipelineContext* context,
                                      const RequestEntityCullResults& shadowCasters,
                                      EntityViewLight* lightView, 
                                      uint32_t index, 
                                      uint32_t* outShadowCount);

            ShaderAsset* m_computeLightAssignment = nullptr;
            ShaderAsset* m_computeCopyCubeShadow = nullptr;
            ShaderAsset* m_computeScreenSpaceShadow = nullptr;
            RHIBufferRef m_lightsBuffer;
            RHIBufferRef m_lightMatricesBuffer;
            RHIBufferRef m_lightsLists;
            RHITextureRef m_lightTiles;
            RHITextureRef m_shadowmaps;
            RHITextureRef m_screenSpaceShadowmapDownsampled;
            RHITextureRef m_screenSpaceShadowmap;
            RHITextureRef m_depthTarget2D;
            RHITextureRef m_depthTargetCube;
            RHITextureRef m_shadowTargetCube;
            
            std::vector<ShadowbatchInfo> m_shadowBatches;
            MemoryBlock<EntityViewLight*> m_lights;
            
            ShadowTypeData m_shadowTypeData[(int)LightType::TypeCount];
            
            CVariableField<float> m_cascadeLinearity = { "Renderer.Lights.CascadeLinearity", 0.5f };
            CVariableField<uint32_t> m_shadowmapSize = { "Renderer.Lights.ShadowmapSize", 1024u };
    };
}