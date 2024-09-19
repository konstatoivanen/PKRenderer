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

        const InputState* GetStatePtr() const final { return &m_state; }
        void UpdateBegin() final;
        void UpdateEnd() final;
        void OnKeyInput(int key, int scancode, int action, int mods);
        void OnMouseButtonInput(int button, int action, int mods);
        void OnScrollInput(double xOffset, double yOffset);

        RHIWindow* m_window;
        InputState m_state;
        InputKey m_keymap[GLFW_KEY_LAST + 1]{};
        float2 m_cursorPositionPrev = PK_FLOAT2_ZERO;
        float2 m_scrollInputRaw = PK_FLOAT2_ONE;
    };
}