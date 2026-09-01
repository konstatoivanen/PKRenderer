#include "PrecompiledHeader.h"
#include "GUILayout.h"

namespace PK
{
    GUILayoutGroup::GUILayoutGroup(GUILayout* layout, const Style& style, const short4& desiredRect) :
        m_layout(layout), 
        m_parent(layout->m_current),
        m_style(style)
    {
        m_style.gridSize = math::max(short2(1,1), m_style.gridSize);
        m_state.rect = m_layout->GetRect(desiredRect);
        m_state.cursor = m_state.rect.xy + m_style.padding;
        m_state.rectLast = m_state.rect;
        m_state.content_min = +PK_SHORT2_MAX;
        m_state.content_max = -PK_SHORT2_MAX;
        m_state.maxLineSize = 0u;
        m_state.index = 0u;
        m_layout->m_current = this;
    }

    GUILayoutGroup::~GUILayoutGroup()
    {
        if (m_layout && m_layout->m_current == this) 
        {
            m_layout->EndGroup();
        }
    }

    GUILayoutGroup::GUILayoutGroup(GUILayoutGroup&& other) noexcept : 
        m_layout(other.m_layout),
        m_parent(other.m_parent),
        m_style(other.m_style),
        m_state(other.m_state)
    {
        if (m_layout) 
        {
            other.m_layout = nullptr;
            other.m_parent = nullptr;

            if (m_layout->m_current == &other) 
            {
                m_layout->m_current = this;
            }
            else
            {
                // In case we're moving group to another variable.
                for (auto group = m_layout->m_current; group; group = group->m_parent)
                {
                    if (group->m_parent == &other)
                    {
                        group->m_parent = this;
                        break;
                    }
                }
            }
        }
    }

    GUILayoutGroup GUILayout::BeginGroup(const short4& desiredRect, const GUILayoutGroup::Style& style)
    {
        return GUILayoutGroup(this, style, desiredRect);
    }

    GUILayoutGroup GUILayout::BeginGroup(const short4& desiredRect, GUILayoutStyle style, short2 padding, short2 gridSize)
    {
        GUILayoutGroup::Style groupStyle;
        groupStyle.style = style;
        groupStyle.gridSize = gridSize;
        groupStyle.padding = padding;
        return GUILayoutGroup(this, groupStyle, desiredRect);
    }

    GUILayoutGroup GUILayout::BeginGroup(GUILayoutStyle style, short2 padding, short2 gridSize)
    {
        GUILayoutGroup::Style groupStyle;
        groupStyle.style = style;
        groupStyle.gridSize = gridSize;
        groupStyle.padding = padding;
        return GUILayoutGroup(this, groupStyle, PK_SHORT4_ZERO);
    }

    void GUILayout::EndGroup()
    {
        if (m_current)
        {
            auto child = m_current;
            auto parent = m_current->m_parent;

            if (parent)
            {
                parent->m_state.content_min = math::min(parent->m_state.content_min, child->m_state.content_min);
                parent->m_state.content_max = math::max(parent->m_state.content_max, child->m_state.content_max); 
            }

            m_current = parent;
            child->m_layout = nullptr;
            child->m_parent = nullptr;
        }
    }

    short4 GUILayout::GetRect(const short4& desiredRect)
    {
        short4 out_rect = desiredRect;
        
        // @TODO return max rect if input is 0
        if (!m_current) 
        {
            out_rect.x += m_rect.x;
            out_rect.y += m_rect.y;
            if (out_rect.z <= 0) out_rect.z = m_rect.z;
            if (out_rect.w <= 0) out_rect.w = m_rect.w;
            return out_rect;
        }
    
        auto& style = m_current->m_style;
        auto& state = m_current->m_state;
        
        const auto inner_size = state.rect.zw - style.padding * (short)2;
        const auto inner_min = state.rect.xy + style.padding;
        const auto inner_max = state.rect.xy + state.rect.zw - style.padding;

        switch (style.style)
        {
            case GUILayoutStyle::Rows: 
            {
                out_rect.xy += state.cursor;
                if (out_rect.z <= 0) out_rect.z = inner_max.x - out_rect.x;
                state.cursor.y = out_rect.y + out_rect.w + style.padding.y;
            }
            break;
    
            case GUILayoutStyle::Columns: 
            {
                out_rect.xy += state.cursor;
                if (out_rect.w <= 0) out_rect.w = inner_max.y - out_rect.y;
                state.cursor.x = out_rect.x + out_rect.z + style.padding.x;
            }
            break;

            case GUILayoutStyle::Flow: 
            {
                if (state.cursor.x + out_rect.x + out_rect.z > inner_max.x)
                {
                    state.cursor.x = inner_min.x;
                    state.cursor.y = state.cursor.y + state.maxLineSize + style.padding.y;
                    state.maxLineSize = 0;
                }
    
                state.maxLineSize = math::max(state.maxLineSize, (uint32_t)math::max(0, out_rect.y + out_rect.w));

                out_rect.xy += state.cursor;
                state.cursor.x = out_rect.x + out_rect.z + style.padding.x;
            }
            break;
    
            case GUILayoutStyle::Grid: 
            {
                const auto cell_dims = style.gridSize;
                const auto cell_max = cell_dims - (short)1;

                const auto cell_idx = state.index % (cell_dims.x * cell_dims.y);
                const auto cell_xy = short2(cell_idx % cell_dims.x, (cell_idx / cell_dims.x) % cell_dims.y);

                const auto cell_size = (inner_size - (style.padding * cell_max)) / cell_dims;
                const auto cell_edge = inner_size - (cell_size + style.padding) * cell_max;

                out_rect.xy = inner_min + cell_xy * (cell_size + style.padding);
                out_rect.zw = math::lerp(cell_size, cell_edge, cell_xy == cell_dims - (short)1);
            }
            break;
    
            case GUILayoutStyle::Absolute: 
            {
                out_rect.xy += state.rect.xy;
            }
            break;
    
            case GUILayoutStyle::Overlay: 
            {
                out_rect = state.rectLast;
            }
            break;
        }
    
        state.index++;
        state.rectLast = out_rect;
        state.content_max = math::max(state.content_max, out_rect.xy + out_rect.zw);
        state.content_min = math::min(state.content_min, out_rect.xy());

        return out_rect;
    }
}
