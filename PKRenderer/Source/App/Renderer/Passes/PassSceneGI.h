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
            struct ViewResources
            {
                RHITextureRef packedGIDiff;
                RHITextureRef packedGISpec;
                RHITextureRef resolvedGI;
                RHITextureRef reservoirs0;
                RHITextureRef reservoirs1;
                RHITextureRef rayhits;
                bool hasResisedTargets;
            };

            PassSceneGI(AssetDatabase* assetDatabase);
            void SetViewConstants(struct RenderView* view);
            void PruneVoxels(CommandBufferExt cmd);
            void DispatchRays(CommandBufferExt cmd, RenderPipelineContext* context);
            void ReprojectGI(CommandBufferExt cmd, RenderPipelineContext* context);
            void Voxelize(CommandBufferExt cmd, RenderPipelineContext* context);
            void RenderGI(CommandBufferExt cmd, RenderPipelineContext* context);
            void VoxelMips(CommandBufferExt cmd);
            void ValidateReservoirs(CommandBufferExt cmd, RenderPipelineContext* context);

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
