#pragma once
#include "Core/Math/Math.h"

namespace PK
{
    enum class GUILayoutMode : uint8_t
    {
        Absolute,   // User rect placement, relative to parent, no layout tracking.
        Partition,  // Subdivides space 
        Rows,       // Top to bottom, overflows
        Columns,    // Left to right, overflows.
        Flow,       // Left to right, new line on horizontal overflow.
        Grid        // Rows then columns, fixed size. repeating pattern.
    };

    struct GUILayoutStyle
    {
        GUILayoutMode mode = GUILayoutMode::Absolute;
        short4 padding = PK_SHORT4_ZERO;
        short2 gridsize = PK_SHORT2_ZERO;
        short2 fixedsize = PK_SHORT2_ZERO;
        bool clampToParent = false;
    };

    struct GUILayout
    {
        short4 outer;
        short4 inner;
        short4 cordon;
        short4 previous;
        short4 content;
        short2 cursor;
        short2 gridsize;
        uint32_t linesize;
        uint32_t count;
        GUILayoutMode mode;
    };

    struct GUILayoutStack
    {
        constexpr const static uint32_t LAYOUT_STACK_MAX = 15u;

        GUILayoutStack(const short4& renderArea);

        bool Begin(const GUILayoutStyle& style, const short4& desiredRect = PK_SHORT4_ZERO);
        void End();
        const GUILayout& Get() const;
        short4 NextRect(const short4& desiredRect = PK_SHORT4_ZERO);

    private:
        GUILayout m_layoutStack[LAYOUT_STACK_MAX + 1u]{};
        uint32_t m_layoutHead = 0u;
    };
}
