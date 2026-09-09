#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Math/Rect.h"
#include "GUIWidgets.h"

namespace PK
{
    bool GUIButton::Tab(GUI* gui, const GUIStyle& style, const short4& rect, const char* name, bool* outDrag)
    {
        gui->PushHash(FixedString128({ name, "_Btn" }).c_str());
        const auto wasPressed = gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        if (outDrag) *outDrag |= gui->GetInput()->HasControl(gui->GetHash());
        const auto rectText = math::rectPad(rect, style.fontPadding);
        const auto colorBg = isHovered ? style.colorHoverBg : style.colorBg;
        const auto colorFg = isHovered ? style.colorHoverFg : style.colorFg;
        gui->GetDrawList()->DentedRect(colorBg, rect, short2(2 * rect.w / 6, rect.w / 6));
        gui->DrawText(colorFg, rectText, name, style.fontStyle);
        gui->PopHash();
        return wasPressed;
    }

    bool GUIButton::Rect(GUI* gui, const GUIStyle& style, const short4& rect, const char* name, bool* outDrag)
    {
        gui->PushHash(FixedString128({ name, "_Btn" }).c_str());
        const auto wasPressed = gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        if (outDrag) *outDrag |= gui->GetInput()->HasControl(gui->GetHash());
        const auto rectText = math::rectPad(rect, style.fontPadding);
        const auto colorBg = isHovered ? style.colorHoverBg : style.colorBg;
        const auto colorFg = isHovered ? style.colorHoverFg : style.colorFg;
        gui->DrawRect(colorBg, rect);
        gui->DrawText(colorFg, rectText, name, style.fontStyle);
        gui->PopHash();
        return wasPressed;
    }

    bool GUIButton::Close(GUI* gui, const GUIStyle& style, const short4& rect, bool* outDrag)
    {
        gui->PushHash("CloseButton");
        const auto wasPressed = gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        if (outDrag) *outDrag |= gui->GetInput()->HasControl(gui->GetHash());
        const auto rectCross = math::rectPad(rect, style.padding);
        const auto thickness = math::max(math::min(rectCross.z, rectCross.w) / 5, 1);
        const auto color = isHovered ? style.colorHoverFg : style.colorFg;
        gui->GetDrawList()->X(color, rectCross, thickness);
        gui->PopHash();
        return wasPressed;
    }

    bool GUIButton::Tab(GUI* gui, const GUIStyle& style, const short2& size, const char* name, bool* outDrag)
    {
        return Tab(gui, style, gui->NextLayoutRect({ 0,0,size }), name, outDrag);
    }

    bool GUIButton::Rect(GUI* gui, const GUIStyle& style, const short2& size, const char* name, bool* outDrag)
    {
        return Rect(gui, style, gui->NextLayoutRect({ 0,0,size }), name, outDrag);
    }

    bool GUIButton::Close(GUI* gui, const GUIStyle& style, const short2& size, bool* outDrag)
    {
        return Close(gui, style, gui->NextLayoutRect({ 0,0, size }), outDrag);
    }

    bool GUIButton::ResizeLowerLeft(GUI* gui, const GUIStyle& style, int16_t padding, int16_t size, int16_t thickness)
    {
        const auto minmax = math::rectToMinMax(gui->GetLayout().outer);
        const auto rect = short4(minmax.x, minmax.w - size, size, size);
        gui->PushHash("ResizeLowerLeft");
        gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto isDragged = gui->GetInput()->HasControl(gui->GetHash());
        const auto colorFg = isHovered ? style.colorHoverBg : style.colorBg;
        gui->GetDrawList()->LowerLeftWedge(colorFg, { rect.x + padding, rect.y - padding, rect.z, rect.w }, thickness);
        gui->PopHash();
        return isDragged;
    }

    bool GUIButton::ResizeLowerRight(GUI* gui, const GUIStyle& style, int16_t padding, int16_t size, int16_t thickness)
    {
        const auto minmax = math::rectToMinMax(gui->GetLayout().outer);
        const auto rect = short4(minmax.z - size, minmax.w - size, size, size);
        gui->PushHash("ResizeLowerRight");
        gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto isDragged = gui->GetInput()->HasControl(gui->GetHash());
        const auto colorFg = isHovered ? style.colorHoverBg : style.colorBg;
        gui->GetDrawList()->LowerRightWedge(colorFg, { rect.x - padding, rect.y - padding, rect.z, rect.w }, thickness);
        gui->PopHash();
        return isDragged;
    }


    bool GUIScrollBar::Vertical(GUI* gui,
        const GUIStyle& style,
        const short4& inputRect,
        const short4& barRect,
        int16_t displayHeight,
        int16_t contentHeight,
        short2* scrollpos)
    {
        if (displayHeight >= contentHeight)
        {
            return false;
        }

        const auto scrollHeight = (int16_t)(contentHeight - displayHeight);
        const auto cursor = gui->GetInput()->GetCursor();

        if (math::rectIntersect(inputRect, cursor))
        {
            scrollpos->y += int16_t(gui->GetInput()->GetScrollDelta().y * 4.0f);
            scrollpos->y = math::clamp(scrollpos->y, (int16_t)0, scrollHeight);
        }
        
        const auto normalizedOffset = (float)scrollpos->y / (float)scrollHeight;
        const auto normalizedHeight = (float)displayHeight / (float)contentHeight;
        const auto barHeight = (int16_t)(barRect.w * normalizedHeight);
        const auto barOffset = (int16_t)((barRect.w - barHeight) * normalizedOffset);
        gui->DrawRect(style.colorBg,{ barRect.x, barRect.y + barOffset, barRect.z, barHeight});
        return true;
    }

    bool GUIScrollBar::Vertical(GUI* gui, const GUIStyle& style, int16_t width, int16_t contentHeight, short2* scrollpos)
    {
        const auto inputRect = gui->GetLayout().cordon;
        const auto displayHeight = inputRect.w;
        const auto rect = gui->NextLayoutRect({0,0,width,0});
        return Vertical(gui, style, inputRect, rect, displayHeight, contentHeight, scrollpos);
    }


    void GUIWindow::Begin(GUI* gui, GUIWindow* window, const char* name, initializer_list<const char*>&& tabs)
    {
        const auto& style = window->style;
        bool dragged = false;

        window->tab = window->tab % tabs.size();

        if (window->rect.z <= 0 || window->rect.w <= 0)
        {
            auto area = gui->GetLayout().inner;
            window->rect.xy = area.xy + short2(area.z * style.align.x, area.w * style.align.y);
            window->rect.zw = style.minSize;
        }

        gui->PushHash(name);
        
        gui->BeginLayout({.mode = GUILayoutMode::Partition, .clampToParent = true }, window->rect);
        window->rect = gui->GetLayout().outer;

        gui->BeginLayout({.mode = GUILayoutMode::Partition}, { 0, style.headerHeight, 0, 0});
        {
            if (GUIButton::Close(gui, style.headerUnfocus, { style.headerHeight, 0 }))
            {
                window->closing = true;
            }

            gui->BeginLayout({ .mode = GUILayoutMode::Grid,.gridsize = { (uint32_t)tabs.size(), 1u} });
            {
                for (auto i = 0u; i < tabs.size(); ++i)
                {
                    const auto styleButton = i == window->tab ? style.headerFocus : style.headerUnfocus;

                    if (GUIButton::Tab(gui, styleButton, PK_SHORT2_ZERO, tabs.begin()[i], &dragged))
                    {
                        window->tab = i;
                    }
                }
            }
            gui->EndLayout();
        }
        gui->EndLayout();

        gui->DrawRect(style.normalFocus.colorBg, gui->GetLayout().cordon);
  
        if (dragged)
        {
            window->rect.xy += gui->GetInput()->GetCursorDelta();
        }

        if (GUIButton::ResizeLowerRight(gui, style.headerUnfocus, 2, 8, 3))
        {
            window->rect.zw += math::max(style.minSize - window->rect.zw, gui->GetInput()->GetCursorDelta());
        }

        if (GUIButton::ResizeLowerLeft(gui, style.headerUnfocus, 2, 8, 3))
        {
            const auto offsetx = math::min((int16_t)(window->rect.z - style.minSize.x), gui->GetInput()->GetCursorDelta().x);
            const auto offsety = math::max((int16_t)(style.minSize.y - window->rect.w), gui->GetInput()->GetCursorDelta().y);
            window->rect.x += offsetx;
            window->rect.z -= offsetx;
            window->rect.w += offsety;
        }

        GUIScrollBar::Vertical(gui, style.normalFocus, style.scrollBarSize.x, window->contentSize.y, &window->scrollpos);

        gui->BeginLayout({ .mode = GUILayoutMode::Rows, .padding = { style.contentIndent,0,0,0} }, { window->scrollpos, 0, 0 });
        gui->PushClipRect(gui->GetLayout().cordon);
    }
            
    void GUIWindow::End(GUI* gui, GUIWindow* window)
    {
        window->contentSize = gui->GetLayout().content.zw;
        gui->PopClipRect();
        gui->EndLayout();

       // window->rect = gui->GetLayout().outer;
        gui->EndLayout();
        gui->PopHash();
    }
}