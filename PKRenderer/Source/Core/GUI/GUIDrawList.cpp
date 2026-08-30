#include "PrecompiledHeader.h"
#include "Core/Rendering/Font.h"
#include "Core/Math/Extended.h"
#include "GUIDrawList.h"

namespace PK
{
    struct GUITextContext
    {
        GUIAllocation* allocation;
        float2 texelSize;
        color32 color;
        uint16_t textureIndex;
    };

    void GUIDrawList::PushClipRect(const short4& nrect) 
    { 
        auto orect = GetClipRect();
        const auto nm = nrect.xy() + nrect.zw();
        const auto om = orect.xy() + orect.zw();
        const auto offset = math::max(nrect.xy(), orect.xy());
        const auto size = math::min(nm, om) - offset;
        m_clipStack.Add(short4(offset, math::max(PK_SHORT2_ZERO, size)));
    }
    
    void GUIDrawList::PopClipRect() 
    { 
        m_clipStack.Pop(); 
    }
    
    void GUIDrawList::PushLayer() 
    { 
        m_layer++; 
    }
    
    void GUIDrawList::PopLayer() 
    { 
        if (m_layer) 
        { 
            --m_layer; 
        } 
    }

    short4 GUIDrawList::GetClipRect() const
    {
        auto rect = m_allocator->GUIGetRenderAreaRect();
        auto back = m_clipStack.GetBack();
        return back ? *back : rect;
    }

    uint16_t GUIDrawList::GetLayer() const
    {
        return m_layer;
    }


    // TRIANGLE DRAW
    void GUIDrawList::DrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c)
    {
        GUIAllocation allocation;
        if (m_allocator->GUIAllocate(m_layer, 3u, 3u, &allocation))
        {
            allocation.indices[0] = allocation.vertexOffset + 0u;
            allocation.indices[1] = allocation.vertexOffset + 1u;
            allocation.indices[2] = allocation.vertexOffset + 2u;
            allocation.vertices[0] = a;
            allocation.vertices[1] = b;
            allocation.vertices[2] = c;
        }
    }

  
    // LINE DRAWS
    void GUIDrawList::DrawLine(const short2& p0, const short2& p1, const color32& color0, const color32& color1, const float width)
    {
        const auto rmax = math::max(p0, p1);
        const auto rmin = math::min(p0, p1);
        const auto rect = short4(rmin, rmax - rmin);

        if (math::intersectRects(rect, GetClipRect()))
        {
            const auto p0f = float2(p0.x + 0.5f, p0.y + 0.5f);
            const auto p1f = float2(p1.x + 0.5f, p1.y + 0.5f);
            const auto delta = p1f - p0f;
            const auto length = math::length(delta);

            GUIAllocation allocation;
            if (length >= 1.0f && m_allocator->GUIAllocate(m_layer, 4u, 6u, &allocation))
            {
                const auto direction = delta / length;
                const auto normal = float2(-direction.y, direction.x);
                const auto offset = normal * (width * 0.5f);
                allocation.indices[0] = allocation.vertexOffset + 0u;
                allocation.indices[1] = allocation.vertexOffset + 1u;
                allocation.indices[2] = allocation.vertexOffset + 2u;
                allocation.indices[3] = allocation.vertexOffset + 2u;
                allocation.indices[4] = allocation.vertexOffset + 3u;
                allocation.indices[5] = allocation.vertexOffset + 0u;
                allocation.vertices[0] = { color0, math::round(p0f + offset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT }; 
                allocation.vertices[1] = { color1, math::round(p1f + offset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT }; 
                allocation.vertices[2] = { color1, math::round(p1f - offset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT }; 
                allocation.vertices[3] = { color0, math::round(p0f - offset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT }; 
            }
        }
    }

    void GUIDrawList::DrawLine(const short2& p0, const short2& p1, const color32& color, const float width)
    {
        DrawLine(p0, p1, color, color, width);
    }

  
    // CURVE DRAWS
    void GUIDrawList::DrawCurve(const short2* points, const color32& color0, const color32& color1, uint32_t count, const float width)
    {
        GUIAllocation allocation;
        if (points && count > 1u && m_allocator->GUIAllocate(m_layer, count * 2u, (count - 1u) * 6u, &allocation))
        {
            for (auto i = 0u; i < count; ++i)
            {
                const auto i0 = (uint32_t)math::max((int32_t)i - 1, 0);
                const auto i1 = (uint32_t)math::min(i + 1u, count - 1u);
                const auto pCurr = float2(points[i].x + 0.5f, points[i].y + 0.5f);
                const auto pPrev = float2(points[i0].x + 0.5f, points[i0].y + 0.5f);
                const auto pNext = float2(points[i1].x + 0.5f, points[i1].y + 0.5f);
                const auto color = math::lerp(color0, color1, (float)i / (float)(count - 1u));
                
                auto normal = PK_FLOAT2_ZERO;

                if (i == 0u)
                {
                    normal = math::safenormalize(pNext - pCurr);
                    normal = float2(-normal.y, normal.x);
                }
                else if (i == count - 1u)
                {
                    normal = math::safenormalize(pCurr - pPrev);
                    normal = float2(-normal.y, normal.x);
                }
                else
                {
                    const auto dirI = math::safenormalize(pCurr - pPrev);
                    const auto dirO = math::safenormalize(pNext - pCurr);
                    const auto normalI = float2(-dirI.y, dirI.x);
                    const auto normalO = float2(-dirO.y, dirO.x);
                    const auto offset = math::safenormalize(normalI + normalO);
                    const auto length = 1.0f / math::max(0.1f, math::dot(offset, normalI));
                    normal = offset * math::min(length, 2.0f);
                }

                allocation.vertices[i * 2u + 0u] = { color, math::round(pCurr + normal * width * 0.5f), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
                allocation.vertices[i * 2u + 1u] = { color, math::round(pCurr - normal * width * 0.5f), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            }

            for (auto i = 0u, j = 0u; i < count - 1u; ++i, j += 6u)
            {
                allocation.indices[j + 0u] = allocation.vertexOffset + i * 2 + 0u;
                allocation.indices[j + 1u] = allocation.vertexOffset + i * 2 + 1u;
                allocation.indices[j + 2u] = allocation.vertexOffset + i * 2 + 2u;
                allocation.indices[j + 3u] = allocation.vertexOffset + i * 2 + 2u;
                allocation.indices[j + 4u] = allocation.vertexOffset + i * 2 + 1u;
                allocation.indices[j + 5u] = allocation.vertexOffset + i * 2 + 3u;
            }
        }
    }
    
    void GUIDrawList::DrawCurve(const short2* points, const color32& color, uint32_t count, const float width)
    {
        DrawCurve(points, color, color, count, width);
    }

   
    // BEZIER DRAWS
    void GUIDrawList::DrawBezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color0, const color32& color1, float width, float density)
    {
        const auto rmax = math::max(p0, p1);
        const auto rmin = math::min(p0, p1);
        const auto rect = short4(rmin, rmax - rmin);
        const auto clipRect = GetClipRect();

        if (math::intersectRects(rect, clipRect))
        {
            short2 points[65u]{};
            
            auto segments = (uint32_t)(math::distance(float2(p0), float2(p1)) / density);
            segments = math::clamp(segments, 4u, 64u);

            for (auto i = 0u; i <= segments; ++i)
            {
                points[i] = math::cubicBezier(p0, p1, cp0, cp1, (float)i / (float)segments);
            }

            DrawCurve(points, color0, color1, segments + 1u, width);
        }
    }

    void GUIDrawList::DrawBezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color, float width, float density)
    {
        DrawBezierCurve(p0, p1, cp0, cp1, color, color, width, density);
    }

    // RECT DRAWS
    void GUIDrawList::DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex, uint16_t renderMode)
    {
        GUIAllocation allocation;
        if (math::intersectRects(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 4u, 6u, &allocation))
        {
            const auto textureSize = m_allocator->GUIGetTextureSize(textureIndex).xy();
            const auto texelSize = math::rcp(float2(textureSize));
            const auto sminmax = short4(rect.xy(), rect.xy() + rect.zw());
            const auto tminmax = float4(textureRect.xy(), textureRect.xy() + textureRect.zw());
            const auto uvminmax = math::f32tof16(tminmax * texelSize.xyxy);

            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 2u;
            allocation.indices[3u] = allocation.vertexOffset + 2u;
            allocation.indices[4u] = allocation.vertexOffset + 3u;
            allocation.indices[5u] = allocation.vertexOffset + 0u;
            allocation.vertices[0u] = { color, sminmax.xy, uvminmax.xw, textureIndex, renderMode };
            allocation.vertices[1u] = { color, sminmax.xw, uvminmax.xy, textureIndex, renderMode };
            allocation.vertices[2u] = { color, sminmax.zw, uvminmax.zy, textureIndex, renderMode };
            allocation.vertices[3u] = { color, sminmax.zy, uvminmax.zw, textureIndex, renderMode };
        }
    }

    void GUIDrawList::DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex)
    {
        DrawRect(color, rect, textureRect, textureIndex, GUI_RENDER_MODE_DEFAULT);
    }

    void GUIDrawList::DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture, uint16_t renderMode)
    {
        DrawRect(color, rect, textureRect, m_allocator->GUIGetTextureIndex(texture), renderMode);
    }

    void GUIDrawList::DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture)
    {
        DrawRect(color, rect, textureRect, m_allocator->GUIGetTextureIndex(texture), GUI_RENDER_MODE_DEFAULT);
    }

    void GUIDrawList::DrawRect(const color32& color, const short4& rect, uint16_t renderMode)
    {
        DrawRect(color, rect, PK_USHORT4_ZERO, GUI_TEX_INDEX_WHITE, renderMode);
    }

    void GUIDrawList::DrawRect(const color32& color, const short4& rect)
    {
        DrawRect(color, rect, PK_USHORT4_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT);
    }
    
    
    // WIRE RECT DRAWS
    void GUIDrawList::DrawWireRect(const color32& color, const short4& rect, short inset, uint16_t renderMode)
    {
        GUIAllocation allocation;
        if (math::intersectRects(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 8u, 24u, &allocation))
        {
            const auto outer = short4(rect.x, rect.y, rect.x + rect.z, rect.y + rect.w);
            const auto inner = short4(outer.x + inset, outer.y + inset, outer.z - inset, outer.w - inset);

            for (auto i = 0u, j = 0u; i < 4u; ++i)
            {
                const auto base0 = allocation.vertexOffset + i * 2u;
                const auto base1 = allocation.vertexOffset + ((i + 1u) % 4u) * 2u;
                
                allocation.indices[j++] = base0 + 0u;
                allocation.indices[j++] = base0 + 1u;
                allocation.indices[j++] = base1 + 1u;

                allocation.indices[j++] = base1 + 1u;
                allocation.indices[j++] = base1 + 0u;
                allocation.indices[j++] = base0 + 0u;
            }

            allocation.vertices[0u] = { color, outer.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[1u] = { color, inner.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[2u] = { color, outer.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[3u] = { color, inner.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[4u] = { color, outer.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[5u] = { color, inner.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[6u] = { color, outer.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
            allocation.vertices[7u] = { color, inner.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, renderMode };
        }
    }

    void GUIDrawList::DrawWireRect(const color32& color, const short4& rect, short inset)
    {
        DrawWireRect(color, rect, inset, GUI_RENDER_MODE_DEFAULT);
    }


    // TEXT DRAWS
    FontGeometryInfo GUIDrawList::CalculateText(const short4& rect, const char* text, Font* font, const FontStyle& style)
    {
        auto clipRect = GetClipRect();

        if (!style.clip || math::intersectRects(rect, clipRect))
        {
            return font->GenerateGeometryInfo(font, style, text, rect, GetClipRect());
        }

        return FontGeometryInfo();
    }

    FontGeometryInfo GUIDrawList::CalculateText(const short4& rect, const char* text, const FontStyle& style)
    {
        return CalculateText(rect, text, m_allocator->GUIGetDefaultFont(), style);
    }
    
    short4 GUIDrawList::DrawText(const color32& color, const FontGeometryInfo& info)
    {        
        GUIAllocation allocation;
        const auto vertexCount = info.rect_count * 4u;
        const auto indexCount = info.rect_count * 6u;

        if (info.rect_count > 0u && m_allocator->GUIAllocate(m_layer, vertexCount, indexCount, &allocation))
        {
            GUITextContext context;
            context.textureIndex = m_allocator->GUIGetTextureIndex(info.font->GetRHI());
            context.texelSize = info.font->GetTexelSize();
            context.allocation = &allocation;
            context.color = color;

            Font::GenerateRects(info, &context, [](void* userdata, const FontRect& crect, uint32_t index)
            {
                auto* context = static_cast<GUITextContext*>(userdata);
                const auto sminmax = short4(crect.rect.x, crect.rect.y, crect.rect.x + crect.rect.z, crect.rect.y + crect.rect.w);
                const auto tminmax = float4(crect.texrect.x, crect.texrect.y, crect.texrect.x + crect.texrect.z, crect.texrect.y + crect.texrect.w);
                const auto uvminmax = math::f32tof16(tminmax * context->texelSize.xyxy);
                context->allocation->indices[index * 6u + 0u] = context->allocation->vertexOffset + index * 4u + 0u;
                context->allocation->indices[index * 6u + 1u] = context->allocation->vertexOffset + index * 4u + 1u;
                context->allocation->indices[index * 6u + 2u] = context->allocation->vertexOffset + index * 4u + 2u;
                context->allocation->indices[index * 6u + 3u] = context->allocation->vertexOffset + index * 4u + 2u;
                context->allocation->indices[index * 6u + 4u] = context->allocation->vertexOffset + index * 4u + 3u;
                context->allocation->indices[index * 6u + 5u] = context->allocation->vertexOffset + index * 4u + 0u;
                context->allocation->vertices[index * 4u + 0u] = { context->color, sminmax.xy, uvminmax.xy, context->textureIndex, GUI_RENDER_MODE_TEXT };
                context->allocation->vertices[index * 4u + 1u] = { context->color, sminmax.xw, uvminmax.xw, context->textureIndex, GUI_RENDER_MODE_TEXT };
                context->allocation->vertices[index * 4u + 2u] = { context->color, sminmax.zw, uvminmax.zw, context->textureIndex, GUI_RENDER_MODE_TEXT };
                context->allocation->vertices[index * 4u + 3u] = { context->color, sminmax.zy, uvminmax.zy, context->textureIndex, GUI_RENDER_MODE_TEXT };
            });
        }

        return info.text_rect;
    }

    short4 GUIDrawList::DrawText(const color32& color, const short4& rect, const char* text, Font* font, const FontStyle& style)
    {
        const auto info = CalculateText(rect, text, font, style);
        DrawText(color, info);
        return info.text_rect;
    }

    short4 GUIDrawList::DrawText(const color32& color, const short4& rect, const char* text, const FontStyle& style)
    {
        const auto info = CalculateText(rect, text, m_allocator->GUIGetDefaultFont(), style);
        DrawText(color, info);
        return info.text_rect;
    }
}
