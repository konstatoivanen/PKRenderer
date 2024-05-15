#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Geometry/IBatcher.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/ShaderBindingTable.h"
#include "Rendering/RHI/RHI.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core, struct ApplicationConfig)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)

namespace PK::Rendering::Passes
{
    class PassSceneGI : public PK::Utilities::NoCopy
    {
        public:
            PassSceneGI(Core::Assets::AssetDatabase* assetDatabase, const Core::ApplicationConfig* config);
            void PreRender(RHI::Objects::CommandBuffer* cmd, const Math::uint3& resolution);
            void PruneVoxels(RHI::Objects::CommandBuffer* cmd);
            void DispatchRays(RHI::Objects::CommandBuffer* cmd);
            void ReprojectGI(RHI::Objects::CommandBuffer* cmd);
            void Voxelize(RHI::Objects::CommandBuffer* cmd, Geometry::IBatcher* batcher, uint32_t batchGroup);
            void RenderGI(RHI::Objects::CommandBuffer* cmd);
            void VoxelMips(RHI::Objects::CommandBuffer* cmd);
            void ValidateReservoirs(RHI::Objects::CommandBuffer* cmd);
            void OnUpdateParameters(const Core::ApplicationConfig* config);

        private:
            RHI::FixedFunctionShaderAttributes m_voxelizeAttribs{};
            RHI::Objects::Shader* m_computeClear = nullptr;
            RHI::Objects::Shader* m_computeMipmap = nullptr;
            RHI::Objects::Shader* m_computeShadeHits = nullptr;
            RHI::Objects::Shader* m_computeAccumulate = nullptr;
            RHI::Objects::Shader* m_computeReproject = nullptr;
            RHI::Objects::Shader* m_computeGradients = nullptr;
            RHI::Objects::Shader* m_computePostFilter = nullptr;
            RHI::Objects::Shader* m_rayTraceGatherGI = nullptr;
            RHI::Objects::Shader* m_rayTraceValidate = nullptr;
            Rendering::Objects::ShaderBindingTable m_sbtRaytrace;
            Rendering::Objects::ShaderBindingTable m_sbtValidate;
            Rendering::Objects::ConstantBufferRef m_parameters;

            RHI::Objects::TextureRef m_voxels;
            RHI::Objects::TextureRef m_voxelMask;
            RHI::Objects::TextureRef m_packedGIDiff;
            RHI::Objects::TextureRef m_packedGISpec;
            RHI::Objects::TextureRef m_resolvedGI;
            RHI::Objects::TextureRef m_reservoirs0;
            RHI::Objects::TextureRef m_reservoirs1;
            RHI::Objects::TextureRef m_rayhits;

            uint32_t m_frameIndex = 0u;
            int32_t m_rasterAxis = 0;
            bool m_useCheckerboardTrace = false;
            bool m_useReSTIR = false;
    };
}