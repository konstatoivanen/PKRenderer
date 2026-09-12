#include "PrecompiledHeader.h"
#include "Core/Math/Extended.h"
#include "Core/Math/Rect.h"
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


    GUIDrawList::GUIDrawList(IGUIAllocator* allocator, const short4& initialClipRect) :
        m_allocator(allocator) 
    {
        m_clipStack[0] = initialClipRect;
        m_clipIndices[0] = 0u;
    }

    void GUIDrawList::PushClipRect(const short4& rect)
    {
        if (m_clipHead < CLIP_STACK_MAX)
        {
            const auto clipRect = GetClipRect();
            const auto newRect = math::rectClip(rect, clipRect);
            m_clipIndices[++m_clipHead] = ++m_clipIndex;
            m_clipStack[m_clipHead] = newRect;
        }
    }

    void GUIDrawList::PopClipRect()
    {
        if (m_clipHead)
        {
            --m_clipHead;
        }
    }

    short4 GUIDrawList::GetClipRect() const
    {
        return m_clipStack[m_clipHead];
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

    uint16_t GUIDrawList::GetLayer() const
    {
        return m_layer;
    }


    void GUIDrawList::Triangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c)
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

    void GUIDrawList::Line(const short2& p0, const short2& p1, const color32& color0, const color32& color1, const float width)
    {
        const auto rect = math::rectFromLine(p0, p1);

        if (math::rectIntersect(rect, GetClipRect()))
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

    void GUIDrawList::Curve(const short2* points, const color32& color0, const color32& color1, uint32_t count, const float width)
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
                    const auto length = 1.0f / math::max(0.25f, math::dot(offset, normalI));
                    normal = offset * length;
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

    void GUIDrawList::BezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color0, const color32& color1, float width, float density)
    {
        const auto rect = math::rectFromLine(p0, p1);
        const auto clipRect = GetClipRect();

        if (math::rectIntersect(rect, clipRect))
        {
            short2 points[65u]{};

            auto segments = (uint32_t)(math::distance(float2(p0), float2(p1)) / density);
            segments = math::clamp(segments, 4u, 64u);

            for (auto i = 0u; i <= segments; ++i)
            {
                points[i] = math::cubicBezier(p0, p1, cp0, cp1, (float)i / (float)segments);
            }

            Curve(points, color0, color1, segments + 1u, width);
        }
    }

    void GUIDrawList::Rect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex, uint16_t renderMode)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 4u, 6u, &allocation))
        {
            const auto textureSize = m_allocator->GUIGetTextureSize(textureIndex).xy();
            const auto texelSize = math::rcp(float2(textureSize));
            const auto sminmax = math::rectToMinMax(rect);
            const auto tminmax = float4(math::rectToMinMax(textureRect));
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

    void GUIDrawList::Rect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture, uint16_t renderMode)
    {
        Rect(color, rect, textureRect, m_allocator->GUIGetTextureIndex(texture), renderMode);
    }

    void GUIDrawList::WireRect(const color32& color, const short4& rect, short inset)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 8u, 24u, &allocation))
        {
            const auto outer = math::rectToMinMax(rect);
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

            allocation.vertices[0u] = { color, outer.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, inner.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, outer.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, inner.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, outer.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, inner.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[6u] = { color, outer.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[7u] = { color, inner.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::DentedRect(const color32& color, const short4& rect, short2 dent)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto minmax = math::rectToMinMax(rect);
            allocation.indices[0] = allocation.vertexOffset + 0u;
            allocation.indices[1] = allocation.vertexOffset + 4u;
            allocation.indices[2] = allocation.vertexOffset + 5u;
            allocation.indices[3] = allocation.vertexOffset + 0u;
            allocation.indices[4] = allocation.vertexOffset + 1u;
            allocation.indices[5] = allocation.vertexOffset + 4u;
            allocation.indices[6] = allocation.vertexOffset + 1u;
            allocation.indices[7] = allocation.vertexOffset + 3u;
            allocation.indices[8] = allocation.vertexOffset + 4u;
            allocation.indices[9] = allocation.vertexOffset + 1u;
            allocation.indices[10] = allocation.vertexOffset + 2u;
            allocation.indices[11] = allocation.vertexOffset + 3u;
            allocation.vertices[0u] = { color, minmax.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, minmax.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, minmax.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, minmax.zy + short2(-dent.x, 0), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, minmax.zy + short2(-dent.x / 2, dent.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, minmax.zy + short2(0, dent.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::DentedWireRect(const color32& color, const short4& rect, short inset, short2 dent)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 12u, 36u, &allocation))
        {
            const auto minmax = math::rectToMinMax(rect);
            const auto bias = (inset + 1) / 2;
            allocation.vertices[0u] = { color, minmax.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, minmax.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, minmax.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, minmax.zy + short2(-dent.x, 0), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, minmax.zy + short2(-dent.x / 2, dent.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, minmax.zy + short2(0, dent.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[6u] = { color, minmax.zw + short2(-inset,-inset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[7u] = { color, minmax.xw + short2(inset,-inset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[8u] = { color, minmax.xy + short2(inset,inset), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[9u] = { color, minmax.zy + short2(0,inset) + short2(-dent.x - bias, 0), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[10u] = { color, minmax.zy + short2(0,inset) + short2(-dent.x / 2 - bias, dent.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[11u] = { color, minmax.zy + short2(-inset,inset) + short2(0, dent.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };

            for (auto i = 0u; i < 6u; ++i)
            {
                const auto v0 = i;
                const auto v1 = (i + 1u) % 6u;
                const auto v2 = v1 + 6u;
                const auto v3 = v0 + 6u;
                allocation.indices[i * 6u + 0u] = allocation.vertexOffset + v0;
                allocation.indices[i * 6u + 1u] = allocation.vertexOffset + v1;
                allocation.indices[i * 6u + 2u] = allocation.vertexOffset + v2;
                allocation.indices[i * 6u + 3u] = allocation.vertexOffset + v2;
                allocation.indices[i * 6u + 4u] = allocation.vertexOffset + v3;
                allocation.indices[i * 6u + 5u] = allocation.vertexOffset + v0;
            }
        }
    }

    void GUIDrawList::X(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 12u, 30u, &allocation))
        {
            const auto outer = math::rectToMinMax(rect);
            const auto inner = short4(outer.xy + thickness, outer.zw - thickness);
            const auto center = rect.xy + (rect.zw / (short)2);
            const auto delta = rect.zw - thickness;
            const auto middle = delta - (rect.zw / ((short)2));
            const auto interection = (float2(middle) + 0.5f) / float2(delta);
            const auto offset = short2(delta.x * interection.y, delta.y * interection.x);

            allocation.vertices[0u] = { color, short2(outer.x, inner.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, short2(inner.x, outer.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, short2(center.x, outer.y + offset.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, short2(inner.z, outer.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, short2(outer.z, inner.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, short2(outer.z - offset.x, center.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[6u] = { color, short2(outer.z, inner.w), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[7u] = { color, short2(inner.z, outer.w), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[8u] = { color, short2(center.x, outer.w - offset.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[9u] = { color, short2(inner.x, outer.w), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[10u] = { color, short2(outer.x, inner.w), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[11u] = { color, short2(outer.x + offset.x, center.y), PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };

            for (auto i = 0u; i < 4u; ++i)
            {
                allocation.indices[i * 6 + 0u] = allocation.vertexOffset + i * 3u + 0u;
                allocation.indices[i * 6 + 1u] = allocation.vertexOffset + i * 3u + 1u;
                allocation.indices[i * 6 + 2u] = allocation.vertexOffset + i * 3u + 2u;
                allocation.indices[i * 6 + 3u] = allocation.vertexOffset + i * 3u + 2u;
                allocation.indices[i * 6 + 4u] = allocation.vertexOffset + (i * 3u + 11u) % 12u;
                allocation.indices[i * 6 + 5u] = allocation.vertexOffset + i * 3u + 0u;
            }

            allocation.indices[24u] = allocation.vertexOffset + 2u;
            allocation.indices[25u] = allocation.vertexOffset + 5u;
            allocation.indices[26u] = allocation.vertexOffset + 8u;
            allocation.indices[27u] = allocation.vertexOffset + 8u;
            allocation.indices[28u] = allocation.vertexOffset + 11u;
            allocation.indices[29u] = allocation.vertexOffset + 2u;
        }
    }

    void GUIDrawList::LowerRightTriangle(const color32& color, const short4& rect)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 3u, 3u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 2u;
            allocation.vertices[0u] = { color, sminmax.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, sminmax.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, sminmax.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::LowerLeftTriangle(const color32& color, const short4& rect)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 3u, 3u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 2u;
            allocation.vertices[0u] = { color, sminmax.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, sminmax.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, sminmax.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::UpperRightTriangle(const color32& color, const short4& rect)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 3u, 3u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 2u;
            allocation.vertices[0u] = { color, sminmax.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, sminmax.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, sminmax.zw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::UpperLeftTriangle(const color32& color, const short4& rect)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 3u, 3u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 2u;
            allocation.vertices[0u] = { color, sminmax.xw, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, sminmax.xy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, sminmax.zy, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::LowerRightWedge(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);

            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 4u;

            allocation.indices[3u] = allocation.vertexOffset + 4u;
            allocation.indices[4u] = allocation.vertexOffset + 5u;
            allocation.indices[5u] = allocation.vertexOffset + 0u;

            allocation.indices[6u] = allocation.vertexOffset + 1u;
            allocation.indices[7u] = allocation.vertexOffset + 2u;
            allocation.indices[8u] = allocation.vertexOffset + 3u;

            allocation.indices[9u] = allocation.vertexOffset + 3u;
            allocation.indices[10u] = allocation.vertexOffset + 4u;
            allocation.indices[11u] = allocation.vertexOffset + 1u;

            allocation.vertices[0u] = { color, { sminmax.z, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, { sminmax.z, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, { sminmax.x, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, { sminmax.x, sminmax.w - thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, { sminmax.z - thickness, sminmax.w - thickness  }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, { sminmax.z - thickness, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::LowerLeftWedge(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 4u;
            allocation.indices[2u] = allocation.vertexOffset + 1u;
            allocation.indices[3u] = allocation.vertexOffset + 4u;
            allocation.indices[4u] = allocation.vertexOffset + 0u;
            allocation.indices[5u] = allocation.vertexOffset + 5u;
            allocation.indices[6u] = allocation.vertexOffset + 1u;
            allocation.indices[7u] = allocation.vertexOffset + 3u;
            allocation.indices[8u] = allocation.vertexOffset + 2u;
            allocation.indices[9u] = allocation.vertexOffset + 3u;
            allocation.indices[10u] = allocation.vertexOffset + 1u;
            allocation.indices[11u] = allocation.vertexOffset + 4u;
            allocation.vertices[0u] = { color, { sminmax.x, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, { sminmax.x, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, { sminmax.z, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, { sminmax.z, sminmax.w - thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, { sminmax.x + thickness, sminmax.w - thickness  }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, { sminmax.x + thickness, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::RightWedge(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            const auto center = rect.xy + rect.zw / (short)2;
            
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 4u;

            allocation.indices[3u] = allocation.vertexOffset + 4u;
            allocation.indices[4u] = allocation.vertexOffset + 5u;
            allocation.indices[5u] = allocation.vertexOffset + 0u;
            
            allocation.indices[6u] = allocation.vertexOffset + 1u;
            allocation.indices[7u] = allocation.vertexOffset + 2u;
            allocation.indices[8u] = allocation.vertexOffset + 3u;
            
            allocation.indices[9u] = allocation.vertexOffset + 3u;
            allocation.indices[10u] = allocation.vertexOffset + 4u;
            allocation.indices[11u] = allocation.vertexOffset + 1u;

            allocation.vertices[0u] = { color, { center.x, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, { sminmax.z, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, { center.x, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, { center.x - thickness, sminmax.w - thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, { sminmax.z - thickness * 2, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, { center.x - thickness, sminmax.y + thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::LeftWedge(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            const auto center = rect.xy + rect.zw / (short)2;
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 4u;
            allocation.indices[2u] = allocation.vertexOffset + 1u;
            allocation.indices[3u] = allocation.vertexOffset + 4u;
            allocation.indices[4u] = allocation.vertexOffset + 0u;
            allocation.indices[5u] = allocation.vertexOffset + 5u;
            allocation.indices[6u] = allocation.vertexOffset + 1u;
            allocation.indices[7u] = allocation.vertexOffset + 3u;
            allocation.indices[8u] = allocation.vertexOffset + 2u;
            allocation.indices[9u] = allocation.vertexOffset + 3u;
            allocation.indices[10u] = allocation.vertexOffset + 1u;
            allocation.indices[11u] = allocation.vertexOffset + 4u;
            allocation.vertices[0u] = { color, { center.x, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, { sminmax.x, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, { center.x, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, { center.x + thickness, sminmax.w - thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, { sminmax.x + thickness * 2, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, { center.x + thickness, sminmax.y + thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::UpWedge(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            const auto center = rect.xy + rect.zw / (short)2;
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 1u;
            allocation.indices[2u] = allocation.vertexOffset + 4u;
            allocation.indices[3u] = allocation.vertexOffset + 4u;
            allocation.indices[4u] = allocation.vertexOffset + 5u;
            allocation.indices[5u] = allocation.vertexOffset + 0u;
            allocation.indices[6u] = allocation.vertexOffset + 1u;
            allocation.indices[7u] = allocation.vertexOffset + 2u;
            allocation.indices[8u] = allocation.vertexOffset + 3u;
            allocation.indices[9u] = allocation.vertexOffset + 3u;
            allocation.indices[10u] = allocation.vertexOffset + 4u;
            allocation.indices[11u] = allocation.vertexOffset + 1u;
            allocation.vertices[0u] = { color, { sminmax.x, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, { center.x, sminmax.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, { sminmax.z, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, { sminmax.z - thickness, center.y + thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, { center.x, sminmax.y + thickness * 2 }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, { sminmax.x + thickness, center.y + thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    void GUIDrawList::DownWedge(const color32& color, const short4& rect, short thickness)
    {
        GUIAllocation allocation;
        if (math::rectIntersect(rect, GetClipRect()) && m_allocator->GUIAllocate(m_layer, 6u, 12u, &allocation))
        {
            const auto sminmax = math::rectToMinMax(rect);
            const auto center = rect.xy + rect.zw / (short)2;
            allocation.indices[0u] = allocation.vertexOffset + 0u;
            allocation.indices[1u] = allocation.vertexOffset + 4u;
            allocation.indices[2u] = allocation.vertexOffset + 1u;
            allocation.indices[3u] = allocation.vertexOffset + 4u;
            allocation.indices[4u] = allocation.vertexOffset + 0u;
            allocation.indices[5u] = allocation.vertexOffset + 5u;
            allocation.indices[6u] = allocation.vertexOffset + 1u;
            allocation.indices[7u] = allocation.vertexOffset + 3u;
            allocation.indices[8u] = allocation.vertexOffset + 2u;
            allocation.indices[9u] = allocation.vertexOffset + 3u;
            allocation.indices[10u] = allocation.vertexOffset + 1u;
            allocation.indices[11u] = allocation.vertexOffset + 4u;
            allocation.vertices[0u] = { color, { sminmax.x, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[1u] = { color, { center.x, sminmax.w }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[2u] = { color, { sminmax.z, center.y }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[3u] = { color, { sminmax.z - thickness, center.y - thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[4u] = { color, { center.x, sminmax.w - thickness * 2 }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
            allocation.vertices[5u] = { color, { sminmax.x + thickness, center.y - thickness }, PK_USHORT2_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT };
        }
    }

    FontGeometryInfo GUIDrawList::CalculateText(const short4& rect, const char* text, Font* font, const FontStyle& style)
    {
        auto clipRect = GetClipRect();

        if (!style.clip || math::rectIntersect(rect, clipRect))
        {
            return font->GenerateGeometryInfo(font, style, text, rect, GetClipRect());
        }

        return FontGeometryInfo();
    }

    short4 GUIDrawList::Text(const color32& color, const FontGeometryInfo& info)
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
                const auto sminmax = math::rectToMinMax(crect.rect);
                const auto tminmax = float4(math::rectToMinMax(crect.texrect));
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

    short4 GUIDrawList::Text(const color32& color, const short4& rect, const char* text, Font* font, const FontStyle& style)
    {
        const auto info = CalculateText(rect, text, font, style);
        Text(color, info);
        return info.text_rect;
    }
}
