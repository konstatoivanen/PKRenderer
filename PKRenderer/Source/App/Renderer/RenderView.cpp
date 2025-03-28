#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Utilities/FixedString.h"
#include "Core/RHI/RHInterfaces.h"
#include "RenderView.h"

namespace PK::App
{
    IRenderViewResources::~IRenderViewResources() = default;

    uint2 GBuffers::AlignResolution(const uint2& resolution)
    {
        auto alignedResolution = resolution;
        alignedResolution.x = Math::GetAlignedSize(resolution.x, RESOLUTION_ALIGNMENT);
        alignedResolution.y = Math::GetAlignedSize(resolution.y, RESOLUTION_ALIGNMENT);
        return alignedResolution;
    }

    uint3 GBuffers::GetResolution() const { return color->GetResolution(); }
    float GBuffers::GetAspectRatio() const { return float(color->GetResolution().x) / float(color->GetResolution().y); }
    GBuffers::View GBuffers::GetView() { return { color.get(), normals.get(), depthBiased.get(), depth.get() }; }

    bool GBuffers::Validate(const uint2& resolution, const Descriptor& descriptor, const char* namePrefix)
    {
        auto alignedResolution = AlignResolution(resolution);

        TextureDescriptor textureDescriptor{};
        textureDescriptor.resolution = { alignedResolution.x, alignedResolution.y, 1 };
        textureDescriptor.sampler.filterMin = FilterMode::Bilinear;
        textureDescriptor.sampler.filterMag = FilterMode::Bilinear;

        auto isOutOfDate = false;

        for (auto i = 0; i < Count; ++i)
        {
            textureDescriptor.format = descriptor[i].format;
            textureDescriptor.usage = descriptor[i].usage;

            if (textureDescriptor.format == TextureFormat::Invalid ||
                textureDescriptor.usage == TextureUsage::None)
            {
                isOutOfDate |= (&color)[i] != nullptr;
                (&color)[i] = nullptr;
            }
            else
            {
                isOutOfDate |= RHI::ValidateTexture((&color)[i], textureDescriptor, FixedString64({namePrefix, Names[i]}).c_str());
            }
        }

        return isOutOfDate;
    }

    bool GBuffersFull::Validate(const uint2& resolution, const GBuffersFullDescriptor& descriptor, const char* namePrefix)
    {
        bool isOutOfDate = false;
        isOutOfDate |= current.Validate(resolution, descriptor.current, FixedString64({namePrefix,".Current."}).c_str());
        isOutOfDate |= previous.Validate(resolution, descriptor.previous, FixedString64({namePrefix,".Previous."}).c_str());
        return isOutOfDate;
    }
}