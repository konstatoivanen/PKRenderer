#pragma once
#include "GUI.h"
#include "Core/Rendering/Font.h"
#include "Core/Base/Containers/InitializerList.h"

namespace PK
{
    struct GUIStyle
    {
        FontStyle fontStyle;
        short4 padding;
        color32 colorBg;
        color32 colorFg;
        color32 colorHoverBg;
        color32 colorHoverFg;
    };

    struct GUIWindowStyle
    {
        GUIStyle headerFocus;
        GUIStyle headerUnfocus;
        GUIStyle label;
        GUIStyle field;
        GUILayoutMode contentMode;
        short4 contentPadding;
        short2 initialSize;
        short2 minSize;
        short2 maxSize;
        float2 align;
        short2 scrollBarSize;
        short headerHeight;

        constexpr static GUIWindowStyle GetRedDark()
        {
            return
            {
                .headerFocus = 
                {
                    .fontStyle = { PK_FLOAT2_UP, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = short4(12,4,6,4),
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,192,192,255),
                    .colorHoverBg = color32(192,192,192,255),
                    .colorHoverFg = color32(32,32,32,255),
                },
                .headerUnfocus = 
                {
                    .fontStyle = { PK_FLOAT2_UP, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = short4(6,4,6,4),
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,0,0,255),
                    .colorHoverBg = color32(192,192,192,255),
                    .colorHoverFg = color32(32,32,32,255)
                },
                .label = 
                {
                    .fontStyle = { PK_FLOAT2_ZERO, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = short4(0,0,4,0),
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,192,192,255),
                    .colorHoverBg = color32(32,32,32,255),
                    .colorHoverFg = color32(192,0,0,255),
                },
                .field = 
                {
                    .fontStyle = { PK_FLOAT2_ZERO, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = PK_SHORT4_ZERO,
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,0,0,255),
                    .colorHoverBg = color32(32,32,32,255),
                    .colorHoverFg = color32(192,192,192,255),
                },
                .contentMode = GUILayoutMode::Rows,
                .contentPadding = short4(0,6,0,6),
                .scrollBarSize = short2(8,8),
                .headerHeight = 24
            };
        }


        constexpr static GUIWindowStyle GetRed()
        {
            return
            {
                .headerFocus =
                {
                    .fontStyle = { PK_FLOAT2_UP, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = short4(6,4,6,4),
                    .colorBg = color32(192,0,0,255),
                    .colorFg = color32(32,32,32,255),
                    .colorHoverBg = color32(192,0,0,255),
                    .colorHoverFg = color32(192,192,192,255),
                },
                .headerUnfocus =
                {
                    .fontStyle = { PK_FLOAT2_UP, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = short4(6,4,6,4),
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,192,192,255),
                    .colorHoverBg = color32(192,192,192,255),
                    .colorHoverFg = color32(32,32,32,255)
                },
                .label =
                {
                    .fontStyle = { PK_FLOAT2_ZERO, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = short4(0,0,4,0),
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,192,192,255),
                    .colorHoverBg = color32(32,32,32,255),
                    .colorHoverFg = color32(192,0,0,255),
                },
                .field =
                {
                    .fontStyle = { PK_FLOAT2_ZERO, PK_FLOAT2_ONE, 16.0f, false, false },
                    .padding = PK_SHORT4_ZERO,
                    .colorBg = color32(32,32,32,255),
                    .colorFg = color32(192,0,0,255),
                    .colorHoverBg = color32(32,32,32,255),
                    .colorHoverFg = color32(192,192,192,255),
                },
                .contentMode = GUILayoutMode::Rows,
                .contentPadding = short4(0,6,0,6),
                .scrollBarSize = short2(8,8),
                .headerHeight = 24
            };
        }
    };

    struct GUILabel
    {
        static void Fit(GUI* gui, const char* text, int16_t align, const GUIStyle& style);
    };

    struct GUIButton
    {
        static bool DragTab(GUI* gui, const char* name, short4* target, const short4& rect, const GUIStyle& style);
        static bool DragTab(GUI* gui, const char* name, short4* target, const short2& size, const GUIStyle& style);
       
        static bool Rect(GUI* gui, const char* name, const short4& rect, const GUIStyle& style);
        static bool Rect(GUI* gui, const char* name, const short2& size, const GUIStyle& style);
        
        static bool Close(GUI* gui, const short4& rect, const GUIStyle& style);
        static bool Close(GUI* gui, const short2& size, const GUIStyle& style);

        static bool ResizeLowerLeft(GUI* gui, short4* target, int16_t padding, int16_t size, int16_t thickness, const GUIStyle& style);
        static bool ResizeLowerRight(GUI* gui, short4* target, int16_t padding, int16_t size, int16_t thickness, const GUIStyle& style);
    };

    struct GUIScrollBar
    {
        static bool Vertical(GUI* gui, 
            short2* scrollpos,
            int16_t displayHeight, 
            int16_t contentHeight, 
            const short4& inputRect, 
            const short4& barRect, 
            const GUIStyle& style);
        
        static bool Vertical(GUI* gui, short2* scrollpos, int16_t width, int16_t contentHeight, const GUIStyle& style);
    };

    struct GUIWindow
    {
        GUIWindowStyle style;
        short2 scrollpos;
        short2 contentSize;
        short4 rect;
        uint32_t tab;
        bool requestsClose;
    
        static void Begin(GUI* gui, GUIWindow* window, const char* name, initializer_list<const char*>&& tabs);
        static void Begin(GUI* gui, GUIWindow* window, const char* name);
        static void End(GUI* gui, GUIWindow* window);
    };

}
