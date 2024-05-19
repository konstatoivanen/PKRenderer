#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NoCopy.h"
#include "Utilities/MemoryBlock.h"
#include "Math/FunctionsMisc.h"
#include "Graphics/GraphicsFwd.h"
#include "Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RequestEntityCullResults)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RenderPipelineContext)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, struct EntityViewLight)

namespace PK::Renderer::Passes
{
    typedef std::array<float, 5> ShadowCascades;

    class PassLights : public Utilities::NoCopy
    {
        public:
            const uint32_t MaxLightsPerTile = 32;
            const uint32_t LightGridSizeZ = 32;
            const uint32_t LightGridTileSizePx = 64;

            PassLights(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void RenderShadows(Graphics::CommandBufferExt cmd, RenderPipelineContext* context);
            void RenderScreenSpaceShadows(Graphics::CommandBufferExt cmd, RenderPipelineContext* context);
            void ComputeClusters(Graphics::CommandBufferExt cmd, RenderPipelineContext* context);
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

            Graphics::Shader* m_computeLightAssignment = nullptr;
            Graphics::Shader* m_computeCopyCubeShadow = nullptr;
            Graphics::Shader* m_computeScreenSpaceShadow = nullptr;
            Graphics::BufferRef m_lightsBuffer;
            Graphics::BufferRef m_lightMatricesBuffer;
            Graphics::BufferRef m_lightsLists;
            Graphics::TextureRef m_lightTiles;
            Graphics::TextureRef m_shadowmaps;
            Graphics::TextureRef m_screenSpaceShadowmapDownsampled;
            Graphics::TextureRef m_screenSpaceShadowmap;
            Graphics::TextureRef m_depthTarget2D;
            Graphics::TextureRef m_depthTargetCube;
            Graphics::TextureRef m_shadowTargetCube;
            
            std::vector<ShadowbatchInfo> m_shadowBatches;
            Utilities::MemoryBlock<ECS::EntityViewLight*> m_lights;
            
            float m_cascadeLinearity;
            ShadowTypeData m_shadowTypeData[(int)LightType::TypeCount];
    };
}