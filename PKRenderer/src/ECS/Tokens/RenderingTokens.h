#pragma once
#include "Math/Types.h"
#include "CullingTokens.h"
#include "BatcherToken.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/CommandBuffer.h"

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
		Rendering::Objects::CommandBuffer* cmd;
		Rendering::Objects::RenderTexture* renderTarget;
		VisibilityList* visibilityList;
		IBatcher* batcher;

		uint32_t outPassGroup = 0xFFFFFFFFu;
		Math::float4x4 viewProjection;
		float znear;
		float zfar;

		TokenRenderEvent(Rendering::Objects::CommandBuffer* cmd,
						 Rendering::Objects::RenderTexture* renderTarget,
						 VisibilityList* visibilityList,
						 IBatcher* batcher,
						 const Math::float4x4& vp, 
						 float znear, 
						 float zfar) :
			cmd(cmd),
			renderTarget(renderTarget),
			visibilityList(visibilityList),
			batcher(batcher),
			viewProjection(vp),
			znear(znear),
			zfar(zfar)
		{
		}
	};
}