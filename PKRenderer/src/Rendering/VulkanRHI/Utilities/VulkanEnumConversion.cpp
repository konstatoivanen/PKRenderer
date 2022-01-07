#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::EnumConvert
{
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
            case TextureFormat::R8:                         return VK_FORMAT_R8_UNORM;
            case TextureFormat::R8_SNORM:                   return VK_FORMAT_R8_SNORM;
            case TextureFormat::R8UI:                       return VK_FORMAT_R8_UINT;
            case TextureFormat::R8I:                        return VK_FORMAT_R8_SINT;
            case TextureFormat::Stencil8:                   return VK_FORMAT_S8_UINT;
            case TextureFormat::R16F:                       return VK_FORMAT_R16_SFLOAT;
            case TextureFormat::R16UI:                      return VK_FORMAT_R16_UINT;
            case TextureFormat::R16I:                       return VK_FORMAT_R16_SINT;
            case TextureFormat::RG8:                        return VK_FORMAT_R8G8_UNORM;
            case TextureFormat::RG8_SNORM:                  return VK_FORMAT_R8G8_SNORM;
            case TextureFormat::RG8UI:                      return VK_FORMAT_R8G8_UINT;
            case TextureFormat::RG8I:                       return VK_FORMAT_R8G8_SINT;
            case TextureFormat::RGB565:                     return VK_FORMAT_R5G6B5_UNORM_PACK16;
            case TextureFormat::RGB5A1:                    return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
            case TextureFormat::RGBA4:                      return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
            case TextureFormat::Depth16:                    return VK_FORMAT_D16_UNORM;
            case TextureFormat::RGB8:                       return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::RGB8_SRGB:                      return VK_FORMAT_R8G8B8_SRGB;
            case TextureFormat::RGB8_SNORM:                 return VK_FORMAT_R8G8B8_SNORM;
            case TextureFormat::RGB8UI:                     return VK_FORMAT_R8G8B8_UINT;
            case TextureFormat::RGB8I:                      return VK_FORMAT_R8G8B8_SINT;
            case TextureFormat::R32F:                       return VK_FORMAT_R32_SFLOAT;
            case TextureFormat::R32UI:                      return VK_FORMAT_R32_UINT;
            case TextureFormat::R32I:                       return VK_FORMAT_R32_SINT;
            case TextureFormat::RG16F:                      return VK_FORMAT_R16G16_SFLOAT;
            case TextureFormat::RG16UI:                     return VK_FORMAT_R16G16_UINT;
            case TextureFormat::RG16I:                      return VK_FORMAT_R16G16_SINT;
            case TextureFormat::R11FG11FB10F:             return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case TextureFormat::RGB9E5:                    return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
            case TextureFormat::RGBA8:                      return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_SRGB:                   return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::BGRA8_SRGB:                   return VK_FORMAT_B8G8R8A8_SRGB;
            case TextureFormat::RGBA8_SNORM:                return VK_FORMAT_R8G8B8A8_SNORM;
            case TextureFormat::RGB10A2:                   return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case TextureFormat::RGBA8UI:                    return VK_FORMAT_R8G8B8A8_UINT;
            case TextureFormat::RGBA8I:                     return VK_FORMAT_R8G8B8A8_SINT;
            case TextureFormat::Depth32F:                   return VK_FORMAT_D32_SFLOAT;
            case TextureFormat::Depth24_Stencil8:           return VK_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::Depth32F_Stencil8:          return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case TextureFormat::RGB16F:                     return VK_FORMAT_R16G16B16_SFLOAT;
            case TextureFormat::RGB16UI:                    return VK_FORMAT_R16G16B16_UINT;
            case TextureFormat::RGB16I:                     return VK_FORMAT_R16G16B16_SINT;
            case TextureFormat::RG32F:                      return VK_FORMAT_R32G32_SFLOAT;
            case TextureFormat::RG32UI:                     return VK_FORMAT_R32G32_UINT;
            case TextureFormat::RG32I:                      return VK_FORMAT_R32G32_SINT;
            case TextureFormat::RGBA16:                     return VK_FORMAT_R16G16B16A16_UNORM;;
            case TextureFormat::RGBA16F:                    return VK_FORMAT_R16G16B16A16_SFLOAT;
            case TextureFormat::RGBA16UI:                   return VK_FORMAT_R16G16B16A16_UINT;
            case TextureFormat::RGBA16I:                    return VK_FORMAT_R16G16B16A16_SINT;
            case TextureFormat::RGB32F:                     return VK_FORMAT_R32G32B32_SFLOAT;
            case TextureFormat::RGB32UI:                    return VK_FORMAT_R32G32B32_UINT;
            case TextureFormat::RGB32I:                     return VK_FORMAT_R32G32B32_SINT;
            case TextureFormat::RGBA32F:                    return VK_FORMAT_R32G32B32A32_SFLOAT;
            case TextureFormat::RGBA32UI:                   return VK_FORMAT_R32G32B32A32_UINT;
            case TextureFormat::RGBA32I:                    return VK_FORMAT_R32G32B32A32_SINT;
            case TextureFormat::DXT4:                       return VK_FORMAT_BC4_UNORM_BLOCK;
            case TextureFormat::DXT1_RGB:                   return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            case TextureFormat::DXT1_SRGB:                  return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            case TextureFormat::DXT1_RGBA:                  return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case TextureFormat::DXT1_SRGBA:                 return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case TextureFormat::DXT3_RGBA:                  return VK_FORMAT_BC2_UNORM_BLOCK;
            case TextureFormat::DXT3_SRGBA:                 return VK_FORMAT_BC2_SRGB_BLOCK;
            case TextureFormat::DXT5_RGBA:                  return VK_FORMAT_BC3_UNORM_BLOCK;
            case TextureFormat::DXT5_SRGBA:                 return VK_FORMAT_BC3_SRGB_BLOCK;
            case TextureFormat::RGBA_ASTC_4x4:              return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_5x4:              return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_5x5:              return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_6x5:              return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_6x6:              return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_8x5:              return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_8x6:              return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_8x8:              return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_10x5:             return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_10x6:             return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_10x8:             return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_10x10:            return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_12x10:            return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
            case TextureFormat::RGBA_ASTC_12x12:            return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_4x4:      return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_5x4:      return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_5x5:      return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_6x5:      return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_6x6:      return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_8x5:      return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_8x6:      return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_8x8:      return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_10x5:     return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_10x6:     return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_10x8:     return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_10x10:    return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_12x10:    return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
            case TextureFormat::SRGB8_ALPHA8_ASTC_12x12:    return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
            case TextureFormat::ETC2_RGB8:                  return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
            case TextureFormat::ETC2_SRGB8:                 return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
            case TextureFormat::ETC2_RGB8_A1:               return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
            case TextureFormat::ETC2_SRGB8_A1:              return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
            case TextureFormat::ETC2_RGBA8:                 return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
            case TextureFormat::ETC2_SRGBA8:                return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
            case TextureFormat::EAC_R11:                    return VK_FORMAT_EAC_R11_UNORM_BLOCK;
            case TextureFormat::EAC_R11_SIGNED:             return VK_FORMAT_EAC_R11_SNORM_BLOCK;
            case TextureFormat::EAC_RG11:                   return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
            case TextureFormat::EAC_RG11_SIGNED:            return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;

            default:
                return VK_FORMAT_UNDEFINED;
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
           case VK_FORMAT_B10G11R11_UFLOAT_PACK32:   return TextureFormat::R11FG11FB10F;
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
           case VK_FORMAT_BC4_UNORM_BLOCK:           return TextureFormat::DXT4;
           case VK_FORMAT_BC1_RGB_UNORM_BLOCK:       return TextureFormat::DXT1_RGB;
           case VK_FORMAT_BC1_RGB_SRGB_BLOCK:        return TextureFormat::DXT1_SRGB;
           case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:      return TextureFormat::DXT1_RGBA;
           case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:       return TextureFormat::DXT1_SRGBA;
           case VK_FORMAT_BC2_UNORM_BLOCK:           return TextureFormat::DXT3_RGBA;
           case VK_FORMAT_BC2_SRGB_BLOCK:            return TextureFormat::DXT3_SRGBA;
           case VK_FORMAT_BC3_UNORM_BLOCK:           return TextureFormat::DXT5_RGBA;
           case VK_FORMAT_BC3_SRGB_BLOCK:            return TextureFormat::DXT5_SRGBA;
           case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_4x4;
           case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_5x4;
           case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_5x5;
           case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_6x5;
           case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_6x6;
           case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_8x5;
           case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_8x6;
           case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:      return TextureFormat::RGBA_ASTC_8x8;
           case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:     return TextureFormat::RGBA_ASTC_10x5;
           case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:     return TextureFormat::RGBA_ASTC_10x6;
           case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:     return TextureFormat::RGBA_ASTC_10x8;
           case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:    return TextureFormat::RGBA_ASTC_10x10;
           case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:    return TextureFormat::RGBA_ASTC_12x10;
           case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:    return TextureFormat::RGBA_ASTC_12x12;
           case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_4x4;
           case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_5x4;
           case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_5x5;
           case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_6x5;
           case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_6x6;
           case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_8x5;
           case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_8x6;
           case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:       return TextureFormat::SRGB8_ALPHA8_ASTC_8x8;
           case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:      return TextureFormat::SRGB8_ALPHA8_ASTC_10x5;
           case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:      return TextureFormat::SRGB8_ALPHA8_ASTC_10x6;
           case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:      return TextureFormat::SRGB8_ALPHA8_ASTC_10x8;
           case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:     return TextureFormat::SRGB8_ALPHA8_ASTC_10x10;
           case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:     return TextureFormat::SRGB8_ALPHA8_ASTC_12x10;
           case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:     return TextureFormat::SRGB8_ALPHA8_ASTC_12x12;
           case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:   return TextureFormat::ETC2_RGB8;
           case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:    return TextureFormat::ETC2_SRGB8;
           case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return TextureFormat::ETC2_RGB8_A1;
           case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:  return TextureFormat::ETC2_SRGB8_A1;
           case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return TextureFormat::ETC2_RGBA8;
           case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:  return TextureFormat::ETC2_SRGBA8;
           case VK_FORMAT_EAC_R11_UNORM_BLOCK:       return TextureFormat::EAC_R11;
           case VK_FORMAT_EAC_R11_SNORM_BLOCK:       return TextureFormat::EAC_R11_SIGNED;
           case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:    return TextureFormat::EAC_RG11;
           case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:    return TextureFormat::EAC_RG11_SIGNED;

            default:
                PK_THROW_ERROR("Unsupported format conversion");
        }
    }

    bool IsDepthFormat(VkFormat format)
    {
        return format == VK_FORMAT_D16_UNORM ||
               format == VK_FORMAT_D16_UNORM_S8_UINT ||
               format == VK_FORMAT_D24_UNORM_S8_UINT ||
               format == VK_FORMAT_D32_SFLOAT ||
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
                return { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                return { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
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
             case SamplerType::Sampler2D:
                 return VK_IMAGE_VIEW_TYPE_2D;
             case SamplerType::Sampler2DArray:
                 return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
             case SamplerType::Sampler3D:
                 return VK_IMAGE_VIEW_TYPE_3D;
             case SamplerType::Cubemap:
                 return VK_IMAGE_VIEW_TYPE_CUBE;
             case SamplerType::CubemapArray:
                 return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }

        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }

    VkImageLayout GetImageLayout(TextureUsage usage, bool useOptimized)
    {
        if (useOptimized)
        {
            if ((usage & (TextureUsage::RTDepth | TextureUsage::RTColor)) == (TextureUsage::RTDepth | TextureUsage::RTColor))
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            if ((usage & TextureUsage::RTDepth) != 0)
            {
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            }

            if ((usage & TextureUsage::RTColor) != 0)
            {
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
        }

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
        }

        return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }

    VkPipelineBindPoint GetPipelineBindPoint(ShaderType type)
    {
        switch (type)
        {
            case Structs::ShaderType::Graphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
            case Structs::ShaderType::Compute: return VK_PIPELINE_BIND_POINT_COMPUTE;
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

    VkFrontFace GetFrontFace(FrontFace face)
    {
        switch (face)
        {
            case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
            case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        return VK_FRONT_FACE_MAX_ENUM;
    }
    
    VkPipelineStageFlagBits GetPipelineStageFlags(MemoryAccessFlags flags)
    {
        /*
            @TODO add support for these later

            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
            VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000,
        */

        uint32_t outflags = 0u;

        if ((flags & MemoryAccessFlags::StageIndirect) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        }

        if ((flags & MemoryAccessFlags::StageVertexInput) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }

        if ((flags & MemoryAccessFlags::StageVertex) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }

        if ((flags & MemoryAccessFlags::StageGeometry) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        }

        if ((flags & MemoryAccessFlags::StageFragment) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if ((flags & MemoryAccessFlags::StageCompute) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }

        if ((flags & MemoryAccessFlags::StageDepthStencil) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }

        if ((flags & MemoryAccessFlags::StageDepthStencilOut) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }

        if ((flags & MemoryAccessFlags::StageColorOut) != 0)
        {
            outflags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        if (flags == 0)
        {
            outflags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }

        return (VkPipelineStageFlagBits)outflags;
    }

    VkAccessFlagBits GetAccessFlags(MemoryAccessFlags flags)
    {
        /*
            @TODO Add support for these later

            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT = 0x00000010,
            VK_ACCESS_TRANSFER_READ_BIT = 0x00000800,
            VK_ACCESS_TRANSFER_WRITE_BIT = 0x00001000,
            VK_ACCESS_HOST_READ_BIT = 0x00002000,
            VK_ACCESS_HOST_WRITE_BIT = 0x00004000,
            VK_ACCESS_MEMORY_READ_BIT = 0x00008000,
            VK_ACCESS_MEMORY_WRITE_BIT = 0x00010000,
        */

        uint32_t outflags = 0u;

        if ((flags & MemoryAccessFlags::ReadShader) != 0)
        {
            outflags |= VK_ACCESS_SHADER_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::ReadUniform) != 0)
        {
            outflags |= VK_ACCESS_UNIFORM_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::ReadVertex) != 0)
        {
            outflags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::ReadIndex) != 0)
        {
            outflags |= VK_ACCESS_INDEX_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::ReadIndirect) != 0)
        {
            outflags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::ReadRTColor) != 0)
        {
            outflags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::ReadRTDepth) != 0)
        {
            outflags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        }

        if ((flags & MemoryAccessFlags::WriteShader) != 0)
        {
            outflags |= VK_ACCESS_SHADER_WRITE_BIT;
        }

        if ((flags & MemoryAccessFlags::WriteRTColor) != 0)
        {
            outflags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        if ((flags & MemoryAccessFlags::WriteRTDepth) != 0)
        {
            outflags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        return (VkAccessFlagBits)outflags;
    }
}