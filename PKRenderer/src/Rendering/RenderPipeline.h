#pragma once
#include "Core/IService.h"
#include "GraphicsAPI.h"
#include "ECS/Sequencer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Mesh.h"
#include "Core/Window.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;
    using namespace VulkanRHI::Objects;

    class RenderPipeline : public PK::Core::IService, public PK::ECS::IConditionalStep<PK::Core::Window>
    {
        public:
            RenderPipeline(AssetDatabase* assetDatabase, int width, int height);
            ~RenderPipeline();

            void Step(Window* window, int condition) override;

        private:
            FixedFunctionState m_fixedFunctionState;
            Mesh* m_mesh;
            Ref<Buffer> m_uniformBuffer;
            Texture* m_testTexture;
            Ref<VulkanTexture> m_depthTexture;
            Shader* m_shader = nullptr;
            uint m_rotation;
    };
}