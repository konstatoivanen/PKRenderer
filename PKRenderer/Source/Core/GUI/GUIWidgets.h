#pragma once
#include "GUI.h"
#include "Core/Rendering/Font.h"
#include "Core/Base/Containers/InitializerList.h"

namespace PK
{
    struct GUIStyle
    {
        FontStyle fontStyle;
        short2 fontPadding;
        short2 padding;
        color32 colorBg;
        color32 colorFg;
        color32 colorHoverBg;
        color32 colorHoverFg;

        constexpr static GUIStyle GetHeaderRed()
        {
            return
            {
                .fontStyle = { PK_FLOAT2_UP, PK_FLOAT2_ONE, 16.0f, false, false },
                .fontPadding = short2(6,4),
                .padding = short2(4,4),
                .colorBg = color32(192,0,0,255),
                .colorFg = color32(32,32,32,255),
                .colorHoverBg = color32(192,0,0,255),
                .colorHoverFg = color32(192,192,192,255),
            };
        }

        constexpr static GUIStyle GetHeaderDark()
        {
            return
            {
                .fontStyle = { PK_FLOAT2_UP, PK_FLOAT2_ONE, 16.0f, false, false },
                .fontPadding = short2(6,4),
                .padding = short2(4,4),
                .colorBg = color32(32,32,32,255),
                .colorFg = color32(192,192,192,255),
                .colorHoverBg = color32(192,192,192,255),
                .colorHoverFg = color32(32,32,32,255)
            };
        }

        constexpr static GUIStyle GetNormalRed()
        {
            return
            {
                .fontStyle = { PK_FLOAT2_ZERO, PK_FLOAT2_ONE, 16.0f, false, false },
                .fontPadding = short2(4,2),
                .padding = short2(4,4),
                .colorBg = color32(192,0,0,255),
                .colorFg = color32(32,32,32,255),
                .colorHoverBg = color32(32,32,32,255),
                .colorHoverFg = color32(192,0,0,255),
            };
        }

        constexpr static GUIStyle GetNormalDark()
        {
            return
            {
                .fontStyle = { PK_FLOAT2_ZERO, PK_FLOAT2_ONE, 16.0f, false, false },
                .fontPadding = short2(4,2),
                .padding = short2(4,4),
                .colorBg = color32(32,32,32,255),
                .colorFg = color32(192,0,0,255),
                .colorHoverBg = color32(32,32,32,255),
                .colorHoverFg = color32(192,0,0,255),
            };
        }
    };

    struct GUIWindowStyle
    {
        GUIStyle headerFocus;
        GUIStyle headerUnfocus;
        GUIStyle normalFocus;
        GUIStyle normalUnfocus;
        short2 minSize;
        float2 align;
        short2 scrollBarSize;
        short contentIndent;
        short headerHeight;

        constexpr static GUIWindowStyle GetRed(const short2& minSize, const float2 align)
        {
            return
            {
                .headerFocus = GUIStyle::GetHeaderRed(),
                .headerUnfocus = GUIStyle::GetHeaderDark(),
                .normalFocus = GUIStyle::GetNormalRed(),
                .normalUnfocus = GUIStyle::GetNormalDark(),
                .minSize = minSize,
                .align = align,
                .scrollBarSize = short2(8,8),
                .contentIndent = 8,
                .headerHeight = 24
            };
        }
    };

    struct GUIButton
    {
        static bool Tab(GUI* gui, const GUIStyle& style, const short4& rect, const char* name, bool* outDrag = nullptr);
        static bool Rect(GUI* gui, const GUIStyle& style, const short4& rect, const char* name, bool* outDrag = nullptr);
        static bool Close(GUI* gui, const GUIStyle& style, const short4& rect, bool* outDrag = nullptr);

        static bool Tab(GUI* gui, const GUIStyle& style, const short2& size, const char* name, bool* outDrag = nullptr);
        static bool Rect(GUI* gui, const GUIStyle& style, const short2& size, const char* name, bool* outDrag = nullptr);
        static bool Close(GUI* gui, const GUIStyle& style, const short2& size, bool* outDrag = nullptr);

        static bool ResizeLowerLeft(GUI* gui, const GUIStyle& style, int16_t padding, int16_t size, int16_t thickness);
        static bool ResizeLowerRight(GUI* gui, const GUIStyle& style, int16_t padding, int16_t size, int16_t thickness);
    };

    struct GUIScrollBar
    {
        static bool Vertical(GUI* gui, 
            const GUIStyle& style, 
            const short4& inputRect, 
            const short4& barRect, 
            int16_t displayHeight, 
            int16_t contentHeight, 
            short2* scrollpos);
        
        static bool Vertical(GUI* gui, const GUIStyle& style, int16_t width, int16_t contentHeight, short2* scrollpos);
    };

    struct GUIWindow
    {
        GUIWindowStyle style;
        short2 scrollpos;
        short2 contentSize;
        short4 rect;
        uint32_t tab;
        bool closing;
    
        static void Begin(GUI* gui, GUIWindow* window, const char* name, initializer_list<const char*>&& tabs);
        static void End(GUI* gui, GUIWindow* window);
    };

}
