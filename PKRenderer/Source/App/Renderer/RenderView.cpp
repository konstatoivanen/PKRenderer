#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/RHI/RHInterfaces.h"
#include "RenderView.h"

namespace PK::App
{
    uint3 GBuffers::GetResolution() const { return color->GetResolution(); }
    float GBuffers::GetAspectRatio() const { return float(color->GetResolution().x) / float(color->GetResolution().y); }
    GBuffers::View GBuffers::GetView() { return { color.get(), normals.get(), depthBiased.get(), depth.get() }; }

    bool GBuffers::Validate(const uint2& resolution, const Descriptor& descriptor, const char* namePrefix)
    {
        auto alignedResolution = resolution;
        alignedResolution.x = Math::GetAlignedSize(resolution.x, RESOLUTION_ALIGNMENT);
        alignedResolution.y = Math::GetAlignedSize(resolution.y, RESOLUTION_ALIGNMENT);

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
                isOutOfDate |= RHI::ValidateTexture((&color)[i], textureDescriptor, (std::string(namePrefix) + Names[i]).c_str());
            }
        }

        return isOutOfDate;
    }

    bool GBuffersFull::Validate(const uint2& resolution, const GBuffersFullDescriptor& descriptor, const char* namePrefix)
    {
        bool isOutOfDate = false;
        isOutOfDate |= current.Validate(resolution, descriptor.current, (std::string(namePrefix) + ".Current.").c_str());
        isOutOfDate |= previous.Validate(resolution, descriptor.previous, (std::string(namePrefix) + ".Previous.").c_str());
        return isOutOfDate;
    }
}