#include "PrecompiledHeader.h"
#include "CullingTokens.h"
#include "Utilities/VectorUtilities.h"

namespace PK::ECS::Tokens
{
    void VisibilityList::Add(uint32_t entityId, uint16_t depth, uint16_t clipId)
    {
        results.Validate(count + 1u);
        results[count++] = { entityId, depth, clipId };
    }
}