#include "PrecompiledHeader.h"
#include "Core/Math/Rect.h"
#include "Core/Input/InputState.h"
#include "GUIInput.h"

namespace PK
{
    GUIInput::GUIInput(const GUIKeys* keys, GUIInputState* state, InputState* input, const short2& screenOffset, const short2& screenScale) :
        m_keys(keys),
        m_state(state),
        m_input(input)
    {
        m_cursor = input ? short2(input->cursorPosition) : PK_SHORT2_ZERO;
        m_cursorDelta = input ? short2(input->cursorPositionDelta) : PK_SHORT2_ZERO;
        m_scrollDelta = input ? short2(input->cursorScroll) : PK_SHORT2_ZERO;
        
        m_cursor.xy *= screenScale;
        m_cursor.xy += screenOffset;
        m_cursorDelta *= screenScale;

        if (m_state->navState != NAV_STATE_NONE)
        {
            m_hotHoverLayer = m_state->navLayer;
            m_navScore = ~0u;
        }
    }

    GUIInput::~GUIInput()
    {
        m_state->controlId = m_hotControlId;
        m_state->hoverId = m_hotHoverId;
        m_state->hoverLayer = m_hotHoverLayer;
        m_state->hoverPos = m_hotHoverPos;

        if (m_state->navState != NAV_STATE_NONE)
        {
            m_state->navState = NAV_STATE_NONE;
        }

        if (!math::any(m_cursorDelta) && !m_state->controlId && m_state->hoverId)
        {
            m_state->navAxis = KeyDownAxis(true);
            m_state->navLayer = m_state->hoverLayer;

            if (math::any(m_state->navAxis))
            {
                m_state->navState = NAV_STATE_DIRECTIONAL;
            }

            if (KeyDown(GUIKey::Exit, true))
            {
                if (m_state->navLayer)
                {
                    m_state->navLayer--;
                    m_state->navState = ~0u;
                }
            }
        }
    }


    void GUIInput::PushLayer()
    {
        ++m_layer;
    }

    void GUIInput::PopLayer()
    {
        if (m_layer)
        {
            --m_layer;
        }
    }

    bool GUIInput::KeyDown(GUIKey key, bool consume)
    {
        const auto& triplet = (&m_keys->Enter)[(uint32_t)key];
        return m_input ? consume ? m_input->ConsumeKeyDown(triplet) : m_input->GetKeyDown(triplet) : false;
    }

    bool GUIInput::KeyUp(GUIKey key, bool consume)
    {
        const auto& triplet = (&m_keys->Enter)[(uint32_t)key];
        return m_input ? consume ? m_input->ConsumeKeyUp(triplet) : m_input->GetKeyUp(triplet) : false;
    }

    bool GUIInput::Key(GUIKey key, bool consume)
    {
        const auto& triplet = (&m_keys->Enter)[(uint32_t)key];
        return m_input ? consume ? m_input->ConsumeKey(triplet) : m_input->GetKey(triplet) : false;
    }

    short2 GUIInput::KeyDownAxis(bool consume)
    {
        short2 axis = PK_SHORT2_ZERO;;
        axis.x += (int16_t)KeyDown(GUIKey::Right, consume);
        axis.x -= (int16_t)KeyDown(GUIKey::Left, consume);
        axis.y += (int16_t)KeyDown(GUIKey::Down, consume);
        axis.y -= (int16_t)KeyDown(GUIKey::Up, consume);
        return axis;
    }

    short2 GUIInput::KeyAxis(bool consume)
    {
        short2 axis = PK_SHORT2_ZERO;;
        axis.x += (int16_t)Key(GUIKey::Right, consume);
        axis.x -= (int16_t)Key(GUIKey::Left, consume);
        axis.y += (int16_t)Key(GUIKey::Down, consume);
        axis.y -= (int16_t)Key(GUIKey::Up, consume);
        return axis;
    }

    bool GUIInput::HasHover(uint64_t uuid) const { return m_state->hoverId == uuid; }
    bool GUIInput::HasControl(uint64_t uuid) const { return m_state->controlId == uuid; }
    bool GUIInput::LostControl(uint64_t uuid) const { return m_hotControlId != uuid && m_state->controlId == uuid; }
    void GUIInput::SetClipRect(const short4& rect) { m_clipRect = rect; }

    bool GUIInput::Hover(const short4& rect, uint64_t uuid)
    {
        // Mouse input hover selection
        const auto is_nav_move = m_state->navState != NAV_STATE_NONE;
        const auto is_cursor_move = !is_nav_move && math::any(m_cursorDelta);

        // No moves try to retain hover id.
        if (!is_nav_move && !is_cursor_move && uuid != m_state->hoverId)
        {
            return false;
        }

        // Cursor move. didn't hit rect. not hovered. Also dicard existing hover on cursor move if invalid.
        if (is_cursor_move && (m_layer < m_hotHoverLayer || !math::rectIntersect(rect, m_clipRect) || !math::rectIntersect(rect, m_cursor)))
        {
            return false;
        }

        // Nav move try to find nearest in selected layer. layer selection at the beginning of frame.
        if (is_nav_move && m_state->hoverId != uuid)
        {
            const auto delta = rect.xy + rect.zw / (short)2 - m_state->hoverPos;
            const auto has_axis = math::any(m_state->navAxis);
            
            if (has_axis && math::dot(m_state->navAxis, delta) <= 0)
            {
                return false;
            }

            const auto ortho = short2(m_state->navAxis.y, -m_state->navAxis.x);
            const auto dist0 = (int16_t)math::length(float2(delta));
            const auto dist1 = math::dot(m_state->navAxis, delta);
            const auto dist2 = math::lerp(dist0, dist1, has_axis);
            const auto dist3 = math::abs(math::dot(ortho, delta));
            const auto dist4 = math::abs((int32_t)m_layer - (int32_t)m_hotHoverLayer) + 1u;
            const auto score = (dist3 * 0xFu + dist2) * dist4;

            if (score >= m_navScore)
            {
                return false;
            }

            m_navScore = score;
        }

        // Hacky fix for preventing passthrough of hoverid when navigating to previous element.
        if (is_nav_move && m_state->hoverId == uuid && m_navScore != ~0u)
        {
            return false;
        }

        m_hotHoverId = uuid;
        m_hotHoverLayer = m_layer;
        m_hotHoverPos = rect.xy + rect.zw / (short)2;
        return true;
    }

    bool GUIInput::Button(const short4& rect, uint64_t uuid)
    {
        Hover(rect, uuid);

        if (!m_hotControlId && (HasHover(uuid) || HasControl(uuid)) && Key(GUIKey::Enter, true))
        {
            m_hotControlId = uuid;
        }

        return LostControl(uuid);
    }

    bool GUIInput::ButtonDrag(const short4& rect, uint64_t uuid, short2* offset)
    {
        const auto pressed = Button(rect, uuid);

        if (offset)
        {
            if (HasHover(uuid) && Key(GUIKey::Control, true))
            {
                *offset += KeyAxis(true) * (int16_t)2;
            }

            if (HasControl(uuid))
            {
                *offset += m_cursorDelta;
            }
        }

        return pressed;
    }
}
