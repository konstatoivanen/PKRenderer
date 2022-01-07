#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Batcher.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Core;
    using namespace PK::ECS::Tokens;
    using namespace PK::Rendering::Objects;

    class PassSceneGI : public PK::Core::NoCopy
    {
        public:
            PassSceneGI(AssetDatabase* assetDatabase, const ApplicationConfig* config);
            void PreRender(CommandBuffer* cmd, const uint3& resolution);
            void RenderVoxels(CommandBuffer* cmd, Batcher* batcher, uint32_t batchGroup);
            void RenderGI(CommandBuffer* cmd);

        private:
            FixedFunctionShaderAttributes m_voxelizeAttribs{};
            Shader* m_computeMipmap = nullptr;
            Shader* m_computeBakeGI = nullptr;
            Ref<ConstantBuffer> m_parameters;
            Ref<Texture> m_voxels;
            Ref<Texture> m_screenSpaceGI;
            uint m_checkerboardIndex = 0u;
            int m_rasterAxis = 0;
    };
}