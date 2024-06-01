#pragma once
#include <gfx.h>
#include <bitset>
#include "Core/Input/InputDevice.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct InputDeviceGLFW : public InputDevice
    {
        InputDeviceGLFW(RHIWindow* window);

        bool GetKeyDown(InputKey key) const final;
        bool GetKeyUp(InputKey key) const final;
        bool GetKey(InputKey key) const final;
        inline float2 GetCursorPosition() const final { return m_cursorPosition; }
        inline float2 GetCursorDelta() const final { return m_cursorDelta; }
        inline float2 GetCursorScroll() const final { return m_scrollInput; }
        float2 GetCursorPositionNormalized() const final;
        void UpdateBegin() final;
        void UpdateEnd() final;
        void OnKeyInput(int key, int scancode, int action, int mods);
        void OnMouseButtonInput(int button, int action, int mods);
        void OnScrollInput(double xOffset, double yOffset);

        RHIWindow* m_window;
        std::bitset<(int)InputKey::Count> m_keyStatesCurrent;
        std::bitset<(int)InputKey::Count> m_keyStatesPrevious;
        InputKey m_keymap[GLFW_KEY_LAST + 1]{};
        float2 m_cursorPositionPrev = PK_FLOAT2_ZERO;
        float2 m_cursorPosition = PK_FLOAT2_ZERO;
        float2 m_cursorPositionNormalized = PK_FLOAT2_ZERO;
        float2 m_cursorDelta = PK_FLOAT2_ZERO;
        float2 m_scrollInputRaw = PK_FLOAT2_ONE;
        float2 m_scrollInput = PK_FLOAT2_ZERO;
    };
}