#pragma once
#include "Utilities/NoCopy.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/ShaderBindingTable.h"
#include "Rendering/Services/Batcher.h"

namespace PK::Rendering::Passes
{
    class PassSceneGI : public PK::Utilities::NoCopy
    {
        public:
            PassSceneGI(Core::Services::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void PreRender(Objects::CommandBuffer* cmd, const Math::uint3& resolution);
            void PruneVoxels(Objects::CommandBuffer* cmd);
            void DispatchRays(Objects::CommandBuffer* cmd);
            void RenderVoxels(Objects::CommandBuffer* cmd, Batcher* batcher, uint32_t batchGroup);
            void RenderGI(Objects::CommandBuffer* cmd);

        private:
            Structs::FixedFunctionShaderAttributes m_voxelizeAttribs{};
            Objects::Shader* m_computeClear = nullptr;
            Objects::Shader* m_computeMipmap = nullptr;
            Objects::Shader* m_computeBakeGI = nullptr;
            Objects::Shader* m_computeReprojectMask = nullptr;
            Objects::Shader* m_computeDenoise = nullptr;
            Objects::Shader* m_rayTraceGatherGI = nullptr;
            Objects::ShaderBindingTable m_shaderBindingTable;
            Utilities::Ref<Objects::ConstantBuffer> m_parameters;
            Utilities::Ref<Objects::Texture> m_voxels;
            Utilities::Ref<Objects::Texture> m_voxelMask;
            Utilities::Ref<Objects::Texture> m_screenSpaceSHY;
            Utilities::Ref<Objects::Texture> m_screenSpaceCoCg;
            Utilities::Ref<Objects::Texture> m_screenSpaceMask;
            Utilities::Ref<Objects::Texture> m_screenSpaceRayhits;
            Utilities::Ref<Objects::Texture> m_screenSpaceAO;
            uint32_t m_rayIndex = 0u;
            uint32_t m_checkerboardIndex = 0u;
            int32_t m_rasterAxis = 0;
    };
}