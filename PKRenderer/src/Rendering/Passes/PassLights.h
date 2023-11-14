#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Tokens/CullingTokens.h"
#include "ECS/EntityViews/LightRenderableView.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/StaticDrawBatcher.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    typedef std::array<float, 5> ShadowCascades;

    class PassLights : public Utilities::NoCopy
    {
        public:
            const uint32_t MaxLightsPerTile = 32;
            const uint32_t LightGridSizeZ = 32;
            const uint32_t LightGridTileSizePx = 64;

            PassLights(Core::Services::AssetDatabase* assetDatabase, 
                       ECS::EntityDatabase* entityDb, 
                       Core::Services::Sequencer* sequencer, 
                       StaticDrawBatcher* batcher,
                       const Core::ApplicationConfig* config);

            void RenderShadows(RHI::Objects::CommandBuffer* cmd);
            void RenderScreenSpaceShadows(RHI::Objects::CommandBuffer* cmd, const Math::float4x4& worldToClip, const Math::uint3& resolution);
            void ComputeClusters(RHI::Objects::CommandBuffer* cmd, Math::uint3 resolution);
            void Cull(void* engineRoot, ECS::Tokens::VisibilityList* visibilityList, const Math::float4x4& worldToClip, float znear, float zfar);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
        
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
                Structs::LightType type = Structs::LightType::TypeCount;
                float maxDepthRange = 0.0f;
            };

            uint32_t BuildShadowBatch(ECS::Tokens::VisibilityList* visibilityList, 
                                        ECS::EntityViews::LightRenderableView* view, 
                                        uint32_t index, 
                                        float maxDepth,
                                        uint32_t* outShadowCount,
                                        float* outMinDepth = nullptr);

            ECS::EntityDatabase* m_entityDb = nullptr;
            Core::Services::Sequencer* m_sequencer = nullptr;
            StaticDrawBatcher* m_batcher = nullptr;

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
            Utilities::MemoryBlock<ECS::EntityViews::LightRenderableView*> m_lights;
            
            float m_directionalHack;

            float m_cascadeLinearity;
            ShadowTypeData m_shadowTypeData[(int)Structs::LightType::TypeCount];
    };
}