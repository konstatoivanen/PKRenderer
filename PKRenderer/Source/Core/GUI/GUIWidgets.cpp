#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Math/Rect.h"
#include "GUIWidgets.h"

namespace PK
{
    void GUILabel::Fit(GUI* gui, const char* text, int16_t align, const GUIStyle& style)
    {
        const auto& layout = gui->GetLayout();
        auto info = gui->CalculateText(layout.cordon, text, style.fontStyle);
        auto rect = PK_SHORT4_ZERO;
        rect.z = info.text_rect.z + style.padding.x + style.padding.z;
        rect.w = info.line_height + style.padding.y + style.padding.w;
        rect.z = math::align(rect.z, align);
        info.area_rect = gui->NextLayoutRect(rect);
        info.area_rect = math::rectPad(info.area_rect, style.padding);
        gui->GetDrawList()->Text(style.colorFg, info);
    }


    bool GUIButton::DragTab(GUI* gui, const char* name, short4* target, const short4& rect, const GUIStyle& style)
    {
        gui->PushHash(FixedString128({ name, "_Btn" }).c_str());
        
        auto offset = PK_SHORT2_ZERO;
        const auto wasPressed = gui->GetInput()->ButtonDrag(rect, gui->GetHash(), &offset);

        if (target)
        {
            target->xy += offset;
        }
        
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto rectText = math::rectPad(rect, style.padding);
        const auto colorBg = isHovered ? style.colorHoverBg : style.colorBg;
        const auto colorFg = isHovered ? style.colorHoverFg : style.colorFg;
        gui->GetDrawList()->DentedRect(colorBg, rect, short2(2 * rect.w / 6, rect.w / 6));
        gui->DrawText(colorFg, rectText, name, style.fontStyle);
        gui->PopHash();
        return wasPressed;
    }
    
    bool GUIButton::DragTab(GUI* gui, const char* name, short4* target, const short2& size, const GUIStyle& style)
    {
        return DragTab(gui, name, target, gui->NextLayoutRect({ 0,0,size }), style);
    }


    bool GUIButton::Rect(GUI* gui, const char* name, const short4& rect, const GUIStyle& style)
    {
        gui->PushHash(FixedString128({ name, "_Btn" }).c_str());
        const auto wasPressed = gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto rectText = math::rectPad(rect, style.padding);
        const auto colorBg = isHovered ? style.colorHoverBg : style.colorBg;
        const auto colorFg = isHovered ? style.colorHoverFg : style.colorFg;
        gui->DrawRect(colorBg, rect);
        gui->DrawText(colorFg, rectText, name, style.fontStyle);
        gui->PopHash();
        return wasPressed;
    }
    
    bool GUIButton::Rect(GUI* gui, const char* name, const short2& size, const GUIStyle& style)
    {
        return Rect(gui, name, gui->NextLayoutRect({ 0,0,size }), style);
    }

    
    bool GUIButton::Close(GUI* gui, const short4& rect, const GUIStyle& style)
    {
        gui->PushHash("CloseButton");
        const auto wasPressed = gui->GetInput()->Button(rect, gui->GetHash());
        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto thickness = (int16_t)math::max(math::min(rect.z, rect.w) / 6, 1);
        const auto rectCross = math::rectPad(rect, thickness);
        const auto color = isHovered ? style.colorHoverFg : style.colorFg;
        gui->GetDrawList()->X(color, rectCross, thickness);
        gui->PopHash();
        return wasPressed;
    }

    bool GUIButton::Close(GUI* gui, const short2& size, const GUIStyle& style)
    {
        return Close(gui, gui->NextLayoutRect({ 0,0, size }), style);
    }


    bool GUIButton::ResizeLowerLeft(GUI* gui, short4* target, int16_t padding, int16_t size, int16_t thickness, const GUIStyle& style)
    {
        const auto minmax = math::rectToMinMax(gui->GetLayout().outer);
        const auto rect = short4(minmax.x, minmax.w - size, size, size);

        gui->PushHash("ResizeLowerLeft");
        
        auto offset = PK_SHORT2_ZERO;
        const auto wasPressed = gui->GetInput()->ButtonDrag(rect, gui->GetHash(), &offset);
        
        target->x += offset.x;
        target->z -= offset.x;
        target->w += offset.y;

        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto colorFg = isHovered ? style.colorHoverFg : style.colorFg;
        gui->GetDrawList()->LowerLeftWedge(colorFg, { rect.x + padding, rect.y - padding, rect.z, rect.w }, thickness);
        gui->PopHash();

        return wasPressed;
    }

    bool GUIButton::ResizeLowerRight(GUI* gui, short4* target, int16_t padding, int16_t size, int16_t thickness, const GUIStyle& style)
    {
        const auto minmax = math::rectToMinMax(gui->GetLayout().outer);
        const auto rect = short4(minmax.z - size, minmax.w - size, size, size);
        
        gui->PushHash("ResizeLowerRight");

        auto offset = PK_SHORT2_ZERO;
        const auto wasPressed = gui->GetInput()->ButtonDrag(rect, gui->GetHash(), &offset);

        target->zw += offset;

        const auto isHovered = gui->GetInput()->HasHover(gui->GetHash());
        const auto colorFg = isHovered ? style.colorHoverFg : style.colorFg;
        gui->GetDrawList()->LowerRightWedge(colorFg, { rect.x - padding, rect.y - padding, rect.z, rect.w }, thickness);
        gui->PopHash();

        return wasPressed;
    }


    bool GUIScrollBar::Vertical(GUI* gui,
        short2* scrollpos,
        int16_t displayHeight,
        int16_t contentHeight,
        const short4& inputRect,
        const short4& barRect,
        const GUIStyle& style)
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

    bool GUIScrollBar::Vertical(GUI* gui, short2* scrollpos, int16_t width, int16_t contentHeight, const GUIStyle& style)
    {
        const auto inputRect = gui->GetLayout().cordon;
        const auto displayHeight = inputRect.w;
        const auto rect = gui->NextLayoutRect({0,0,width,0});
        return Vertical(gui, scrollpos, displayHeight, contentHeight, inputRect, rect, style);
    }


    void GUIWindow::Begin(GUI* gui, GUIWindow* window, const char* name, initializer_list<const char*>&& tabs)
    {
        const auto& style = window->style;
        const auto& area = gui->GetLayout().local;

        window->tab = window->tab % tabs.size();

        if (window->rect.z <= 0 || window->rect.w <= 0)
        {
            window->rect.xy = area.xy + short2(area.z * style.align.x, area.w * style.align.y);
            window->rect.zw = style.initialSize;
            window->rect.zw = math::clamp(window->rect.zw(), style.minSize, style.maxSize);
        }

        gui->PushHash(name);
        
        gui->BeginLayout({.mode = GUILayoutMode::Partition, .clampToParent = true }, window->rect);
        
        window->rect = gui->GetLayout().local;

        gui->BeginLayout({.mode = GUILayoutMode::Partition}, { 0, style.headerHeight, 0, 0});
        {
            window->requestsClose = GUIButton::Close(gui, { style.headerHeight, 0 }, style.field);

            gui->BeginLayout({ .mode = GUILayoutMode::Grid,.gridsize = { (uint32_t)tabs.size(), 1u} });
            {
                for (auto i = 0u; i < tabs.size(); ++i)
                {
                    const auto styleButton = i == window->tab ? style.headerFocus : style.headerUnfocus;

                    if (GUIButton::DragTab(gui, tabs.begin()[i], &window->rect, PK_SHORT2_ZERO, styleButton))
                    {
                        window->tab = i;
                    }
                }
            }
            gui->EndLayout();
        }
        gui->EndLayout();

        gui->DrawRect(style.label.colorBg, gui->GetLayout().cordon);

        if (math::any(window->style.maxSize > window->style.minSize))
        {
            auto resize = math::rectClamp(window->rect, area);

            GUIButton::ResizeLowerRight(gui, &resize, 3, 8, 4, style.field);
            GUIButton::ResizeLowerLeft(gui, &resize, 3, 8, 4, style.field);

            resize = math::rectClip(resize, area);

            if (resize.z < window->style.minSize.x)
            {
                resize.x = window->rect.x;
                resize.z = window->rect.z;
            }

            if (resize.w < window->style.minSize.y)
            {
                resize.y = window->rect.y;
                resize.w = window->rect.w;
            }

            if (resize.z > window->style.maxSize.x)
            {
                resize.x = window->rect.x;
                resize.z = window->rect.z;
            }

            if (resize.w > window->style.maxSize.y)
            {
                resize.y = window->rect.y;
                resize.w = window->rect.w;
            }

            window->rect = resize;
        }

        GUIScrollBar::Vertical(gui, &window->scrollpos, style.scrollBarSize.x, window->contentSize.y, style.field);

        gui->BeginLayout({ .mode = style.contentMode, .padding = { 14, 6, 6, 12 } }, { window->scrollpos, 0, 0 });
        gui->PushClipRect(gui->GetLayout().cordon);
        gui->PushLayer();
    }

    void GUIWindow::Begin(GUI* gui, GUIWindow* window, const char* name)
    {
        Begin(gui, window, name, { name });
    }
            
    void GUIWindow::End(GUI* gui, GUIWindow* window)
    {
        window->contentSize = gui->GetLayout().content.zw;
        gui->PopLayer();
        gui->PopClipRect();
        gui->EndLayout();
        gui->EndLayout();
        gui->PopHash();
    }

}