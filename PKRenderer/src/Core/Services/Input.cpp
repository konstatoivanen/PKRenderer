#include "PreCompiledHeader.h"
#include "Input.h"
#include "Core/Services/Log.h"
#include "Core/UpdateStep.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Core::Services
{
    void Input::OnKeyInput(int key, int scancode, int action, int mods)
    {
        if (key < 0 || key >= (int)KeyCode::COUNT)
        {
            return;
        }

        m_inputActionsCurrent[key] = action;
    }

    void Input::OnScrollInput(double scrollX, double scrollY)
    {
        m_mouseScrollRaw.x = (float)scrollX;
        m_mouseScrollRaw.y = (float)scrollY;
    }

    void Input::OnMouseButtonInput(int button, int action, int mods)
    {
        if (button < 0 || button >= (int)KeyCode::COUNT)
        {
            return;
        }

        m_inputActionsCurrent[button] = action;
    }

    bool Input::GetKeyDown(KeyCode key)
    {
        return m_inputActionsCurrent[(int)key] == GLFW_PRESS && m_inputActionsPrevious[(int)key] == GLFW_RELEASE;
    }

    bool Input::GetKeyUp(KeyCode key)
    {
        return m_inputActionsCurrent[(int)key] == GLFW_RELEASE && m_inputActionsPrevious[(int)key] == GLFW_PRESS;
    }

    bool Input::GetKey(KeyCode key)
    {
        auto action = m_inputActionsCurrent[(int)key];
        return action == GLFW_PRESS || action == GLFW_REPEAT;
    }

    Math::float2 Input::GetAxis2D(KeyCode front, KeyCode back, KeyCode right, KeyCode left)
    {
        return
        {
            (GetKey(front) ? 1.0f : GetKey(back) ? -1.0f : 0.0f),
            (GetKey(right) ? 1.0f : GetKey(left) ? -1.0f : 0.0f)
        };
    }

    Math::float3 Input::GetAxis3D(KeyCode up, KeyCode down, KeyCode front, KeyCode back, KeyCode right, KeyCode left)
    {
        return
        {
            (GetKey(right) ? 1.0f : GetKey(left) ? -1.0f : 0.0f),
            (GetKey(up) ? 1.0f : GetKey(down) ? -1.0f : 0.0f),
            (GetKey(front) ? 1.0f : GetKey(back) ? -1.0f : 0.0f)
        };
    }

    void Input::Step(Window* window, int condition)
    {
        auto step = (PK::Core::UpdateStep)condition;

        switch (step)
        {
        case PK::Core::UpdateStep::CloseFrame:
        {
            memcpy(m_inputActionsPrevious, m_inputActionsCurrent, sizeof(m_inputActionsCurrent));
        }
        break;

        case PK::Core::UpdateStep::UpdateInput:
        {
            double xpos, ypos;
            int w, h;
            auto glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());
            glfwGetCursorPos(glfwWindow, &xpos, &ypos);
            glfwGetWindowSize(glfwWindow, &w, &h);

            m_mousePosition.x = (float)xpos;
            m_mousePosition.y = (float)ypos;
            m_mousePosition.y = (float)h - (float)ypos;
            m_mousePositionNormalized.x = m_mousePosition.x / w;
            m_mousePositionNormalized.y = m_mousePosition.y / h;
            m_mouseDelta = m_mousePosition - m_mousePrev;
            m_mousePrev = m_mousePosition;
            m_mouseScroll = m_mouseScrollRaw;
            m_mouseScrollRaw = { 0, 0 };

            m_sequencer->Next(this, this, 0);
        }
        break;
        }
    }

    std::string Input::KeyToString(KeyCode keycode)
    {
        switch (keycode)
        {
        case KeyCode::MOUSE1: return "MOUSE1";
        case KeyCode::MOUSE2: return "MOUSE2";
        case KeyCode::MOUSE3: return "MOUSE3";
        case KeyCode::MOUSE4: return "MOUSE4";
        case KeyCode::MOUSE5: return "MOUSE5";
        case KeyCode::MOUSE6: return "MOUSE6";
        case KeyCode::MOUSE7: return "MOUSE7";
        case KeyCode::MOUSE8: return "MOUSE8";
        case KeyCode::SPACE: return "SPACE";
        case KeyCode::APOSTROPHE: return "APOSTROPHE";
        case KeyCode::COMMA: return "COMMA";
        case KeyCode::MINUS: return "MINUS";
        case KeyCode::PERIOD: return "PERIOD";
        case KeyCode::SLASH: return "SLASH";
        case KeyCode::ALPHA0: return "ALPHA0";
        case KeyCode::ALPHA1: return "ALPHA1";
        case KeyCode::ALPHA2: return "ALPHA2";
        case KeyCode::ALPHA3: return "ALPHA3";
        case KeyCode::ALPHA4: return "ALPHA4";
        case KeyCode::ALPHA5: return "ALPHA5";
        case KeyCode::ALPHA6: return "ALPHA6";
        case KeyCode::ALPHA7: return "ALPHA7";
        case KeyCode::ALPHA8: return "ALPHA8";
        case KeyCode::ALPHA9: return "ALPHA9";
        case KeyCode::SEMICOLON: return "SEMICOLON";
        case KeyCode::EQUAL: return "EQUAL";
        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";
        case KeyCode::LEFT_BRACKET: return "LEFT_BRACKET";
        case KeyCode::BACKSLASH: return "BACKSLASH";
        case KeyCode::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case KeyCode::GRAVE_ACCENT: return "GRAVE_ACCENT";
        case KeyCode::WORLD_1: return "WORLD_1";
        case KeyCode::WORLD_2: return "WORLD_2";
        case KeyCode::ESCAPE: return "ESCAPE";
        case KeyCode::ENTER: return "ENTER";
        case KeyCode::TAB: return "TAB";
        case KeyCode::BACKSPACE: return "BACKSPACE";
        case KeyCode::INSERT: return "INSERT";
        case KeyCode::DEL: return "DEL";
        case KeyCode::RIGHT: return "RIGHT";
        case KeyCode::LEFT: return "LEFT";
        case KeyCode::DOWN: return "DOWN";
        case KeyCode::UP: return "UP";
        case KeyCode::PAGE_UP: return "PAGE_UP";
        case KeyCode::PAGE_DOWN: return "PAGE_DOWN";
        case KeyCode::HOME: return "HOME";
        case KeyCode::END: return "END";
        case KeyCode::CAPS_LOCK: return "CAPS_LOCK";
        case KeyCode::SCROLL_LOCK: return "SCROLL_LOCK";
        case KeyCode::NUM_LOCK: return "NUM_LOCK";
        case KeyCode::PRINT_SCREEN: return "PRINT_SCREEN";
        case KeyCode::PAUSE: return "PAUSE";
        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";
        case KeyCode::F13: return "F13";
        case KeyCode::F14: return "F14";
        case KeyCode::F15: return "F15";
        case KeyCode::F16: return "F16";
        case KeyCode::F17: return "F17";
        case KeyCode::F18: return "F18";
        case KeyCode::F19: return "F19";
        case KeyCode::F20: return "F20";
        case KeyCode::F21: return "F21";
        case KeyCode::F22: return "F22";
        case KeyCode::F23: return "F23";
        case KeyCode::F24: return "F24";
        case KeyCode::F25: return "F25";
        case KeyCode::KP_0: return "KP_0";
        case KeyCode::KP_1: return "KP_1";
        case KeyCode::KP_2: return "KP_2";
        case KeyCode::KP_3: return "KP_3";
        case KeyCode::KP_4: return "KP_4";
        case KeyCode::KP_5: return "KP_5";
        case KeyCode::KP_6: return "KP_6";
        case KeyCode::KP_7: return "KP_7";
        case KeyCode::KP_8: return "KP_8";
        case KeyCode::KP_9: return "KP_9";
        case KeyCode::KP_DECIMAL: return "KP_DECIMAL";
        case KeyCode::KP_DIVIDE: return "KP_DIVIDE";
        case KeyCode::KP_MULTIPLY: return "KP_MULTIPLY";
        case KeyCode::KP_SUBTRACT: return "KP_SUBTRACT";
        case KeyCode::KP_ADD: return "KP_ADD";
        case KeyCode::KP_ENTER: return "KP_ENTER";
        case KeyCode::KP_EQUAL: return "KP_EQUAL";
        case KeyCode::LEFT_SHIFT: return "LEFT_SHIFT";
        case KeyCode::LEFT_CONTROL: return "LEFT_CONTROL";
        case KeyCode::LEFT_ALT: return "LEFT_ALT";
        case KeyCode::LEFT_SUPER: return "LEFT_SUPER";
        case KeyCode::RIGHT_SHIFT: return "RIGHT_SHIFT";
        case KeyCode::RIGHT_CONTROL: return "RIGHT_CONTROL";
        case KeyCode::RIGHT_ALT: return "RIGHT_ALT";
        case KeyCode::RIGHT_SUPER: return "RIGHT_SUPER";
        }

        PK_THROW_ERROR("Invalid keycode value: %i", keycode);
    }

    KeyCode Input::StringToKey(const std::string& string)
    {
        if (string == "SPACE") return KeyCode::SPACE;
        if (string == "APOSTROPHE") return KeyCode::APOSTROPHE;
        if (string == "COMMA") return KeyCode::COMMA;
        if (string == "MINUS") return KeyCode::MINUS;
        if (string == "PERIOD") return KeyCode::PERIOD;
        if (string == "SLASH") return KeyCode::SLASH;
        if (string == "ALPHA0") return KeyCode::ALPHA0;
        if (string == "ALPHA1") return KeyCode::ALPHA1;
        if (string == "ALPHA2") return KeyCode::ALPHA2;
        if (string == "ALPHA3") return KeyCode::ALPHA3;
        if (string == "ALPHA4") return KeyCode::ALPHA4;
        if (string == "ALPHA5") return KeyCode::ALPHA5;
        if (string == "ALPHA6") return KeyCode::ALPHA6;
        if (string == "ALPHA7") return KeyCode::ALPHA7;
        if (string == "ALPHA8") return KeyCode::ALPHA8;
        if (string == "ALPHA9") return KeyCode::ALPHA9;
        if (string == "SEMICOLON") return KeyCode::SEMICOLON;
        if (string == "EQUAL") return KeyCode::EQUAL;
        if (string == "A") return KeyCode::A;
        if (string == "B") return KeyCode::B;
        if (string == "C") return KeyCode::C;
        if (string == "D") return KeyCode::D;
        if (string == "E") return KeyCode::E;
        if (string == "F") return KeyCode::F;
        if (string == "G") return KeyCode::G;
        if (string == "H") return KeyCode::H;
        if (string == "I") return KeyCode::I;
        if (string == "J") return KeyCode::J;
        if (string == "K") return KeyCode::K;
        if (string == "L") return KeyCode::L;
        if (string == "M") return KeyCode::M;
        if (string == "N") return KeyCode::N;
        if (string == "O") return KeyCode::O;
        if (string == "P") return KeyCode::P;
        if (string == "Q") return KeyCode::Q;
        if (string == "R") return KeyCode::R;
        if (string == "S") return KeyCode::S;
        if (string == "T") return KeyCode::T;
        if (string == "U") return KeyCode::U;
        if (string == "V") return KeyCode::V;
        if (string == "W") return KeyCode::W;
        if (string == "X") return KeyCode::X;
        if (string == "Y") return KeyCode::Y;
        if (string == "Z") return KeyCode::Z;
        if (string == "LEFT_BRACKET") return KeyCode::LEFT_BRACKET;
        if (string == "BACKSLASH") return KeyCode::BACKSLASH;
        if (string == "RIGHT_BRACKET") return KeyCode::RIGHT_BRACKET;
        if (string == "GRAVE_ACCENT") return KeyCode::GRAVE_ACCENT;
        if (string == "WORLD_1") return KeyCode::WORLD_1;
        if (string == "WORLD_2") return KeyCode::WORLD_2;
        if (string == "ESCAPE") return KeyCode::ESCAPE;
        if (string == "ENTER") return KeyCode::ENTER;
        if (string == "TAB") return KeyCode::TAB;
        if (string == "BACKSPACE") return KeyCode::BACKSPACE;
        if (string == "INSERT") return KeyCode::INSERT;
        if (string == "DEL") return KeyCode::DEL;
        if (string == "RIGHT") return KeyCode::RIGHT;
        if (string == "LEFT") return KeyCode::LEFT;
        if (string == "DOWN") return KeyCode::DOWN;
        if (string == "UP") return KeyCode::UP;
        if (string == "PAGE_UP") return KeyCode::PAGE_UP;
        if (string == "PAGE_DOWN") return KeyCode::PAGE_DOWN;
        if (string == "HOME") return KeyCode::HOME;
        if (string == "END") return KeyCode::END;
        if (string == "CAPS_LOCK") return KeyCode::CAPS_LOCK;
        if (string == "SCROLL_LOCK") return KeyCode::SCROLL_LOCK;
        if (string == "NUM_LOCK") return KeyCode::NUM_LOCK;
        if (string == "PRINT_SCREEN") return KeyCode::PRINT_SCREEN;
        if (string == "PAUSE") return KeyCode::PAUSE;
        if (string == "F1") return KeyCode::F1;
        if (string == "F2") return KeyCode::F2;
        if (string == "F3") return KeyCode::F3;
        if (string == "F4") return KeyCode::F4;
        if (string == "F5") return KeyCode::F5;
        if (string == "F6") return KeyCode::F6;
        if (string == "F7") return KeyCode::F7;
        if (string == "F8") return KeyCode::F8;
        if (string == "F9") return KeyCode::F9;
        if (string == "F10") return KeyCode::F10;
        if (string == "F11") return KeyCode::F11;
        if (string == "F12") return KeyCode::F12;
        if (string == "F13") return KeyCode::F13;
        if (string == "F14") return KeyCode::F14;
        if (string == "F15") return KeyCode::F15;
        if (string == "F16") return KeyCode::F16;
        if (string == "F17") return KeyCode::F17;
        if (string == "F18") return KeyCode::F18;
        if (string == "F19") return KeyCode::F19;
        if (string == "F20") return KeyCode::F20;
        if (string == "F21") return KeyCode::F21;
        if (string == "F22") return KeyCode::F22;
        if (string == "F23") return KeyCode::F23;
        if (string == "F24") return KeyCode::F24;
        if (string == "F25") return KeyCode::F25;
        if (string == "KP_0") return KeyCode::KP_0;
        if (string == "KP_1") return KeyCode::KP_1;
        if (string == "KP_2") return KeyCode::KP_2;
        if (string == "KP_3") return KeyCode::KP_3;
        if (string == "KP_4") return KeyCode::KP_4;
        if (string == "KP_5") return KeyCode::KP_5;
        if (string == "KP_6") return KeyCode::KP_6;
        if (string == "KP_7") return KeyCode::KP_7;
        if (string == "KP_8") return KeyCode::KP_8;
        if (string == "KP_9") return KeyCode::KP_9;
        if (string == "KP_DECIMAL") return KeyCode::KP_DECIMAL;
        if (string == "KP_DIVIDE") return KeyCode::KP_DIVIDE;
        if (string == "KP_MULTIPLY") return KeyCode::KP_MULTIPLY;
        if (string == "KP_SUBTRACT") return KeyCode::KP_SUBTRACT;
        if (string == "KP_ADD") return KeyCode::KP_ADD;
        if (string == "KP_ENTER") return KeyCode::KP_ENTER;
        if (string == "KP_EQUAL") return KeyCode::KP_EQUAL;
        if (string == "LEFT_SHIFT") return KeyCode::LEFT_SHIFT;
        if (string == "LEFT_CONTROL") return KeyCode::LEFT_CONTROL;
        if (string == "LEFT_ALT") return KeyCode::LEFT_ALT;
        if (string == "LEFT_SUPER") return KeyCode::LEFT_SUPER;
        if (string == "RIGHT_SHIFT") return KeyCode::RIGHT_SHIFT;
        if (string == "RIGHT_CONTROL") return KeyCode::RIGHT_CONTROL;
        if (string == "RIGHT_ALT") return KeyCode::RIGHT_ALT;
        if (string == "RIGHT_SUPER") return KeyCode::RIGHT_SUPER;

        PK_THROW_ERROR("Invalid keycode value: %s", string);
    }
}