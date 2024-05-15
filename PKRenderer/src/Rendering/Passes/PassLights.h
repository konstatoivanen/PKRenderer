#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Utilities/MemoryBlock.h"
#include "Math/FunctionsMisc.h"
#include "Rendering/EntityEnums.h"
#include "Rendering/RHI/RHI.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering, struct RequestEntityCullResults)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering, struct RenderPipelineContext)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, struct EntityViewLight)

namespace PK::Rendering::Passes
{
    typedef std::array<float, 5> ShadowCascades;

    class PassLights : public Utilities::NoCopy
    {
        public:
            const uint32_t MaxLightsPerTile = 32;
            const uint32_t LightGridSizeZ = 32;
            const uint32_t LightGridTileSizePx = 64;

            PassLights(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void RenderShadows(RHI::Objects::CommandBuffer* cmd, RenderPipelineContext* context);
            void RenderScreenSpaceShadows(RHI::Objects::CommandBuffer* cmd, RenderPipelineContext* context);
            void ComputeClusters(RHI::Objects::CommandBuffer* cmd, RenderPipelineContext* context);
            void BuildLights(RenderPipelineContext* context);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
            inline Math::float4 GetCascadeZSplitsFloat4(float znear, float zfar) const { return Math::Functions::GetCascadeDepthsFloat4(znear, zfar, m_cascadeLinearity, LightGridSizeZ); }
        
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
                                      ECS::EntityViewLight* lightView, 
                                      uint32_t index, 
                                      uint32_t* outShadowCount);

            RHI::Objects::Shader* m_computeLightAssignment = nullptr;
            RHI::Objects::Shader* m_computeCopyCubeShadow = nullptr;
            RHI::Objects::Shader* m_computeScreenSpaceShadow = nullptr;
            RHI::Objects::BufferRef m_lightsBuffer;
            RHI::Objects::BufferRef m_lightMatricesBuffer;
            RHI::Objects::BufferRef m_lightsLists;
            RHI::Objects::TextureRef m_lightTiles;
            RHI::Objects::TextureRef m_shadowmaps;
            RHI::Objects::TextureRef m_screenSpaceShadowmapDownsampled;
            RHI::Objects::TextureRef m_screenSpaceShadowmap;
            RHI::Objects::TextureRef m_depthTarget2D;
            RHI::Objects::TextureRef m_depthTargetCube;
            RHI::Objects::TextureRef m_shadowTargetCube;
            
            std::vector<ShadowbatchInfo> m_shadowBatches;
            Utilities::MemoryBlock<ECS::EntityViewLight*> m_lights;
            
            float m_cascadeLinearity;
            ShadowTypeData m_shadowTypeData[(int)LightType::TypeCount];
    };
}