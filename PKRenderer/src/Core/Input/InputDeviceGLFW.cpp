#include "PrecompiledHeader.h"
#include "Core/Input/InputSystem.h"
#include "InputDeviceGLFW.h"

namespace PK::Core::Input
{
    using namespace PK::Math;
    using namespace PK::Rendering::RHI;

    InputDeviceGLFW::InputDeviceGLFW(Window* window) : m_window(window)
    {
        m_keymap[GLFW_MOUSE_BUTTON_1] = InputKey::Mouse1;
        m_keymap[GLFW_MOUSE_BUTTON_2] = InputKey::Mouse2;
        m_keymap[GLFW_MOUSE_BUTTON_3] = InputKey::Mouse3;
        m_keymap[GLFW_MOUSE_BUTTON_4] = InputKey::Mouse4;
        m_keymap[GLFW_MOUSE_BUTTON_5] = InputKey::Mouse5;
        m_keymap[GLFW_MOUSE_BUTTON_6] = InputKey::Mouse6;
        m_keymap[GLFW_MOUSE_BUTTON_7] = InputKey::Mouse7;
        m_keymap[GLFW_MOUSE_BUTTON_8] = InputKey::Mouse8;
        m_keymap[GLFW_KEY_SPACE] = InputKey::Space;
        m_keymap[GLFW_KEY_APOSTROPHE] = InputKey::Apostrophe;
        m_keymap[GLFW_KEY_COMMA] = InputKey::Comma;
        m_keymap[GLFW_KEY_MINUS] = InputKey::Minus;
        m_keymap[GLFW_KEY_PERIOD] = InputKey::Period;
        m_keymap[GLFW_KEY_SLASH] = InputKey::Slash;
        m_keymap[GLFW_KEY_0] = InputKey::Alpha0;
        m_keymap[GLFW_KEY_1] = InputKey::Alpha1;
        m_keymap[GLFW_KEY_2] = InputKey::Alpha2;
        m_keymap[GLFW_KEY_3] = InputKey::Alpha3;
        m_keymap[GLFW_KEY_4] = InputKey::Alpha4;
        m_keymap[GLFW_KEY_5] = InputKey::Alpha5;
        m_keymap[GLFW_KEY_6] = InputKey::Alpha6;
        m_keymap[GLFW_KEY_7] = InputKey::Alpha7;
        m_keymap[GLFW_KEY_8] = InputKey::Alpha8;
        m_keymap[GLFW_KEY_9] = InputKey::Alpha9;
        m_keymap[GLFW_KEY_SEMICOLON] = InputKey::Semicolon;
        m_keymap[GLFW_KEY_EQUAL] = InputKey::Equal;
        m_keymap[GLFW_KEY_A] = InputKey::A;
        m_keymap[GLFW_KEY_B] = InputKey::B;
        m_keymap[GLFW_KEY_C] = InputKey::C;
        m_keymap[GLFW_KEY_D] = InputKey::D;
        m_keymap[GLFW_KEY_E] = InputKey::E;
        m_keymap[GLFW_KEY_F] = InputKey::F;
        m_keymap[GLFW_KEY_G] = InputKey::G;
        m_keymap[GLFW_KEY_H] = InputKey::H;
        m_keymap[GLFW_KEY_I] = InputKey::I;
        m_keymap[GLFW_KEY_J] = InputKey::J;
        m_keymap[GLFW_KEY_K] = InputKey::K;
        m_keymap[GLFW_KEY_L] = InputKey::L;
        m_keymap[GLFW_KEY_M] = InputKey::M;
        m_keymap[GLFW_KEY_N] = InputKey::N;
        m_keymap[GLFW_KEY_O] = InputKey::O;
        m_keymap[GLFW_KEY_P] = InputKey::P;
        m_keymap[GLFW_KEY_Q] = InputKey::Q;
        m_keymap[GLFW_KEY_R] = InputKey::R;
        m_keymap[GLFW_KEY_S] = InputKey::S;
        m_keymap[GLFW_KEY_T] = InputKey::T;
        m_keymap[GLFW_KEY_U] = InputKey::U;
        m_keymap[GLFW_KEY_V] = InputKey::V;
        m_keymap[GLFW_KEY_W] = InputKey::W;
        m_keymap[GLFW_KEY_X] = InputKey::X;
        m_keymap[GLFW_KEY_Y] = InputKey::Y;
        m_keymap[GLFW_KEY_Z] = InputKey::Z;
        m_keymap[GLFW_KEY_LEFT_BRACKET] = InputKey::LeftBracket;
        m_keymap[GLFW_KEY_BACKSLASH] = InputKey::BackSlash;
        m_keymap[GLFW_KEY_RIGHT_BRACKET] = InputKey::RightBracket;
        m_keymap[GLFW_KEY_GRAVE_ACCENT] = InputKey::GraveAccent;
        m_keymap[GLFW_KEY_WORLD_1] = InputKey::World1;
        m_keymap[GLFW_KEY_WORLD_2] = InputKey::World2;
        m_keymap[GLFW_KEY_ESCAPE] = InputKey::Escape;
        m_keymap[GLFW_KEY_ENTER] = InputKey::Enter;
        m_keymap[GLFW_KEY_TAB] = InputKey::Tab;
        m_keymap[GLFW_KEY_BACKSPACE] = InputKey::Backspace;
        m_keymap[GLFW_KEY_INSERT] = InputKey::Insert;
        m_keymap[GLFW_KEY_DELETE] = InputKey::Del;
        m_keymap[GLFW_KEY_RIGHT] = InputKey::Right;
        m_keymap[GLFW_KEY_LEFT] = InputKey::Left;
        m_keymap[GLFW_KEY_DOWN] = InputKey::Down;
        m_keymap[GLFW_KEY_UP] = InputKey::Up;
        m_keymap[GLFW_KEY_PAGE_UP] = InputKey::PageUp;
        m_keymap[GLFW_KEY_PAGE_DOWN] = InputKey::PageDown;
        m_keymap[GLFW_KEY_HOME] = InputKey::Home;
        m_keymap[GLFW_KEY_END] = InputKey::End;
        m_keymap[GLFW_KEY_CAPS_LOCK] = InputKey::CapsLock;
        m_keymap[GLFW_KEY_SCROLL_LOCK] = InputKey::ScrollLock;
        m_keymap[GLFW_KEY_NUM_LOCK] = InputKey::NumLock;
        m_keymap[GLFW_KEY_PRINT_SCREEN] = InputKey::PrintScreen;
        m_keymap[GLFW_KEY_PAUSE] = InputKey::Pause;
        m_keymap[GLFW_KEY_F1] = InputKey::F1;
        m_keymap[GLFW_KEY_F2] = InputKey::F2;
        m_keymap[GLFW_KEY_F3] = InputKey::F3;
        m_keymap[GLFW_KEY_F4] = InputKey::F4;
        m_keymap[GLFW_KEY_F5] = InputKey::F5;
        m_keymap[GLFW_KEY_F6] = InputKey::F6;
        m_keymap[GLFW_KEY_F7] = InputKey::F7;
        m_keymap[GLFW_KEY_F8] = InputKey::F8;
        m_keymap[GLFW_KEY_F9] = InputKey::F9;
        m_keymap[GLFW_KEY_F10] = InputKey::F10;
        m_keymap[GLFW_KEY_F11] = InputKey::F11;
        m_keymap[GLFW_KEY_F12] = InputKey::F12;
        m_keymap[GLFW_KEY_F13] = InputKey::F13;
        m_keymap[GLFW_KEY_F14] = InputKey::F14;
        m_keymap[GLFW_KEY_F15] = InputKey::F15;
        m_keymap[GLFW_KEY_F16] = InputKey::F16;
        m_keymap[GLFW_KEY_F17] = InputKey::F17;
        m_keymap[GLFW_KEY_F18] = InputKey::F18;
        m_keymap[GLFW_KEY_F19] = InputKey::F19;
        m_keymap[GLFW_KEY_F20] = InputKey::F20;
        m_keymap[GLFW_KEY_F21] = InputKey::F21;
        m_keymap[GLFW_KEY_F22] = InputKey::F22;
        m_keymap[GLFW_KEY_F23] = InputKey::F23;
        m_keymap[GLFW_KEY_F24] = InputKey::F24;
        m_keymap[GLFW_KEY_F25] = InputKey::F25;
        m_keymap[GLFW_KEY_KP_0] = InputKey::KeyPad0;
        m_keymap[GLFW_KEY_KP_1] = InputKey::KeyPad1;
        m_keymap[GLFW_KEY_KP_2] = InputKey::KeyPad2;
        m_keymap[GLFW_KEY_KP_3] = InputKey::KeyPad3;
        m_keymap[GLFW_KEY_KP_4] = InputKey::KeyPad4;
        m_keymap[GLFW_KEY_KP_5] = InputKey::KeyPad5;
        m_keymap[GLFW_KEY_KP_6] = InputKey::KeyPad6;
        m_keymap[GLFW_KEY_KP_7] = InputKey::KeyPad7;
        m_keymap[GLFW_KEY_KP_8] = InputKey::KeyPad8;
        m_keymap[GLFW_KEY_KP_9] = InputKey::KeyPad9;
        m_keymap[GLFW_KEY_KP_DECIMAL] = InputKey::KeyPadDecimal;
        m_keymap[GLFW_KEY_KP_DIVIDE] = InputKey::KeyPadDivide;
        m_keymap[GLFW_KEY_KP_MULTIPLY] = InputKey::KeyPadMultiply;
        m_keymap[GLFW_KEY_KP_SUBTRACT] = InputKey::KeyPadSubtract;
        m_keymap[GLFW_KEY_KP_ADD] = InputKey::KeyPadAdd;
        m_keymap[GLFW_KEY_KP_ENTER] = InputKey::KeyPadEnter;
        m_keymap[GLFW_KEY_KP_EQUAL] = InputKey::KeyPadEqual;
        m_keymap[GLFW_KEY_LEFT_SHIFT] = InputKey::LeftShift;
        m_keymap[GLFW_KEY_LEFT_CONTROL] = InputKey::LeftControl;
        m_keymap[GLFW_KEY_LEFT_ALT] = InputKey::LeftAlt;
        m_keymap[GLFW_KEY_LEFT_SUPER] = InputKey::LeftSuper;
        m_keymap[GLFW_KEY_RIGHT_SHIFT] = InputKey::RightShift;
        m_keymap[GLFW_KEY_RIGHT_CONTROL] = InputKey::RightControl;
        m_keymap[GLFW_KEY_RIGHT_ALT] = InputKey::RightAlt;
        m_keymap[GLFW_KEY_RIGHT_SUPER] = InputKey::RightSuper;
        m_keymap[GLFW_KEY_MENU] = InputKey::Menu;

        GLFWwindow* glfwWIndow = static_cast<GLFWwindow*>(m_window->GetNativeWindow());
        glfwSetKeyCallback(glfwWIndow, [](GLFWwindow* nativeWindow, int key, int scancode, int action, int mods) { InputSystem::Get()->GetDevice<InputDeviceGLFW>(nativeWindow)->OnKeyInput(key, scancode, action, mods); });
        // glfwSetCharCallback(glfwWIndow, [](GLFWwindow* nativeWindow, uint32_t keycode) { Input::Get()->GetDevice<InputDeviceGLFW>(nativeWindow)->OnCharInput(keycode); });
        glfwSetMouseButtonCallback(glfwWIndow, [](GLFWwindow* nativeWindow, int button, int action, int mods) { InputSystem::Get()->GetDevice<InputDeviceGLFW>(nativeWindow)->OnMouseButtonInput(button, action, mods); });
        glfwSetScrollCallback(glfwWIndow, [](GLFWwindow* nativeWindow, double xOffset, double yOffset) { InputSystem::Get()->GetDevice<InputDeviceGLFW>(nativeWindow)->OnScrollInput(xOffset, yOffset); });
        // glfwSetCursorPosCallback(glfwWIndow, [](GLFWwindow* nativeWindow, double xPos, double yPos) { Input::Get()->GetDevice<InputDeviceGLFW>(nativeWindow)->OnCursorInput(xPos, yPos); });
    }

    bool InputDeviceGLFW::GetKeyDown(InputKey key) const
    {
        return m_keyStatesCurrent[(int)key] && !m_keyStatesPrevious[(int)key];
    }

    bool InputDeviceGLFW::GetKeyUp(InputKey key) const
    {
        return !m_keyStatesCurrent[(int)key] && m_keyStatesCurrent[(int)key];
    }

    bool InputDeviceGLFW::GetKey(InputKey key) const
    {
        return m_keyStatesCurrent[(int)key];
    }

    float2 InputDeviceGLFW::GetCursorPositionNormalized() const
    {
        int w, h;
        auto glfwWindow = static_cast<GLFWwindow*>(m_window->GetNativeWindow());
        glfwGetWindowSize(glfwWindow, &w, &h);
        return { (float)m_cursorPosition.x / w, ((float)h - (float)m_cursorPosition.x) / h };
    }

    void InputDeviceGLFW::UpdateBegin()
    {
        double xpos, ypos;
        int w, h;
        auto glfwWindow = static_cast<GLFWwindow*>(m_window->GetNativeWindow());
        glfwGetCursorPos(glfwWindow, &xpos, &ypos);
        glfwGetWindowSize(glfwWindow, &w, &h);

        m_cursorPosition.x = (float)xpos;
        m_cursorPosition.y = (float)h - (float)ypos;
        m_cursorPositionNormalized.x = m_cursorPosition.x / w;
        m_cursorPositionNormalized.y = m_cursorPosition.y / h;
        m_cursorDelta = m_cursorPosition - m_cursorPositionPrev;
        m_cursorPositionPrev = m_cursorPosition;

        m_scrollInput = m_scrollInputRaw;
        m_scrollInputRaw = { 0, 0 };
    }

    void InputDeviceGLFW::UpdateEnd()
    {
        // Copy previous state to correctly detect releases
        m_keyStatesPrevious = m_keyStatesCurrent;
        m_keyStatesCurrent[(int)InputKey::MouseScrollUp] = false;
        m_keyStatesCurrent[(int)InputKey::MouseScrollDown] = false;
    }

    void InputDeviceGLFW::OnKeyInput(int key, int scancode, int action, int mods)
    {
        auto keyCode = m_keymap[key];

        if (keyCode != InputKey::None)
        {
            m_keyStatesCurrent[(int)keyCode] = action == GLFW_PRESS || action == GLFW_REPEAT;
        }
    }

    void InputDeviceGLFW::OnMouseButtonInput(int button, int action, int mods)
    {
        auto keyCode = m_keymap[button];

        if (keyCode != InputKey::None)
        {
            m_keyStatesCurrent[(int)keyCode] = action == GLFW_PRESS || action == GLFW_REPEAT;
        }
    }

    void InputDeviceGLFW::OnScrollInput(double scrollX, double scrollY)
    {
        m_keyStatesCurrent[(int)InputKey::MouseScrollUp] = scrollY > 0.5f;
        m_keyStatesCurrent[(int)InputKey::MouseScrollDown] = scrollY < -0.5f;
        m_scrollInputRaw.x = (float)scrollX;
        m_scrollInputRaw.y = (float)scrollY;
    }
}