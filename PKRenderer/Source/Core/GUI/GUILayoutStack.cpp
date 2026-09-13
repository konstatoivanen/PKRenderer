#include "PrecompiledHeader.h"
#include "Core/Math/Rect.h"
#include "GUILayoutStack.h"

namespace PK
{
    GUILayoutStack::GUILayoutStack(const short4& renderArea)
    {
        m_layoutStack[0].area = renderArea;
        m_layoutStack[0].local = short4(0, 0, renderArea.z, renderArea.w);
        m_layoutStack[0].cursor = renderArea;
        m_layoutStack[0].previous = PK_SHORT4_ZERO;
        m_layoutStack[0].content = PK_SHORT4_ZERO;
        m_layoutStack[0].gridsize = PK_SHORT2_ONE;
        m_layoutStack[0].count = 0u;
        m_layoutStack[0].mode = GUILayoutMode::Absolute;
    }

    bool GUILayoutStack::Begin(const GUILayoutStyle& style, const short4& desiredRect)
    {
        if (m_layoutHead < LAYOUT_STACK_MAX)
        {
            auto& parent = Get();
            auto rect = desiredRect;

            if (style.clampToParent)
            {
                rect = math::rectClamp(rect, parent.local);
            }

            GUILayout layout;
            layout.area = math::rectPad(NextRect(rect), style.padding);
            layout.local.xy = layout.area.xy() - parent.area.xy();
            layout.local.zw = layout.area.zw();
            layout.previous = layout.area;
            layout.content = short4(layout.area.xy, 0, 0);
            layout.gridsize = math::max(PK_SHORT2_ONE, style.gridsize);
            layout.count = 0u;
            layout.mode = style.mode;

            switch (layout.mode)
            {
                case GUILayoutMode::Absolute: layout.cursor = layout.area; break;
                case GUILayoutMode::Partition: layout.cursor = layout.area; break;
                case GUILayoutMode::Rows: layout.cursor = short4(layout.area.x, layout.area.y, layout.area.z, 0); break;
                case GUILayoutMode::Columns: layout.cursor = short4(layout.area.x, layout.area.y, 0, layout.area.w); break;
                case GUILayoutMode::Flow: layout.cursor = short4(layout.area.x, layout.area.y, 0, 0); break;
                case GUILayoutMode::Grid: layout.cursor = math::rectGrid(layout.area, layout.gridsize, layout.count); break;
            }

            m_layoutStack[++m_layoutHead] = layout;
            return true;
        }

        return false;
    }

    void GUILayoutStack::End()
    {
        if (m_layoutHead)
        {
            auto content = Get().content;
            auto& child = m_layoutStack[--m_layoutHead];
            child.content = math::rectMerge(child.content, content);
        }
    }

    const GUILayout& GUILayoutStack::Get() const
    {
        return m_layoutStack[m_layoutHead];
    }

    short4 GUILayoutStack::NextRect(const short4& desiredRect)
    {
        auto& state = m_layoutStack[m_layoutHead];
        const auto inner_max = short2(state.area.xy + state.area.zw);
        auto rect = desiredRect;

        switch (state.mode)
        {
            case GUILayoutMode::Absolute:
            {
                rect.xy += state.area.xy;
                rect.z = rect.z <= 0 ? inner_max.x - rect.x : rect.z;
                rect.w = rect.w <= 0 ? inner_max.y - rect.y : rect.w;
            }
            break;

            case GUILayoutMode::Partition:
            {
                if (rect.x > 0) rect.yzw = 0;
                if (rect.y > 0) rect.xzw = 0;
                if (rect.z > 0) rect.xyw = 0;
                if (rect.w > 0) rect.xyz = 0;

                if (rect.x || rect.y)
                {
                    auto [head, tail] = math::rectSplitMin(state.cursor, rect.xy());
                    state.cursor = tail;
                    rect = head;
                }
                else if (rect.z || rect.w)
                {
                    auto [head, tail] = math::rectSplitMax(state.cursor, rect.zw());
                    state.cursor = tail;
                    rect = head;
                }
                else
                {
                    return state.cursor;
                }
            }
            break;

            case GUILayoutMode::Rows: 
            {
                rect.xy += state.cursor.xy;
                rect.z = rect.z <= 0 ? inner_max.x - rect.x : rect.z;
                state.cursor.y = rect.y + rect.w;
            }
            break;
    
            case GUILayoutMode::Columns:
            {
                rect.xy += state.cursor.xy;
                rect.w = rect.w <= 0 ? inner_max.y - rect.y : rect.w;
                state.cursor.x = rect.x + rect.z;
            }
            break;

            case GUILayoutMode::Flow:
            {
                rect.z = rect.z <= 0 && rect.w <= 0 ? state.area.z - rect.x : rect.z;
                rect.z = rect.z <= 0 ? state.cursor.z - rect.x : rect.z;

                if (rect.x + rect.z > state.cursor.z)
                {
                    state.cursor.x = state.area.x;
                    state.cursor.y = state.cursor.y + state.cursor.w;
                    state.cursor.w = 0;
                }

                rect.w = rect.w <= 0 ? inner_max.y - state.cursor.y + rect.y : rect.w;
                state.cursor.w = math::max(state.cursor.w, (int16_t)(rect.y + rect.w));
                
                rect.xy += state.cursor.xy;
                state.cursor.x = rect.x + rect.z;
                state.cursor.z = inner_max.x - state.cursor.x;
            }
            break;
    
            case GUILayoutMode::Grid:
            {
                rect = math::rectGrid(state.area, state.gridsize, state.count);
                state.cursor = rect;
            }
            break;
        }
    
        state.count++;
        state.previous = rect;
        state.content = math::rectMerge(state.content, rect);
        return rect;
    }
}
