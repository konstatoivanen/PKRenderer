#pragma once
#include "Core/Math/Math.h"
#include "GUIKeys.h"

namespace PK
{
    struct InputState;

    struct GUIInputState
    {
        constexpr const static uint32_t TEXT_INPUT_MAX = 511u;

        uint64_t controlId = 0ull;
        uint64_t hoverId = 0ull;
        uint32_t hoverLayer = 0u;
        short2   hoverPos = PK_SHORT2_ZERO;
        
        uint32_t navState = 0u;
        uint32_t navLayer = 0u;
        short2   navAxis = PK_SHORT2_ZERO;

        uint32_t textLength = 0u;
        uint32_t textCaret = 0u;
        uint32_t textSelect = 0u;
        char textBuffer[TEXT_INPUT_MAX + 1u]{};
    };

    struct GUIInput
    {
        constexpr const static uint32_t NAV_STATE_NONE = 0u;
        constexpr const static uint32_t NAV_STATE_NEXT = 1u;
        constexpr const static uint32_t NAV_STATE_PREV = 2u;
        constexpr const static uint32_t NAV_STATE_DIRECTIONAL = 3u;

        GUIInput(const GUIKeys* keys,
            GUIInputState* state, 
            InputState* input, 
            const short2& screenOffset,
            const short2& screenScale);
        ~GUIInput();

        void PushLayer();
        void PopLayer();

        inline short2 GetCursor() const { return m_cursor; }
        inline short2 GetCursorDelta() const { return m_cursorDelta; }
        inline short2 GetScrollDelta() const { return m_scrollDelta; }

        bool KeyDown(GUIKey key, bool consume);
        bool KeyUp(GUIKey key, bool consume);
        bool Key(GUIKey key, bool consume);
        short2 KeyDownAxis(bool consume);
        short2 KeyAxis(bool consume);

        bool HasHover(uint64_t uuid) const;
        bool HasControl(uint64_t uuid) const;
        bool LostControl(uint64_t uuid) const;
        void SetClipRect(const short4& rect);

        bool Hover(const short4& rect, uint64_t uuid);
        bool Button(const short4& rect, uint64_t uuid);
        bool ButtonDrag(const short4& rect, uint64_t uuid, short2* offset);

    private:
        const GUIKeys* m_keys;
        GUIInputState* m_state;
        InputState* m_input;

        uint64_t m_hotControlId = 0ull;
        uint64_t m_hotHoverId = 0ull;
        uint32_t m_hotHoverLayer = 0ull;
        short2 m_hotHoverPos = PK_SHORT2_ZERO;
        short4 m_clipRect = { PK_SHORT2_ZERO, PK_SHORT2_MAX };
        short2 m_cursor = -PK_SHORT2_MAX;
        short2 m_cursorDelta = PK_SHORT2_ZERO;
        short2 m_scrollDelta = PK_SHORT2_ZERO;
        uint32_t m_layer = 0u;
        uint32_t m_navScore = 0u;
    };
}
