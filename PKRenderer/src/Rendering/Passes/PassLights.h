#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "ECS/Contextual/Tokens/CullingTokens.h"
#include "ECS/Contextual/EntityViews/LightRenderableView.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Batcher.h"

namespace PK::Rendering::Passes
{
    typedef struct ShadowCascades { float planes[5]; } ShadowCascades;

    struct ShadowmapLightTypeData
    {
        Utilities::Ref<Objects::RenderTexture> SceneRenderTarget = nullptr;
        uint32_t BlurPass0 = 0u;
        uint32_t BlurPass1 = 0u;
        uint32_t TileCount = 0u;
        uint32_t LayerStride = 0u;
    };

    class PassLights : public Utilities::NoCopy
    {
        public:
            const uint32_t MaxLightsPerTile = 64;
            const uint32_t GridSizeX = 16;
            const uint32_t GridSizeY = 9;
            const uint32_t GridSizeZ = 24;
            const uint32_t ClusterCount = GridSizeX * GridSizeY * GridSizeZ;
            const float DepthGroupSize = 32.0f;

            PassLights(Core::Services::AssetDatabase* assetDatabase, 
                       ECS::EntityDatabase* entityDb, 
                       Core::Services::Sequencer* sequencer, 
                       Batcher* batcher, 
                       const Core::ApplicationConfig* config);
            void Cull(void* engineRoot, ECS::Tokens::VisibilityList* visibilityList, const Math::float4x4& viewProjection, float znear, float zfar);
            void Render(Objects::CommandBuffer* cmd);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
        
        private:
            void BuildShadowmapBatches(void* engineRoot, ECS::Tokens::CullTokens* tokens, ECS::EntityViews::LightRenderableView* view, const Math::float4x4& inverseViewProjection);

            ECS::EntityDatabase* m_entityDb = nullptr;
            Core::Services::Sequencer* m_sequencer = nullptr;
            Batcher* m_batcher = nullptr;
            Objects::Shader* m_computeLightAssignment = nullptr;
            Objects::Shader* m_shadowmapBlur = nullptr;
            float m_cascadeLinearity;
            uint32_t m_shadowmapCubeFaceSize;
            uint32_t m_shadowmapTileSize;
            uint32_t m_shadowmapTileCount;
            uint32_t m_shadowmapCount;
            uint32_t m_projectionCount;
            uint32_t m_lightCount;
            ShadowCascades m_cascadeSplits;
            ShadowmapLightTypeData m_shadowmapTypeData[(int)Structs::LightType::TypeCount];
            Utilities::MemoryBlock<ECS::EntityViews::LightRenderableView*> m_lights;
            Utilities::Ref<Objects::Buffer> m_lightsBuffer;
            Utilities::Ref<Objects::Buffer> m_lightMatricesBuffer;
            Utilities::Ref<Objects::Buffer> m_lightDirectionsBuffer;
            Utilities::Ref<Objects::Buffer> m_globalLightsList;
            Utilities::Ref<Objects::Buffer> m_globalLightIndex;
            Utilities::Ref<Objects::Texture> m_lightTiles;      
            Utilities::Ref<Objects::Texture> m_shadowmaps;        
    };
}