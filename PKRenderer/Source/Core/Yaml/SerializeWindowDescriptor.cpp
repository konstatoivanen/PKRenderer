#include "PrecompiledHeader.h"
#include "Core/Rendering/Window.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    template<>
    bool Read<WindowDescriptor>(const ConstNode& node, WindowDescriptor* rhs)
    {
        bool isValid = true;
        isValid &= YAML::Read<FixedString64>(node, "WindowDescriptor.title", &rhs->title);
        isValid &= YAML::Read<FixedString256>(node, "WindowDescriptor.iconPath", &rhs->iconPath);
        isValid &= YAML::Read<int2>(node, "WindowDescriptor.position", &rhs->position);
        isValid &= YAML::Read<int2>(node, "WindowDescriptor.size", &rhs->size);
        isValid &= YAML::Read<int2>(node, "WindowDescriptor.sizemax", &rhs->sizemax);
        isValid &= YAML::Read<uint32_t>(node, "WindowDescriptor.swapchainImageCount", &rhs->swapchainImageCount);
        isValid &= YAML::Read<VSyncMode>(node, "WindowDescriptor.vsync", &rhs->vsync);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.visible", &rhs->visible);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.resizable", &rhs->resizable);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.floating", &rhs->floating);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.dpiScaling", &rhs->dpiScaling);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.autoActivate", &rhs->autoActivate);
        return isValid;
    }

    template<>
    void Write<WindowDescriptor>(Node& parent, const char* memberName, const WindowDescriptor* rhs)
    {
        auto node = parent[memberName];
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

    PK_YAML_DECLARE_READ_MEMBER(WindowDescriptor)
}
