#pragma once
#include "Core/IService.h"
#include "GraphicsAPI.h"
#include "ECS/Sequencer.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/Objects/VulkanShader.h"
#include "Core/Window.h"

namespace PK::Rendering
{
    using namespace VulkanRHI::Objects;

    class RenderPipeline : public PK::Core::IService, public PK::ECS::IConditionalStep<PK::Core::Window>
    {
        public:
            RenderPipeline(int width, int height);
            ~RenderPipeline();

            void Step(Window* window, int condition) override;

        private:
            FixedFunctionState m_fixedFunctionState;
            Ref<VulkanBuffer> m_vertexBuffer;
            Ref<VulkanBuffer> m_indexBuffer;
            Ref<VulkanBuffer> m_uniformBuffer;
            Ref<VulkanTexture> m_vulkanTexture;
            Ref<VulkanTexture> m_depthTexture;
            Ref<VulkanShader> m_shader;
            uint m_rotation;
    };
}