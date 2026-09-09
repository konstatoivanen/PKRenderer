#include "PrecompiledHeader.h"
#include "Core/Math/Rect.h"
#include "GUILayoutStack.h"

namespace PK
{
    GUILayoutStack::GUILayoutStack(const short4& renderArea)
    {
        m_layoutStack[0].outer = renderArea;
        m_layoutStack[0].inner = renderArea;
        m_layoutStack[0].cordon = renderArea;
        m_layoutStack[0].previous = PK_SHORT4_ZERO;
        m_layoutStack[0].content = PK_SHORT4_ZERO;
        m_layoutStack[0].cursor = renderArea.xy;
        m_layoutStack[0].gridsize = PK_SHORT2_ONE;
        m_layoutStack[0].linesize = 0u;
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
                rect = math::rectClamp(rect, parent.inner);
            }

            GUILayout layout;
            layout.outer = NextRect(rect);
            layout.inner = math::rectPad(layout.outer, style.padding);
            layout.cordon = layout.inner;
            layout.previous = layout.inner;
            layout.content = short4(layout.inner.xy, 0, 0);
            layout.gridsize = math::max(PK_SHORT2_ONE, style.gridsize);
            layout.cursor = layout.inner.xy;
            layout.linesize = 0u;
            layout.count = 0u;
            layout.mode = style.mode;
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
        const auto inner_max = short2(state.inner.xy + state.inner.zw);
        auto rect = desiredRect;

        switch (state.mode)
        {
            case GUILayoutMode::Absolute:
            {
                rect.xy += state.inner.xy;
                if (rect.z <= 0) rect.z = inner_max.x - rect.x;
                if (rect.w <= 0) rect.w = inner_max.y - rect.y;
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
                    auto [head, tail] = math::rectSplitMin(state.cordon, rect.xy());
                    state.cordon = tail;
                    rect = head;
                }
                else if (rect.z || rect.w)
                {
                    auto [head, tail] = math::rectSplitMax(state.cordon, rect.zw());
                    state.cordon = tail;
                    rect = head;
                }
                else
                {
                    return state.cordon;
                }
            }
            break;

            case GUILayoutMode::Rows: 
            {
                rect.xy += state.cursor;
                if (rect.z <= 0) rect.z = inner_max.x - rect.x;
                state.cursor.y = rect.y + rect.w;
                state.cordon = short4(state.cursor, inner_max - state.cursor);
            }
            break;
    
            case GUILayoutMode::Columns:
            {
                rect.xy += state.cursor;
                if (rect.w <= 0) rect.w = inner_max.y - rect.y;
                state.cursor.x = rect.x + rect.z;
                state.cordon = short4(state.cursor, inner_max - state.cursor);
            }
            break;

            case GUILayoutMode::Flow:
            {
                if (state.cursor.x + rect.x + rect.z > inner_max.x)
                {
                    state.cursor.x = state.inner.x;
                    state.cursor.y = state.cursor.y + state.linesize;
                    state.linesize = 0;
                }
    
                state.linesize = math::max(state.linesize, (uint32_t)(rect.y + rect.w));
                rect.xy += state.cursor;
                state.cursor.x = rect.x + rect.z;
                state.cordon.xy = short2(state.inner.x, state.cursor.y);
                state.cordon.zw = inner_max - state.cordon.xy;
            }
            break;
    
            case GUILayoutMode::Grid:
            {
                rect = math::rectGrid(state.inner, state.gridsize, state.count);
                state.cursor = rect.xy;
                state.cordon.xy = short2(state.inner.x, state.cursor.y);
                state.cordon.zw = inner_max - state.cordon.xy;
            }
            break;
        }
    
        state.count++;
        state.previous = rect;
        state.content = math::rectMerge(state.content, rect);
        return rect;
    }
}
