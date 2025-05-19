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
        isValid &= YAML::Read<VSyncMode>(node, "WindowDescriptor.vsync", &rhs->vsync);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.visible", &rhs->visible);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.resizable", &rhs->resizable);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.floating", &rhs->floating);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.dpiScaling", &rhs->dpiScaling);
        isValid &= YAML::Read<bool>(node, "WindowDescriptor.autoActivate", &rhs->autoActivate);
        return isValid;
    }

    PK_YAML_DECLARE_READ_MEMBER(WindowDescriptor)
}
