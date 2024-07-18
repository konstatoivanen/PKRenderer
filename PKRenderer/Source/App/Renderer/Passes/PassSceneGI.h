#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/CLI/CVariable.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/Rendering/ShaderBindingTable.h"
#include "App/Renderer/IBatcher.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    class PassSceneGI : public NoCopy
    {
        public:
            PassSceneGI(AssetDatabase* assetDatabase, const uint2& initialResolution);
            void SetViewConstants(struct RenderView* view);
            void PreRender(CommandBufferExt cmd, const uint3& resolution);
            void PruneVoxels(CommandBufferExt cmd);
            void DispatchRays(CommandBufferExt cmd);
            void ReprojectGI(CommandBufferExt cmd);
            void Voxelize(CommandBufferExt cmd, IBatcher* batcher, uint32_t batchGroup);
            void RenderGI(CommandBufferExt cmd);
            void VoxelMips(CommandBufferExt cmd);
            void ValidateReservoirs(CommandBufferExt cmd);

        private:
            FixedFunctionShaderAttributes m_voxelizeAttribs{};
            ShaderAsset* m_computeClear = nullptr;
            ShaderAsset* m_computeMipmap = nullptr;
            ShaderAsset* m_computeShadeHits = nullptr;
            ShaderAsset* m_computeAccumulate = nullptr;
            ShaderAsset* m_computeReproject = nullptr;
            ShaderAsset* m_computeGradients = nullptr;
            ShaderAsset* m_computePostFilter = nullptr;
            ShaderAsset* m_rayTraceGatherGI = nullptr;
            ShaderAsset* m_rayTraceValidate = nullptr;
            ShaderBindingTable m_sbtRaytrace;
            ShaderBindingTable m_sbtValidate;

            RHITextureRef m_voxels;
            RHITextureRef m_voxelMask;
            RHITextureRef m_packedGIDiff;
            RHITextureRef m_packedGISpec;
            RHITextureRef m_resolvedGI;
            RHITextureRef m_reservoirs0;
            RHITextureRef m_reservoirs1;
            RHITextureRef m_rayhits;

            uint32_t m_frameIndex = 0u;
            int32_t m_rasterAxis = 0;

            struct Settings
            {
                CVariableField<bool> ReSTIR = { "Renderer.GI.ReSTIR" , true};
                CVariableField<bool> approximateRoughSpecular = { "Renderer.GI.ApproximateRoughSpecular" , true};
                CVariableField<bool> screenSpacePretrace = { "Renderer.GI.ScreenSpacePretrace" , false};
                CVariableField<bool> checkerboardTrace = { "Renderer.GI.CheckerboardTrace" , true};
                CVariableField<bool> specularVirtualReproject = { "Renderer.GI.SpecularVirtualReproject" , true};
            }
            m_settings;
    };
}