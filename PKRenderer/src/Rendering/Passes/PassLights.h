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

    class PassLights : public PK::Core::NoCopy
    {
        public:
            const uint MaxLightsPerTile = 64;
            const uint GridSizeX = 16;
            const uint GridSizeY = 9;
            const uint GridSizeZ = 24;
            const uint ClusterCount = GridSizeX * GridSizeY * GridSizeZ;
            const float DepthGroupSize = 32.0f;

            PassLights(EntityDatabase* entityDb, Sequencer* sequencer, Batcher* batcher, float cascadeLinearity);
            void Cull(void* engineRoot, VisibilityList* visibilityList, const float4x4& viewProjection, float znear, float zfar);
            void RenderShadowmaps(CommandBuffer* cmd);
            void RenderTiles(CommandBuffer* cmd);
            ShadowCascades GetCascadeZSplits(float znear, float zfar) const;
        
        private:
            EntityDatabase* m_entityDb = nullptr;
            Sequencer* m_sequencer = nullptr;
            Batcher* m_batcher = nullptr;
            Ref<Buffer> m_lights;
            std::vector<uint32_t> m_passGroups;
            std::vector<LightRenderableView*> m_visibleLights;
            const float m_cascadeLinearity;
    };
}