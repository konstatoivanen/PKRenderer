#pragma once
#include "IGUIAllocator.h"
#include "GUIDrawList.h"
#include "GUIHashStack.h"
#include "GUILayoutStack.h"
#include "GUIInput.h"

namespace PK
{
    struct GUIContext
    {
        const GUIKeys* keys = nullptr;
        IGUIAllocator* allocator = nullptr;
        InputState* input = nullptr;
        Font* defautFont = nullptr;

        GUIInputState inputState{};
        short4 clipRect = PK_SHORT4_ZERO;
        short4 renderArea = PK_SHORT4_ZERO;
        float2 screenScale = PK_FLOAT2_ZERO;
        float2 screenOffset = PK_FLOAT2_ZERO;
    };

    struct GUI
    {
        GUI(GUIContext* ctx) :
            m_ctx(ctx),
            m_hashStack(),
            m_layoutStack(ctx->renderArea),
            m_drawList(ctx->allocator, ctx->clipRect),
            m_input(ctx->keys, &ctx->inputState, ctx->input, ctx->screenOffset, ctx->screenScale)
        {
        }

        template<typename T>
        T* AllocateState() { return m_ctx->allocator->GUIAllocateState<T>(GetHash()); }

        inline GUIHashStack* GetHashStack() { return &m_hashStack; }
        inline GUILayoutStack* GetLayoutStack() { return &m_layoutStack; }
        inline GUIDrawList* GetDrawList() { return &m_drawList; }
        inline GUIInput* GetInput() { return &m_input; }

        inline void PushHash(const char* name) { m_hashStack.Push(name); }
        inline void PopHash() { m_hashStack.Pop(); }
        inline uint64_t GetHash() const { return m_hashStack.Get(); }

        inline void PushClipRect(const short4& rect) 
        {
            m_drawList.PushClipRect(rect); 
            m_input.SetClipRect(m_drawList.GetClipRect());
        }

        inline void PopClipRect() 
        {
            m_drawList.PopClipRect(); 
            m_input.SetClipRect(m_drawList.GetClipRect());
        }

        inline short4 GetClipRect() const { return m_drawList.GetClipRect(); }

        inline void PushLayer() 
        {
            m_drawList.PushLayer(); 
            m_input.PushLayer();
        }
        
        inline void PopLayer() 
        {
            m_drawList.PopLayer(); 
            m_input.PopLayer();
        }

        inline uint16_t GetLayer() const { return m_drawList.GetLayer(); }
        
        inline bool BeginLayout(const GUILayoutStyle& style, const short4& desiredRect = PK_SHORT4_ZERO) { return m_layoutStack.Begin(style, desiredRect); }
        inline void EndLayout() { m_layoutStack.End(); }
        inline const GUILayout& GetLayout() const { return m_layoutStack.Get(); }
        inline short4 NextLayoutRect(const short4& desiredRect = PK_SHORT4_ZERO) { return m_layoutStack.NextRect(desiredRect); }
        inline short4 GetLayoutArea() const { return GetLayout().area; }
        inline short4 GetLayoutContent() const { return GetLayout().content; }

        inline void DrawLine(const short2& p0, const short2& p1, const color32& color, const float width) 
        {
            m_drawList.Line(p0, p1, color, color, width);
        }
        
        inline void DrawCurve(const short2* points, const color32& color, uint32_t count, const float width) 
        { 
            m_drawList.Curve(points, color, color, count, width);
        }
        
        inline void DrawBezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color, float width, float density = 12.0f)
        {
            m_drawList.BezierCurve(p0, p1, cp0, cp1, color, color, width, density);
        }

        inline void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex)
        {
            m_drawList.Rect(color, rect, textureRect, textureIndex, GUI_RENDER_MODE_DEFAULT);
        }

        inline void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture)
        {
            m_drawList.Rect(color, rect, textureRect, texture, GUI_RENDER_MODE_DEFAULT);
        }

        inline void DrawRect(const color32& color, const short4& rect, uint16_t renderMode)
        {
            m_drawList.Rect(color, rect, PK_USHORT4_ZERO, GUI_TEX_INDEX_WHITE, renderMode);
        }

        inline void DrawRect(const color32& color, const short4& rect)
        {
            m_drawList.Rect(color, rect, PK_USHORT4_ZERO, GUI_TEX_INDEX_WHITE, GUI_RENDER_MODE_DEFAULT);
        }

        inline int16_t GetLineHeight(const FontStyle& style) { return (int16_t)m_ctx->defautFont->GetLineHeight(style); }

        inline FontGeometryInfo CalculateText(const short4& rect, const char* text, const FontStyle& style)
        {
            return m_drawList.CalculateText(rect, text, m_ctx->defautFont, style);
        }

        inline short4 DrawText(const color32& color, const short4& rect, const char* text, const FontStyle& style)
        {
            return m_drawList.Text(color, rect, text, m_ctx->defautFont, style);
        }

    private:
        GUIContext* m_ctx;
        GUIHashStack m_hashStack;
        GUILayoutStack m_layoutStack;
        GUIDrawList m_drawList;
        GUIInput m_input;
    };
}
