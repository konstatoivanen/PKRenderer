#pragma once
#include "Math/Types.h"
#include "CullingTokens.h"
#include "BatcherToken.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/GBuffers.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::ECS::Tokens
{
	// Events forwarded out of renderpipeline to engines that might use them
	enum class RenderEvent
	{
		CollectDraws,
		GBuffer,
		ForwardOpaque,
		ForwardTransparent,
		AfterPostEffects
	};

	struct TokenRenderEvent
	{
		Rendering::RHI::Objects::CommandBuffer* cmd;
		Rendering::GBuffersViewFull gbuffers;
		VisibilityList* visibilityList;
		IBatcher* batcher;

		uint32_t outPassGroup = 0xFFFFFFFFu;
		Math::float4x4 viewToClip;
		float znear;
		float zfar;

		TokenRenderEvent(Rendering::RHI::Objects::CommandBuffer* cmd,
						 Rendering::GBuffersViewFull gbuffers,
						 VisibilityList* visibilityList,
						 IBatcher* batcher,
						 const Math::float4x4& viewToClip, 
						 float znear, 
						 float zfar) :
			cmd(cmd),
			gbuffers(gbuffers),
			visibilityList(visibilityList),
			batcher(batcher),
			viewToClip(viewToClip),
			znear(znear),
			zfar(zfar)
		{
		}
	};
}