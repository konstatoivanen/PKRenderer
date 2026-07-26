#include "PrecompiledHeader.h"
#include "Core/Rendering/Window.h"
#include "Core/Yaml/Serialize.h"

namespace PK::YAML
{
    template<>
    void Read<WindowDescriptor>(const ConstNode& node, WindowDescriptor* rhs)
    {
        YAML::Read<FixedString64>(node, "WindowDescriptor.title", &rhs->title);
        YAML::Read<FixedString256>(node, "WindowDescriptor.iconPath", &rhs->iconPath);
        YAML::Read<int2>(node, "WindowDescriptor.position", &rhs->position);
        YAML::Read<int2>(node, "WindowDescriptor.size", &rhs->size);
        YAML::Read<int2>(node, "WindowDescriptor.sizemax", &rhs->sizemax);
        YAML::Read<uint32_t>(node, "WindowDescriptor.swapchainImageCount", &rhs->swapchainImageCount);
        YAML::Read<VSyncMode>(node, "WindowDescriptor.vsync", &rhs->vsync);
        YAML::Read<bool>(node, "WindowDescriptor.visible", &rhs->visible);
        YAML::Read<bool>(node, "WindowDescriptor.resizable", &rhs->resizable);
        YAML::Read<bool>(node, "WindowDescriptor.floating", &rhs->floating);
        YAML::Read<bool>(node, "WindowDescriptor.dpiScaling", &rhs->dpiScaling);
        YAML::Read<bool>(node, "WindowDescriptor.autoActivate", &rhs->autoActivate);
    }

    template<>
    void Write<WindowDescriptor>(Node& node, const WindowDescriptor* rhs)
    {
        node |= ryml::MAP;
        YAML::Write<FixedString64>(node, "WindowDescriptor.title", &rhs->title);
        YAML::Write<FixedString256>(node, "WindowDescriptor.iconPath", &rhs->iconPath);
        YAML::Write<int2>(node, "WindowDescriptor.position", &rhs->position);
        YAML::Write<int2>(node, "WindowDescriptor.size", &rhs->size);
        YAML::Write<int2>(node, "WindowDescriptor.sizemax", &rhs->sizemax);
        YAML::Write<uint32_t>(node, "WindowDescriptor.swapchainImageCount", &rhs->swapchainImageCount);
        YAML::Write<VSyncMode>(node, "WindowDescriptor.vsync", &rhs->vsync);
        YAML::Write<bool>(node, "WindowDescriptor.visible", &rhs->visible);
        YAML::Write<bool>(node, "WindowDescriptor.resizable", &rhs->resizable);
        YAML::Write<bool>(node, "WindowDescriptor.floating", &rhs->floating);
        YAML::Write<bool>(node, "WindowDescriptor.dpiScaling", &rhs->dpiScaling);
        YAML::Write<bool>(node, "WindowDescriptor.autoActivate", &rhs->autoActivate);
    }
}
