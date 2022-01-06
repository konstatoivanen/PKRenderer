#include "PrecompiledHeader.h"
#include "CullingTokens.h"
#include "Utilities/VectorUtilities.h"

namespace PK::ECS::Tokens
{
    void VisibilityList::Add(uint entityId, ushort depth, ushort clipId)
    {
        results.Validate(count + 1u);
        results[count++] = { entityId, depth, clipId };
    }
}