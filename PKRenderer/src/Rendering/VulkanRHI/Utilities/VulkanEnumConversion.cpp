#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::EnumConvert
{
    using namespace Rendering::Structs;

    VkFormat GetFormat(ElementType format)
    {
        switch (format)
        {
            case Structs::ElementType::Invalid: return VK_FORMAT_UNDEFINED;
            case Structs::ElementType::Float: return VK_FORMAT_R32_SFLOAT;
            case Structs::ElementType::Float2: return VK_FORMAT_R32G32_SFLOAT;
            case Structs::ElementType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
            case Structs::ElementType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Structs::ElementType::Double: return VK_FORMAT_R64_SFLOAT;
            case Structs::ElementType::Double2: return VK_FORMAT_R64G64_SFLOAT;
            case Structs::ElementType::Double3: return VK_FORMAT_R64G64B64_SFLOAT;
            case Structs::ElementType::Double4: return VK_FORMAT_R64G64B64A64_SFLOAT;
            case Structs::ElementType::Half: return VK_FORMAT_R16_SFLOAT;
            case Structs::ElementType::Half2: return VK_FORMAT_R16G16_SFLOAT;
            case Structs::ElementType::Half3: return VK_FORMAT_R16G16B16_SFLOAT;
            case Structs::ElementType::Half4: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case Structs::ElementType::Int: return VK_FORMAT_R32_SINT;
            case Structs::ElementType::Int2: return VK_FORMAT_R32G32_SINT;
            case Structs::ElementType::Int3: return VK_FORMAT_R32G32B32_SINT;
            case Structs::ElementType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
            case Structs::ElementType::Uint: return VK_FORMAT_R32_UINT;
            case Structs::ElementType::Uint2: return VK_FORMAT_R32G32_UINT;
            case Structs::ElementType::Uint3: return VK_FORMAT_R32G32B32_UINT;
            case Structs::ElementType::Uint4: return VK_FORMAT_R32G32B32A32_UINT;
            case Structs::ElementType::Short: return VK_FORMAT_R16_SINT;
            case Structs::ElementType::Short2: return VK_FORMAT_R16G16_SINT;
            case Structs::ElementType::Short3: return VK_FORMAT_R16G16B16_SINT;
            case Structs::ElementType::Short4: return VK_FORMAT_R16G16B16A16_SINT;
            case Structs::ElementType::Ushort: return VK_FORMAT_R16_UINT;
            case Structs::ElementType::Ushort2: return VK_FORMAT_R16G16_UINT;
            case Structs::ElementType::Ushort3: return VK_FORMAT_R16G16B16_UINT;
            case Structs::ElementType::Ushort4: return VK_FORMAT_R16G16B16A16_UINT;
            case Structs::ElementType::Long: return VK_FORMAT_R64_SINT;
            case Structs::ElementType::Long2: return VK_FORMAT_R64G64_SINT;
            case Structs::ElementType::Long3: return VK_FORMAT_R64G64B64_SINT;
            case Structs::ElementType::Long4: return VK_FORMAT_R64G64B64A64_SINT;
            case Structs::ElementType::Ulong: return VK_FORMAT_R64_UINT;
            case Structs::ElementType::Ulong2: return VK_FORMAT_R64G64_UINT;
            case Structs::ElementType::Ulong3: return VK_FORMAT_R64G64B64_UINT;
            case Structs::ElementType::Ulong4: return VK_FORMAT_R64G64B64A64_UINT;
            case Structs::ElementType::Float2x2: return VK_FORMAT_R32G32_SFLOAT;
            case Structs::ElementType::Float3x3: return VK_FORMAT_R32G32B32_SFLOAT;
            case Structs::ElementType::Float4x4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Structs::ElementType::Double2x2: return VK_FORMAT_R64G64_SFLOAT;
            case Structs::ElementType::Double3x3: return VK_FORMAT_R64G64B64_SFLOAT;
            case Structs::ElementType::Double4x4: return VK_FORMAT_R64G64B64A64_SFLOAT;
            case Structs::ElementType::Half2x2: return VK_FORMAT_R16G16_SFLOAT;
            case Structs::ElementType::Half3x3: return VK_FORMAT_R16G16B16_SFLOAT;
            case Structs::ElementType::Half4x4: return VK_FORMAT_R16G16B16A16_SFLOAT;
        }

        return VK_FORMAT_UNDEFINED;
    }

    ElementType GetElementType(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R32_SFLOAT: return Structs::ElementType::Float;
            case VK_FORMAT_R32G32_SFLOAT: return Structs::ElementType::Float2;
            case VK_FORMAT_R32G32B32_SFLOAT: return Structs::ElementType::Float3;
            case VK_FORMAT_R32G32B32A32_SFLOAT: return Structs::ElementType::Float4;
            case VK_FORMAT_R64_SFLOAT: return Structs::ElementType::Double;
            case VK_FORMAT_R64G64_SFLOAT: return Structs::ElementType::Double2;
            case VK_FORMAT_R64G64B64_SFLOAT: return Structs::ElementType::Double3;
            case VK_FORMAT_R64G64B64A64_SFLOAT: return Structs::ElementType::Double4;
            case VK_FORMAT_R16_SFLOAT: return Structs::ElementType::Half;
            case VK_FORMAT_R16G16_SFLOAT: return Structs::ElementType::Half2;
            case VK_FORMAT_R16G16B16_SFLOAT: return Structs::ElementType::Half3;
            case VK_FORMAT_R16G16B16A16_SFLOAT: return Structs::ElementType::Half4;
            case VK_FORMAT_R32_SINT: return Structs::ElementType::Int;
            case VK_FORMAT_R32G32_SINT: return Structs::ElementType::Int2;
            case VK_FORMAT_R32G32B32_SINT: return Structs::ElementType::Int3;
            case VK_FORMAT_R32G32B32A32_SINT: return Structs::ElementType::Int4;
            case VK_FORMAT_R32_UINT: return Structs::ElementType::Uint;
            case VK_FORMAT_R32G32_UINT: return Structs::ElementType::Uint2;
            case VK_FORMAT_R32G32B32_UINT: return Structs::ElementType::Uint3;
            case VK_FORMAT_R32G32B32A32_UINT: return Structs::ElementType::Uint4;
            case VK_FORMAT_R16_SINT: return Structs::ElementType::Short;
            case VK_FORMAT_R16G16_SINT: return Structs::ElementType::Short2;
            case VK_FORMAT_R16G16B16_SINT: return Structs::ElementType::Short3;
            case VK_FORMAT_R16G16B16A16_SINT: return Structs::ElementType::Short4;
            case VK_FORMAT_R16_UINT: return Structs::ElementType::Ushort;
            case VK_FORMAT_R16G16_UINT: return Structs::ElementType::Ushort2;
            case VK_FORMAT_R16G16B16_UINT: return Structs::ElementType::Ushort3;
            case VK_FORMAT_R16G16B16A16_UINT: return Structs::ElementType::Ushort4;
            case VK_FORMAT_R64_SINT: return Structs::ElementType::Long;
            case VK_FORMAT_R64G64_SINT: return Structs::ElementType::Long2;
            case VK_FORMAT_R64G64B64_SINT: return Structs::ElementType::Long3;
            case VK_FORMAT_R64G64B64A64_SINT: return Structs::ElementType::Long4;
            case VK_FORMAT_R64_UINT: return Structs::ElementType::Ulong;
            case VK_FORMAT_R64G64_UINT: return Structs::ElementType::Ulong2;
            case VK_FORMAT_R64G64B64_UINT: return Structs::ElementType::Ulong3;
            case VK_FORMAT_R64G64B64A64_UINT: return Structs::ElementType::Ulong4;
        }

        return Structs::ElementType::Invalid;
    }

    VkFormat GetFormat(TextureFormat format)
    {
        switch (format)
        {
            // 8 bits per element.
            case TextureFormat::R8:                 return VK_FORMAT_R8_UNORM;
            case TextureFormat::R8_SNORM:           return VK_FORMAT_R8_SNORM;
            case TextureFormat::R8UI:               return VK_FORMAT_R8_UINT;
            case TextureFormat::R8I:                return VK_FORMAT_R8_SINT;
            case TextureFormat::Stencil8:           return VK_FORMAT_S8_UINT;
            case TextureFormat::R16F:               return VK_FORMAT_R16_SFLOAT;
            case TextureFormat::R16UI:              return VK_FORMAT_R16_UINT;
            case TextureFormat::R16I:               return VK_FORMAT_R16_SINT;
            case TextureFormat::RG8:                return VK_FORMAT_R8G8_UNORM;
            case TextureFormat::RG8_SNORM:          return VK_FORMAT_R8G8_SNORM;
            case TextureFormat::RG8UI:              return VK_FORMAT_R8G8_UINT;
            case TextureFormat::RG8I:               return VK_FORMAT_R8G8_SINT;
            case TextureFormat::RGB565:             return VK_FORMAT_R5G6B5_UNORM_PACK16;
            case TextureFormat::RGB5A1:             return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
            case TextureFormat::RGBA4:              return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
            case TextureFormat::Depth16:            return VK_FORMAT_D16_UNORM;
            case TextureFormat::RGB8:               return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::RGB8_SRGB:          return VK_FORMAT_R8G8B8_SRGB;
            case TextureFormat::RGB8_SNORM:         return VK_FORMAT_R8G8B8_SNORM;
            case TextureFormat::RGB8UI:             return VK_FORMAT_R8G8B8_UINT;
            case TextureFormat::RGB8I:              return VK_FORMAT_R8G8B8_SINT;
            case TextureFormat::R32F:               return VK_FORMAT_R32_SFLOAT;
            case TextureFormat::R32UI:              return VK_FORMAT_R32_UINT;
            case TextureFormat::R32I:               return VK_FORMAT_R32_SINT;
            case TextureFormat::RG16F:              return VK_FORMAT_R16G16_SFLOAT;
            case TextureFormat::RG16UI:             return VK_FORMAT_R16G16_UINT;
            case TextureFormat::RG16I:              return VK_FORMAT_R16G16_SINT;
            case TextureFormat::B10G11R11UF:        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case TextureFormat::RGB9E5:             return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
            case TextureFormat::RGBA8:              return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_SRGB:         return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::BGRA8_SRGB:         return VK_FORMAT_B8G8R8A8_SRGB;
            case TextureFormat::RGBA8_SNORM:        return VK_FORMAT_R8G8B8A8_SNORM;
            case TextureFormat::RGB10A2:            return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case TextureFormat::RGBA8UI:            return VK_FORMAT_R8G8B8A8_UINT;
            case TextureFormat::RGBA8I:             return VK_FORMAT_R8G8B8A8_SINT;
            case TextureFormat::Depth32F:           return VK_FORMAT_D32_SFLOAT;
            case TextureFormat::Depth24_Stencil8:   return VK_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::Depth32F_Stencil8:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case TextureFormat::RGB16F:             return VK_FORMAT_R16G16B16_SFLOAT;
            case TextureFormat::RGB16UI:            return VK_FORMAT_R16G16B16_UINT;
            case TextureFormat::RGB16I:             return VK_FORMAT_R16G16B16_SINT;
            case TextureFormat::RG32F:              return VK_FORMAT_R32G32_SFLOAT;
            case TextureFormat::RG32UI:             return VK_FORMAT_R32G32_UINT;
            case TextureFormat::RG32I:              return VK_FORMAT_R32G32_SINT;
            case TextureFormat::RGBA16:             return VK_FORMAT_R16G16B16A16_UNORM;;
            case TextureFormat::RGBA16F:            return VK_FORMAT_R16G16B16A16_SFLOAT;
            case TextureFormat::RGBA16UI:           return VK_FORMAT_R16G16B16A16_UINT;
            case TextureFormat::RGBA16I:            return VK_FORMAT_R16G16B16A16_SINT;
            case TextureFormat::RGB32F:             return VK_FORMAT_R32G32B32_SFLOAT;
            case TextureFormat::RGB32UI:            return VK_FORMAT_R32G32B32_UINT;
            case TextureFormat::RGB32I:             return VK_FORMAT_R32G32B32_SINT;
            case TextureFormat::RGBA32F:            return VK_FORMAT_R32G32B32A32_SFLOAT;
            case TextureFormat::RGBA32UI:           return VK_FORMAT_R32G32B32A32_UINT;
            case TextureFormat::RGBA32I:            return VK_FORMAT_R32G32B32A32_SINT;
            case TextureFormat::RGBA64UI:           return VK_FORMAT_R64G64B64A64_UINT;
            case TextureFormat::BC1_RGB:            return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            case TextureFormat::BC1_SRGB:           return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            case TextureFormat::BC1_RGBA:           return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case TextureFormat::BC1_SRGBA:          return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case TextureFormat::BC2_RGBA:           return VK_FORMAT_BC2_UNORM_BLOCK;
            case TextureFormat::BC2_SRGBA:          return VK_FORMAT_BC2_SRGB_BLOCK;
            case TextureFormat::BC3_RGBA:           return VK_FORMAT_BC3_UNORM_BLOCK;
            case TextureFormat::BC3_SRGBA:          return VK_FORMAT_BC3_SRGB_BLOCK;
            case TextureFormat::BC4:                return VK_FORMAT_BC4_UNORM_BLOCK;
            case TextureFormat::BC6H_RGBUF:         return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case TextureFormat::BC6H_RGBF:          return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case TextureFormat::BC7_UNORM:          return VK_FORMAT_BC7_UNORM_BLOCK;
            default:                                return VK_FORMAT_UNDEFINED;
        }
    }

    VkIndexType GetIndexType(ElementType format)
    {
        switch (format)
        {
            case ElementType::Ushort: return VK_INDEX_TYPE_UINT16;
            case ElementType::Uint: return VK_INDEX_TYPE_UINT32;
        }

        return VK_INDEX_TYPE_MAX_ENUM;
    }

    TextureFormat GetTextureFormat(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8_UNORM:                  return TextureFormat::R8;
            case VK_FORMAT_R8_SNORM:                  return TextureFormat::R8_SNORM;
            case VK_FORMAT_R8_UINT:                   return TextureFormat::R8UI;
            case VK_FORMAT_R8_SINT:                   return TextureFormat::R8I;
            case VK_FORMAT_S8_UINT:                   return TextureFormat::Stencil8;
            case VK_FORMAT_R16_SFLOAT:                return TextureFormat::R16F;
            case VK_FORMAT_R16_UINT:                  return TextureFormat::R16UI;
            case VK_FORMAT_R16_SINT:                  return TextureFormat::R16I;
            case VK_FORMAT_R8G8_UNORM:                return TextureFormat::RG8;
            case VK_FORMAT_R8G8_SNORM:                return TextureFormat::RG8_SNORM;
            case VK_FORMAT_R8G8_UINT:                 return TextureFormat::RG8UI;
            case VK_FORMAT_R8G8_SINT:                 return TextureFormat::RG8I;
            case VK_FORMAT_R5G6B5_UNORM_PACK16:       return TextureFormat::RGB565;
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:     return TextureFormat::RGB5A1;
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:     return TextureFormat::RGBA4;
            case VK_FORMAT_D16_UNORM:                 return TextureFormat::Depth16;
            case VK_FORMAT_R8G8B8_UNORM:              return TextureFormat::RGB8;
            case VK_FORMAT_R8G8B8_SRGB:               return TextureFormat::RGB8_SRGB;
            case VK_FORMAT_R8G8B8_SNORM:              return TextureFormat::RGB8_SNORM;
            case VK_FORMAT_R8G8B8_UINT:               return TextureFormat::RGB8UI;
            case VK_FORMAT_R8G8B8_SINT:               return TextureFormat::RGB8I;
            case VK_FORMAT_R32_SFLOAT:                return TextureFormat::R32F;
            case VK_FORMAT_R32_UINT:                  return TextureFormat::R32UI;
            case VK_FORMAT_R32_SINT:                  return TextureFormat::R32I;
            case VK_FORMAT_R16G16_SFLOAT:             return TextureFormat::RG16F;
            case VK_FORMAT_R16G16_UINT:               return TextureFormat::RG16UI;
            case VK_FORMAT_R16G16_SINT:               return TextureFormat::RG16I;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:   return TextureFormat::B10G11R11UF;
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:    return TextureFormat::RGB9E5;
            case VK_FORMAT_R8G8B8A8_UNORM:            return TextureFormat::RGBA8;
            case VK_FORMAT_R8G8B8A8_SRGB:             return TextureFormat::RGBA8_SRGB;
            case VK_FORMAT_B8G8R8A8_SRGB:             return TextureFormat::BGRA8_SRGB;
            case VK_FORMAT_R8G8B8A8_SNORM:            return TextureFormat::RGBA8_SNORM;
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:  return TextureFormat::RGB10A2;
            case VK_FORMAT_R8G8B8A8_UINT:             return TextureFormat::RGBA8UI;
            case VK_FORMAT_R8G8B8A8_SINT:             return TextureFormat::RGBA8I;
            case VK_FORMAT_D32_SFLOAT:                return TextureFormat::Depth32F;
            case VK_FORMAT_D24_UNORM_S8_UINT:         return TextureFormat::Depth24_Stencil8;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:        return TextureFormat::Depth32F_Stencil8;
            case VK_FORMAT_R16G16B16_SFLOAT:          return TextureFormat::RGB16F;
            case VK_FORMAT_R16G16B16_UINT:            return TextureFormat::RGB16UI;
            case VK_FORMAT_R16G16B16_SINT:            return TextureFormat::RGB16I;
            case VK_FORMAT_R32G32_SFLOAT:             return TextureFormat::RG32F;
            case VK_FORMAT_R32G32_UINT:               return TextureFormat::RG32UI;
            case VK_FORMAT_R32G32_SINT:               return TextureFormat::RG32I;
            case VK_FORMAT_R16G16B16A16_UNORM:        return TextureFormat::RGBA16;
            case VK_FORMAT_R16G16B16A16_SFLOAT:       return TextureFormat::RGBA16F;
            case VK_FORMAT_R16G16B16A16_UINT:         return TextureFormat::RGBA16UI;
            case VK_FORMAT_R16G16B16A16_SINT:         return TextureFormat::RGBA16I;
            case VK_FORMAT_R32G32B32_SFLOAT:          return TextureFormat::RGB32F;
            case VK_FORMAT_R32G32B32_UINT:            return TextureFormat::RGB32UI;
            case VK_FORMAT_R32G32B32_SINT:            return TextureFormat::RGB32I;
            case VK_FORMAT_R32G32B32A32_SFLOAT:       return TextureFormat::RGBA32F;
            case VK_FORMAT_R32G32B32A32_UINT:         return TextureFormat::RGBA32UI;
            case VK_FORMAT_R32G32B32A32_SINT:         return TextureFormat::RGBA32I;
            case VK_FORMAT_R64G64B64A64_UINT:         return TextureFormat::RGBA64UI;
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:       return TextureFormat::BC1_RGB;
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK:        return TextureFormat::BC1_SRGB;
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:      return TextureFormat::BC1_RGBA;
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:       return TextureFormat::BC1_SRGBA;
            case VK_FORMAT_BC2_UNORM_BLOCK:           return TextureFormat::BC2_RGBA;
            case VK_FORMAT_BC2_SRGB_BLOCK:            return TextureFormat::BC2_SRGBA;
            case VK_FORMAT_BC3_UNORM_BLOCK:           return TextureFormat::BC3_RGBA;
            case VK_FORMAT_BC3_SRGB_BLOCK:            return TextureFormat::BC3_SRGBA;
            case VK_FORMAT_BC4_UNORM_BLOCK:           return TextureFormat::BC4;
            case VK_FORMAT_BC6H_UFLOAT_BLOCK:         return TextureFormat::BC6H_RGBUF;
            case VK_FORMAT_BC6H_SFLOAT_BLOCK:         return TextureFormat::BC6H_RGBF;
            case VK_FORMAT_BC7_UNORM_BLOCK:           return TextureFormat::BC7_UNORM;

            default:
                PK_THROW_ERROR("Unsupported format conversion");
        }
    }

    VkImageAspectFlagBits GetFormatAspect(VkFormat format)
    {
        if (IsDepthStencilFormat(format))
        {
            return (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        }

        if (IsDepthFormat(format))
        {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    uint32_t ExpandVkRange16(uint32_t v)
    {
        return v >= 0x7FFF ? VK_REMAINING_ARRAY_LAYERS : v;
    }


    bool IsDepthFormat(VkFormat format)
    {
        return format == VK_FORMAT_D16_UNORM ||
            format == VK_FORMAT_D16_UNORM_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT ||
            format == VK_FORMAT_D32_SFLOAT ||
            format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    bool IsDepthStencilFormat(VkFormat format)
    {
        return  format == VK_FORMAT_D16_UNORM_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT ||
            format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    VkComponentMapping GetSwizzle(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_S8_UINT:
            case VK_FORMAT_R16_SFLOAT:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R32_SFLOAT:
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            case VK_FORMAT_BC4_UNORM_BLOCK:
                return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO };

            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_R32G32B32_SFLOAT:
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SINT:
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
                return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO };

            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            case VK_FORMAT_BC3_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO };
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
                return { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };

            case VK_FORMAT_B8G8R8A8_SRGB:
                return { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

            default:
                PK_THROW_ERROR("Swizzle conversion for supplied format is not defined!");
        }
    }

    VkImageViewType GetViewType(SamplerType samplerType)
    {
        switch (samplerType)
        {
            case SamplerType::Sampler2D: return VK_IMAGE_VIEW_TYPE_2D;
            case SamplerType::Sampler2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case SamplerType::Sampler3D: return VK_IMAGE_VIEW_TYPE_3D;
            case SamplerType::Cubemap: return VK_IMAGE_VIEW_TYPE_CUBE;
            case SamplerType::CubemapArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }

        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }

    VkImageLayout GetImageLayout(TextureUsage usage)
    {
        if ((usage & (TextureUsage::RTDepth | TextureUsage::RTColor | TextureUsage::Storage)) != 0)
        {
            return VK_IMAGE_LAYOUT_GENERAL;
        }

        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkAttachmentLoadOp GetLoadOp(LoadOp loadOp)
    {
        switch (loadOp)
        {
            case Structs::LoadOp::Keep: return  VK_ATTACHMENT_LOAD_OP_LOAD;
            case Structs::LoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case Structs::LoadOp::Discard: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
    }

    VkAttachmentLoadOp GetLoadOp(VkImageLayout layout, Rendering::Structs::LoadOp loadOp)
    {
        return layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : GetLoadOp(loadOp);
    }

    VkAttachmentStoreOp GetStoreOp(StoreOp storeOp)
    {
        switch (storeOp)
        {
            case Structs::StoreOp::Store: return  VK_ATTACHMENT_STORE_OP_STORE;
            case Structs::StoreOp::Discard: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
    }

    VkCompareOp GetCompareOp(Comparison comparison)
    {
        switch (comparison)
        {
            case Structs::Comparison::Off: return VK_COMPARE_OP_ALWAYS;
            case Structs::Comparison::Never: return VK_COMPARE_OP_NEVER;
            case Structs::Comparison::Less: return VK_COMPARE_OP_LESS;
            case Structs::Comparison::Equal: return VK_COMPARE_OP_EQUAL;
            case Structs::Comparison::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case Structs::Comparison::Greater: return VK_COMPARE_OP_GREATER;
            case Structs::Comparison::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case Structs::Comparison::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case Structs::Comparison::Always: return VK_COMPARE_OP_ALWAYS;
        }

        return VK_COMPARE_OP_MAX_ENUM;
    }

    VkBorderColor GetBorderColor(BorderColor color)
    {
        switch (color)
        {
            case Structs::BorderColor::FloatClear: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            case Structs::BorderColor::IntClear: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
            case Structs::BorderColor::FloatBlack: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            case Structs::BorderColor::IntBlack: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            case Structs::BorderColor::FloatWhite: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            case Structs::BorderColor::IntWhite: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        }

        return VK_BORDER_COLOR_MAX_ENUM;
    }

    VkSamplerAddressMode GetSamplerAddressMode(WrapMode wrap)
    {
        switch (wrap)
        {
            case Structs::WrapMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case Structs::WrapMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case Structs::WrapMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case Structs::WrapMode::MirrorOnce: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            case Structs::WrapMode::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        }

        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }

    VkFilter GetFilterMode(FilterMode filter)
    {
        switch (filter)
        {
            case Structs::FilterMode::Point: return VK_FILTER_NEAREST;
            case Structs::FilterMode::Bilinear: return VK_FILTER_LINEAR;
            case Structs::FilterMode::Trilinear: return VK_FILTER_LINEAR;
            case Structs::FilterMode::Bicubic: return VK_FILTER_CUBIC_IMG;
        }

        return VK_FILTER_MAX_ENUM;
    }

    VkDescriptorType GetDescriptorType(ResourceType type)
    {
        switch (type)
        {
            case Structs::ResourceType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case Structs::ResourceType::SamplerTexture: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case Structs::ResourceType::Texture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case Structs::ResourceType::Image: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case Structs::ResourceType::ConstantBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case Structs::ResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case Structs::ResourceType::DynamicConstantBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case Structs::ResourceType::DynamicStorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case Structs::ResourceType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case Structs::ResourceType::AccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        }

        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }

    ResourceType GetResourceType(VkDescriptorType type, uint32_t count)
    {
        switch (type)
        {
            case VK_DESCRIPTOR_TYPE_SAMPLER: return Structs::ResourceType::Sampler;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return Structs::ResourceType::SamplerTexture;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return Structs::ResourceType::Texture;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return Structs::ResourceType::Image;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return Structs::ResourceType::ConstantBuffer;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return Structs::ResourceType::StorageBuffer;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return Structs::ResourceType::DynamicConstantBuffer;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return Structs::ResourceType::DynamicStorageBuffer;
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return Structs::ResourceType::InputAttachment;
            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return Structs::ResourceType::AccelerationStructure;
        }

        return Structs::ResourceType::Invalid;
    }

    VkShaderStageFlagBits GetShaderStage(ShaderStage stage)
    {
        switch (stage)
        {
            case Structs::ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case Structs::ShaderStage::TesselationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case Structs::ShaderStage::TesselationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case Structs::ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
            case Structs::ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case Structs::ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
            case Structs::ShaderStage::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case Structs::ShaderStage::RayMiss: return VK_SHADER_STAGE_MISS_BIT_KHR;
            case Structs::ShaderStage::RayClosestHit: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case Structs::ShaderStage::RayAnyHit: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case Structs::ShaderStage::RayIntersection: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        }

        return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }

    VkPipelineBindPoint GetPipelineBindPoint(ShaderType type)
    {
        switch (type)
        {
            case Structs::ShaderType::Graphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
            case Structs::ShaderType::Compute: return VK_PIPELINE_BIND_POINT_COMPUTE;
            case Structs::ShaderType::RayTracing: return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        }

        return VK_PIPELINE_BIND_POINT_MAX_ENUM;
    }

    VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples)
    {
        uint32_t bit = VK_SAMPLE_COUNT_64_BIT;

        while ((samples & bit) == 0 && bit > VK_SAMPLE_COUNT_1_BIT)
        {
            bit >>= 1;
        }

        return (VkSampleCountFlagBits)bit;
    }

    VkVertexInputRate GetInputRate(InputRate inputRate)
    {
        switch (inputRate)
        {
            case InputRate::PerVertex: return VK_VERTEX_INPUT_RATE_VERTEX;
            case InputRate::PerInstance: return VK_VERTEX_INPUT_RATE_INSTANCE;
        }

        return VK_VERTEX_INPUT_RATE_MAX_ENUM;
    }

    VkShaderStageFlagBits GetShaderStageFlags(uint32_t pkStageFlags)
    {
        uint32_t flags = 0;

        if ((pkStageFlags & (1 << (int)ShaderStage::Vertex)) != 0)
        {
            flags |= VK_SHADER_STAGE_VERTEX_BIT;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::TesselationControl)) != 0)
        {
            flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::TesselationEvaluation)) != 0)
        {
            flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::Geometry)) != 0)
        {
            flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::Fragment)) != 0)
        {
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::Compute)) != 0)
        {
            flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::RayGeneration)) != 0)
        {
            flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::RayMiss)) != 0)
        {
            flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::RayClosestHit)) != 0)
        {
            flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::RayAnyHit)) != 0)
        {
            flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        }

        if ((pkStageFlags & (1 << (int)ShaderStage::RayIntersection)) != 0)
        {
            flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        }

        return (VkShaderStageFlagBits)flags;
    }

    VkPolygonMode GetPolygonMode(PolygonMode mode)
    {
        switch (mode)
        {
            case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
            case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
            case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
        }

        return VK_POLYGON_MODE_MAX_ENUM;
    }

    VkBlendOp GetBlendOp(BlendOp op)
    {
        switch (op)
        {
            case BlendOp::None: return VK_BLEND_OP_ADD;
            case BlendOp::Add: return VK_BLEND_OP_ADD;
            case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
            case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
            case BlendOp::Min: return VK_BLEND_OP_MIN;
            case BlendOp::Max: return VK_BLEND_OP_MAX;
        }

        return VK_BLEND_OP_MAX_ENUM;;
    }

    VkBlendFactor GetBlendFactor(BlendFactor factor, VkBlendFactor fallback)
    {
        switch (factor)
        {
            case BlendFactor::None: return fallback;
            case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
            case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
            case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
            case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
            case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
            case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
            case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case BlendFactor::ConstColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
            case BlendFactor::OneMinusConstColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            case BlendFactor::ConstAlpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
            case BlendFactor::OneMinusConstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        }

        return VK_BLEND_FACTOR_MAX_ENUM;
    }

    VkLogicOp GetLogicOp(Rendering::Structs::LogicOp op)
    {
        switch (op)
        {
            case LogicOp::Clear: return VK_LOGIC_OP_CLEAR;
            case LogicOp::And: return VK_LOGIC_OP_AND;
            case LogicOp::AndReverse: return VK_LOGIC_OP_AND_REVERSE;
            case LogicOp::Copy: return VK_LOGIC_OP_COPY;
            case LogicOp::AndInverted: return VK_LOGIC_OP_AND_INVERTED;
            case LogicOp::None: return VK_LOGIC_OP_NO_OP;
            case LogicOp::XOR: return VK_LOGIC_OP_XOR;
            case LogicOp::OR: return VK_LOGIC_OP_OR;
            case LogicOp::NOR: return VK_LOGIC_OP_NOR;
            case LogicOp::Equal: return VK_LOGIC_OP_EQUIVALENT;
            case LogicOp::Invert: return VK_LOGIC_OP_INVERT;
            case LogicOp::OrReverse: return VK_LOGIC_OP_OR_REVERSE;
            case LogicOp::CopyInverted: return VK_LOGIC_OP_COPY_INVERTED;
            case LogicOp::OrInverted: return VK_LOGIC_OP_OR_INVERTED;
            case LogicOp::NAND: return VK_LOGIC_OP_NAND;
            case LogicOp::Set: return VK_LOGIC_OP_SET;
        }
        return VK_LOGIC_OP_MAX_ENUM;
    }

    VkCullModeFlagBits GetCullMode(CullMode op)
    {
        switch (op)
        {
            case CullMode::Off: return VK_CULL_MODE_NONE;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        }

        return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
    }

    VkConservativeRasterizationModeEXT GetRasterMode(Rendering::Structs::RasterMode mode)
    {
        switch (mode)
        {
            case RasterMode::Default: return VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
            case RasterMode::OverEstimate: return VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
            case RasterMode::UnderEstimate: return VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT;
        }

        return VK_CONSERVATIVE_RASTERIZATION_MODE_MAX_ENUM_EXT;
    }

    VkPrimitiveTopology GetTopology(Rendering::Structs::Topology topology)
    {
        switch (topology)
        {
            case Topology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case Topology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case Topology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case Topology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case Topology::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            case Topology::LineListWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
            case Topology::LineStripWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
            case Topology::TriangleListWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
            case Topology::TriangleStripWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
            case Topology::PatchList: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        }

        return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }

    VkFrontFace GetFrontFace(FrontFace face)
    {
        switch (face)
        {
            case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
            case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        return VK_FRONT_FACE_MAX_ENUM;
    }

    VkPipelineStageFlags GetPipelineStageFlags(VkShaderStageFlags flags)
    {
        VkPipelineStageFlags outflags = 0u;

        if (flags & VK_SHADER_STAGE_VERTEX_BIT)
        {
            outflags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }

        if (flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
        {
            outflags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        }

        if (flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
        {
            outflags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
        }

        if (flags & VK_SHADER_STAGE_GEOMETRY_BIT)
        {
            outflags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        }

        if (flags & VK_SHADER_STAGE_FRAGMENT_BIT)
        {
            outflags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (flags & VK_SHADER_STAGE_COMPUTE_BIT)
        {
            outflags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }

        if (flags &
            (VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                VK_SHADER_STAGE_MISS_BIT_KHR |
                VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
                VK_SHADER_STAGE_CALLABLE_BIT_KHR))
        {
            outflags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        }

        return outflags;
    }

    VkRayTracingShaderGroupTypeKHR GetRayTracingStageGroupType(Rendering::Structs::ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::RayGeneration:
            case ShaderStage::RayMiss: return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            case ShaderStage::RayClosestHit:
            case ShaderStage::RayAnyHit: return VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            case ShaderStage::RayIntersection: return VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
        }

        return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    }

    bool IsReadAccess(VkAccessFlags flags)
    {
        const VkAccessFlags readMask =
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_UNIFORM_READ_BIT |
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_TRANSFER_READ_BIT |
            VK_ACCESS_HOST_READ_BIT |
            VK_ACCESS_MEMORY_READ_BIT |
            VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
            VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
            VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
            VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |
            VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR |
            VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV;

        return (flags & readMask) != 0u;
    }

    bool IsWriteAccess(VkAccessFlags flags)
    {
        const VkAccessFlags writeMask =
            VK_ACCESS_SHADER_WRITE_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_TRANSFER_WRITE_BIT |
            VK_ACCESS_HOST_WRITE_BIT |
            VK_ACCESS_MEMORY_WRITE_BIT |
            VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
            VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
            VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV;

        return (flags & writeMask) != 0u;
    }
}