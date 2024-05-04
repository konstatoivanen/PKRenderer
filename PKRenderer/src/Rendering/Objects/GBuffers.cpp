#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "GBuffers.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Math;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    bool GBuffers::Validate(const uint2& resolution, const Descriptor& descriptor, const char* namePrefix)
    {
        auto alignedResolution = resolution;
        alignedResolution.x = Functions::GetAlignedSize(resolution.x, RESOLUTION_ALIGNMENT);
        alignedResolution.y = Functions::GetAlignedSize(resolution.y, RESOLUTION_ALIGNMENT);

        TextureDescriptor textureDescriptor{};
        textureDescriptor.resolution = { alignedResolution.x, alignedResolution.y, 1 };
        textureDescriptor.sampler.filterMin = FilterMode::Bilinear;
        textureDescriptor.sampler.filterMag = FilterMode::Bilinear;

        auto isOutOfDate = false;

        for (auto i = 0; i < Count; ++i)
        {
            textureDescriptor.format = descriptor.formats[i];
            textureDescriptor.usage = descriptor.usages[i];

            if (textureDescriptor.format == TextureFormat::Invalid ||
                textureDescriptor.usage == TextureUsage::None)
            {
                isOutOfDate |= (&color)[i] != nullptr;
                (&color)[i] = nullptr;
            }
            else
            {
                isOutOfDate |= Texture::Validate((&color)[i], textureDescriptor, (std::string(namePrefix) + Names[i]).c_str());
            }
        }

        return isOutOfDate;
    }

    bool GBuffersFull::Validate(const uint2& resolution, const Descriptor& descriptor, const char* namePrefix)
    {
        bool isOutOfDate = false;
        isOutOfDate |= current.Validate(resolution, descriptor.current, (std::string(namePrefix) + ".Current.").c_str());
        isOutOfDate |= previous.Validate(resolution, descriptor.previous, (std::string(namePrefix) + ".Previous.").c_str());
        return isOutOfDate;
    }
}