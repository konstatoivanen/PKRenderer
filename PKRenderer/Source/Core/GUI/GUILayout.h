#pragma once
#include "Core/Math/Math.h"

namespace PK
{
    enum class GUILayoutStyle : uint8_t
    {
        Rows,       // Top to bottom, overflows
        Columns,    // Left to right, overflows.
        Flow,       // Left to right, new line on horizontal overflow.
        Grid,       // Rows then columns, fixed size. repeating pattern.
        Absolute,   // User rect placement, no layout tracking.
        Overlay     // Last layout rect..
    };

    struct GUILayoutGroup
    {
        friend struct GUILayout;
        
        struct Style
        {
            GUILayoutStyle style;
            short2 gridSize;
            short2 padding;
        };

        struct State
        {
            short2 cursor;
            short4 rect;
            short4 rectLast;
            short2 content_min;
            short2 content_max;
            uint32_t maxLineSize;
            uint32_t index;
        };

        GUILayoutGroup(const GUILayoutGroup&) = delete;
        GUILayoutGroup& operator=(const GUILayoutGroup&) = delete;
        GUILayoutGroup& operator=(GUILayoutGroup&& other) = delete;
        GUILayoutGroup(GUILayoutGroup&& other) noexcept;
        ~GUILayoutGroup();

        constexpr short4 GetRect() const { return m_state.rect; }
        constexpr uint32_t GetCount() const { return m_state.index; }
        constexpr short4 GetContentRect() const { return short4(m_state.content_min, m_state.content_max - m_state.content_min); }

    private:
        GUILayoutGroup(GUILayout* layout, const Style& style, const short4& desiredRect);

        GUILayout* m_layout = nullptr;
        GUILayoutGroup* m_parent = nullptr;
        Style m_style;
        State m_state;
    };

    struct GUILayout
    {
        friend struct GUILayoutGroup;

        GUILayout(short4 rect) : m_rect(rect) {}

        [[nodiscard]] GUILayoutGroup BeginGroup(const short4& desiredRect, const GUILayoutGroup::Style& style);
        [[nodiscard]] GUILayoutGroup BeginGroup(const short4& desiredRect, GUILayoutStyle style, short2 padding = PK_SHORT2_ZERO, short2 gridSize = PK_SHORT2_ZERO);
        [[nodiscard]] GUILayoutGroup BeginGroup(GUILayoutStyle style, short2 padding = PK_SHORT2_ZERO, short2 gridSize = PK_SHORT2_ZERO);
        void EndGroup();

        short4 GetRect(const short4& desiredRect = PK_SHORT4_ZERO);

    private:

        GUILayoutGroup* m_current = nullptr;
        short4 m_rect;
    };
}