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
    using namespace PK::Core;
    using namespace PK::ECS::Tokens;
    using namespace PK::ECS::EntityViews;
    using namespace PK::Rendering::Objects;

    typedef struct ShadowCascades { float planes[5]; } ShadowCascades;

    struct ShadowmapLightTypeData
    {
        Ref<RenderTexture> SceneRenderTarget = nullptr;
        uint32_t BlurPass0 = 0u;
        uint32_t BlurPass1 = 0u;
        uint32_t TileCount = 0u;
    };

    class PassLights : public PK::Core::NoCopy
    {
        public:
            const uint MaxLightsPerTile = 64;
            const uint GridSizeX = 16;
            const uint GridSizeY = 9;
            const uint GridSizeZ = 24;
            const uint ClusterCount = GridSizeX * GridSizeY * GridSizeZ;
            const float DepthGroupSize = 32.0f;

            PassLights(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher, const ApplicationConfig* config);
            void Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float znear, float zfar);
            void Render(CommandBuffer* cmd);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
        
        private:
            void BuildShadowmapBatches(void* engineRoot, CullTokens* tokens, LightRenderableView* view, const float4x4& inverseViewProjection);

            EntityDatabase* m_entityDb = nullptr;
            Sequencer* m_sequencer = nullptr;
            Batcher* m_batcher = nullptr;
            Shader* m_computeLightAssignment = nullptr;
            Shader* m_shadowmapBlur = nullptr;
            float m_cascadeLinearity;
            uint m_shadowmapCubeFaceSize;
            uint m_shadowmapTileSize;
            uint m_shadowmapTileCount;

            uint32_t m_shadowmapCount;
            uint32_t m_projectionCount;
            uint32_t m_lightCount;
            ShadowCascades m_cascadeSplits;
            ShadowmapLightTypeData m_shadowmapTypeData[(int)LightType::TypeCount];

            MemoryBlock<LightRenderableView*> m_lights;
            Ref<Buffer> m_lightsBuffer;
            Ref<Buffer> m_lightMatricesBuffer;
            Ref<Buffer> m_lightDirectionsBuffer;
            Ref<Buffer> m_globalLightsList;
            Ref<Buffer> m_globalLightIndex;
            Ref<Texture> m_lightTiles;      
            Ref<Texture> m_shadowmaps;        
    };
}