#pragma once
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"
#include "Utilities/MemoryBlock.h"

namespace PK::ECS::Tokens
{
	using namespace PK::Utilities;
	using namespace PK::Math;
	using namespace PK::Rendering::Structs;

	struct VisibleItem
	{
		uint entityId;
		ushort depth;
		ushort clipId;
	};

	struct VisibilityList
	{
		MemoryBlock<VisibleItem> results;
		size_t count;
		VisibilityList(size_t count) : results(512){}
		void Add(uint entityId, ushort depth, ushort clipId);
		inline void Clear() { count = 0ull; }
		inline const VisibleItem& operator [] (size_t index) const { return results[index]; }
	};

	struct TokenCullBase
	{
		VisibilityList* results;
		RenderableFlags mask;
		float depthRange;
	};

	struct TokenCullFrustum : public TokenCullBase
	{
		FrustumPlanes planes;
	};

	struct TokenCullCubeFaces : public TokenCullBase
	{
		BoundingBox aabb;
	};

	struct TokenCullCascades : public TokenCullBase
	{
		FrustumPlanes* cascades;
		uint count;
	};

	struct CullTokens
	{
		TokenCullFrustum frustum;
		TokenCullCubeFaces cube;
		TokenCullCascades cascades;
	};
}