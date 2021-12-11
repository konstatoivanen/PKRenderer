#pragma once
#include "Core/IService.h"
#include "GraphicsAPI.h"
#include "ECS/Sequencer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Mesh.h"
#include "Core/Window.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;

    class RenderPipeline : public PK::Core::IService, public PK::ECS::IConditionalStep<PK::Core::Window>
    {
        public:
            RenderPipeline(AssetDatabase* assetDatabase, int width, int height);
            ~RenderPipeline();

            void Step(Window* window, int condition) override;

        private:
            Ref<Buffer> m_perFrameConstants;
            Ref<Buffer> m_modelMatrices;
            Ref<RenderTexture> m_renderTarget;
            Mesh* m_mesh;
            Texture* m_testTexture;
            Shader* m_shader = nullptr;
            uint m_rotation;
    };
}