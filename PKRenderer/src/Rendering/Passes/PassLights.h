#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Tokens/CullingTokens.h"
#include "ECS/EntityViews/LightRenderableView.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Services/Batcher.h"

namespace PK::Rendering::Passes
{
    typedef std::array<float, 5> ShadowCascades;

    struct ShadowmapLightTypeData
    {
        uint32_t TileCount = 0u;
        uint32_t MaxBatchSize = 0u;
        uint32_t LayerStride = 0u;
    };

    struct ShadowbatchInfo
    {
        uint32_t firstIndex = 0u;
        uint32_t count = 0u;
        uint32_t batchGroup = 0u;
        Structs::LightType batchType = Structs::LightType::TypeCount;
        float maxDepthRange = 0.0f;
    };

    class PassLights : public Utilities::NoCopy
    {
        public:
            const uint32_t MaxLightsPerTile = 32;
            const uint32_t LightGridSizeZ = 32;
            const uint32_t LightGridTileSizePx = 64;

            PassLights(Core::Services::AssetDatabase* assetDatabase, 
                       ECS::EntityDatabase* entityDb, 
                       Core::Services::Sequencer* sequencer, 
                       Batcher* batcher, 
                       const Core::ApplicationConfig* config);
            void Cull(void* engineRoot, ECS::Tokens::VisibilityList* visibilityList, const Math::float4x4& viewProjection, float znear, float zfar);
            void RenderShadows(Objects::CommandBuffer* cmd);
            void ComputeClusters(Objects::CommandBuffer* cmd, Math::uint3 resolution);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
        
        private:
            uint32_t BuildShadowBatch(ECS::Tokens::VisibilityList* visibilityList, 
                                        ECS::EntityViews::LightRenderableView* view, 
                                        uint32_t index, 
                                        float maxDepth,
                                        uint32_t* outShadowCount,
                                        float* outMinDepth = nullptr);

            ECS::EntityDatabase* m_entityDb = nullptr;
            Core::Services::Sequencer* m_sequencer = nullptr;
            Batcher* m_batcher = nullptr;
            Objects::Shader* m_computeLightAssignment = nullptr;
            Objects::Shader* m_computeCopyCubeShadow  = nullptr;
            float m_cascadeLinearity;
            uint32_t m_shadowmapCubeFaceSize;
            uint32_t m_shadowmapTileSize;
            ShadowmapLightTypeData m_shadowmapTypeData[(int)Structs::LightType::TypeCount];
            std::vector<ShadowbatchInfo> m_shadowBatches;
            Utilities::MemoryBlock<ECS::EntityViews::LightRenderableView*> m_lights;
            Utilities::Ref<Objects::Buffer> m_lightsBuffer;
            Utilities::Ref<Objects::Buffer> m_lightMatricesBuffer;
            Utilities::Ref<Objects::Buffer> m_lightsLists;
            Utilities::Ref<Objects::Texture> m_lightTiles;
            Utilities::Ref<Objects::Texture> m_shadowmaps;
            Utilities::Ref<Objects::Texture> m_depthTarget2D;
            Utilities::Ref<Objects::Texture> m_depthTargetCube;
            Utilities::Ref<Objects::Texture> m_shadowTargetCube;
    };
}