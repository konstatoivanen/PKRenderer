#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMatrix.h"
#include "ComponentProjection.h"

namespace PK::App
{
    float4x4 ComponentProjection::ResolveProjectionMatrix(float aspect)
    {
        switch (mode)
        {
            case ComponentProjection::CustomMatrix: return customViewToClip;
            case ComponentProjection::Perspective: return Math::GetPerspective(fieldOfView, aspect, zNear, zFar);
            case ComponentProjection::OrthoGraphic: return Math::GetOrtho(orthoBounds.min.x * aspect, orthoBounds.max.x * aspect, orthoBounds.min.y, orthoBounds.max.y, orthoBounds.min.z, orthoBounds.max.z);
        }

        return PK_FLOAT4X4_IDENTITY;
    }
}
