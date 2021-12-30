#include "PrecompiledHeader.h"
#include "CullingTokens.h"
#include "Utilities/VectorUtilities.h"

namespace PK::ECS::Tokens
{
    void VisibilityList::Add(uint entityId, ushort depth, ushort clipId)
    {
        Utilities::Vector::ValidateSize(results, count + 1);
        results[count++] = { entityId, depth, clipId };
    }
}