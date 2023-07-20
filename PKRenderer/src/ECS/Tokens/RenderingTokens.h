#pragma once
#include "Math/Types.h"
#include "CullingTokens.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/CommandBuffer.h"

namespace PK::ECS::Tokens
{
	struct TokenRenderBase
	{
		Rendering::Objects::CommandBuffer* cmd;
		Math::float4x4 viewProjection;
		float znear;
		float zfar;

		TokenRenderBase(Rendering::Objects::CommandBuffer* cmd, 
			const Math::float4x4& vp, 
			float znear, 
			float zfar) :
			cmd(cmd),
			viewProjection(vp),
			znear(znear),
			zfar(zfar)
		{
		}
	};

	struct TokenRenderCollectDrawCalls : public TokenRenderBase 
	{
		VisibilityList* visibilityList;

		TokenRenderCollectDrawCalls(Rendering::Objects::CommandBuffer* cmd, 
			const Math::float4x4& vp, 
			float znear, 
			float zfar, 
			VisibilityList* list) :
			TokenRenderBase(cmd, vp, znear, zfar),
			visibilityList(list)
		{
		}
	};

	// @TODO add more here as needed
	struct TokenRenderAfterPostEffects : public TokenRenderBase 
	{
		Rendering::Objects::RenderTexture* renderTarget;

		TokenRenderAfterPostEffects(Rendering::Objects::CommandBuffer* cmd,
			const Math::float4x4& vp,
			float znear,
			float zfar,
			Rendering::Objects::RenderTexture* target) :
			TokenRenderBase(cmd, vp, znear, zfar),
			renderTarget(target)
		{
		}
	};
}