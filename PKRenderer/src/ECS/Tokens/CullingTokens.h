#pragma once
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"
#include "Utilities/MemoryBlock.h"

namespace PK::ECS::Tokens
{
	struct VisibleItem
	{
		uint32_t entityId;
		uint16_t depth;
		uint16_t clipId;
	};

	struct VisibilityList
	{
		Utilities::MemoryBlock<VisibleItem> results;
		size_t count;
		VisibilityList(size_t count) : results(512), count(count){}
		void Add(uint32_t entityId, uint16_t depth, uint16_t clipId);
		inline void Clear() { count = 0ull; }
		inline const VisibleItem& operator [] (size_t index) const { return results[index]; }
	};

	struct TokenCullBase
	{
		VisibilityList* results;
		Rendering::Structs::RenderableFlags mask;
		float depthRange;
	};

	struct TokenCullFrustum : public TokenCullBase
	{
		Math::FrustumPlanes planes;
	};

	struct TokenCullCubeFaces : public TokenCullBase
	{
		Math::BoundingBox aabb;
	};

	struct TokenCullCascades : public TokenCullBase
	{
		Math::FrustumPlanes* cascades;
		uint32_t count;
	};

	struct CullTokens
	{
		TokenCullFrustum frustum;
		TokenCullCubeFaces cube;
		TokenCullCascades cascades;
	};
}