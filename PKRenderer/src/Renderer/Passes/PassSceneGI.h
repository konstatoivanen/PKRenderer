#pragma once
#include "Utilities/NoCopy.h"
#include "Graphics/RHI/RHI.h"
#include "Graphics/ShaderBindingTable.h"
#include "Graphics/GraphicsFwd.h"
#include "Renderer/IBatcher.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)

namespace PK::Renderer::Passes
{
    class PassSceneGI : public PK::Utilities::NoCopy
    {
        public:
            PassSceneGI(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void PreRender(Graphics::CommandBufferExt cmd, const Math::uint3& resolution);
            void PruneVoxels(Graphics::CommandBufferExt cmd);
            void DispatchRays(Graphics::CommandBufferExt cmd);
            void ReprojectGI(Graphics::CommandBufferExt cmd);
            void Voxelize(Graphics::CommandBufferExt cmd, IBatcher* batcher, uint32_t batchGroup);
            void RenderGI(Graphics::CommandBufferExt cmd);
            void VoxelMips(Graphics::CommandBufferExt cmd);
            void ValidateReservoirs(Graphics::CommandBufferExt cmd);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            Graphics::RHI::FixedFunctionShaderAttributes m_voxelizeAttribs{};
            Graphics::Shader* m_computeClear = nullptr;
            Graphics::Shader* m_computeMipmap = nullptr;
            Graphics::Shader* m_computeShadeHits = nullptr;
            Graphics::Shader* m_computeAccumulate = nullptr;
            Graphics::Shader* m_computeReproject = nullptr;
            Graphics::Shader* m_computeGradients = nullptr;
            Graphics::Shader* m_computePostFilter = nullptr;
            Graphics::Shader* m_rayTraceGatherGI = nullptr;
            Graphics::Shader* m_rayTraceValidate = nullptr;
            Graphics::ShaderBindingTable m_sbtRaytrace;
            Graphics::ShaderBindingTable m_sbtValidate;
            Graphics::ConstantBufferRef m_parameters;

            Graphics::TextureRef m_voxels;
            Graphics::TextureRef m_voxelMask;
            Graphics::TextureRef m_packedGIDiff;
            Graphics::TextureRef m_packedGISpec;
            Graphics::TextureRef m_resolvedGI;
            Graphics::TextureRef m_reservoirs0;
            Graphics::TextureRef m_reservoirs1;
            Graphics::TextureRef m_rayhits;

            uint32_t m_frameIndex = 0u;
            int32_t m_rasterAxis = 0;
            bool m_useCheckerboardTrace = false;
            bool m_useReSTIR = false;
    };
}