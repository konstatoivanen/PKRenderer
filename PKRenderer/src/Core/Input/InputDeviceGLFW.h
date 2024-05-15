#pragma once
#include <gfx.h>
#include <bitset>
#include "Core/Input/InputDevice.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Core::Input
{
    struct InputDeviceGLFW : public InputDevice
    {
        InputDeviceGLFW(Rendering::RHI::Objects::Window* window);

        bool GetKeyDown(InputKey key) const final;
        bool GetKeyUp(InputKey key) const final;
        bool GetKey(InputKey key) const final;
        inline Math::float2 GetCursorPosition() const final { return m_cursorPosition; }
        inline Math::float2 GetCursorDelta() const final { return m_cursorDelta; }
        inline Math::float2 GetCursorScroll() const final { return m_scrollInput; }
        Math::float2 GetCursorPositionNormalized() const final;
        void UpdateBegin() final;
        void UpdateEnd() final;
        void OnKeyInput(int key, int scancode, int action, int mods);
        void OnMouseButtonInput(int button, int action, int mods);
        void OnScrollInput(double xOffset, double yOffset);

        Rendering::RHI::Objects::Window* m_window;
        std::bitset<(int)InputKey::Count> m_keyStatesCurrent;
        std::bitset<(int)InputKey::Count> m_keyStatesPrevious;
        InputKey m_keymap[GLFW_KEY_LAST + 1]{};
        Math::float2 m_cursorPositionPrev = PK::Math::PK_FLOAT2_ZERO;
        Math::float2 m_cursorPosition = PK::Math::PK_FLOAT2_ZERO;
        Math::float2 m_cursorPositionNormalized = PK::Math::PK_FLOAT2_ZERO;
        Math::float2 m_cursorDelta = PK::Math::PK_FLOAT2_ZERO;
        Math::float2 m_scrollInputRaw = PK::Math::PK_FLOAT2_ONE;
        Math::float2 m_scrollInput = PK::Math::PK_FLOAT2_ZERO;
    };
}