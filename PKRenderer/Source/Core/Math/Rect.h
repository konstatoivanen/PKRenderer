#pragma once
#include "Math.h"

namespace PK::math
{
    template<typename T>
    struct rectsplit
    {
        vector<T,4> tail;
        vector<T,4> head;
    };

    template<typename T> bool rectIntersect(const vector<T,4>& a, const vector<T,4>& b)
    {
        return (a.x + a.z) >= b.x && a.x <= (b.x + b.z) && (a.y + a.w) >= b.y && a.y <= (b.y + b.w);
    }

    template<typename T> bool rectIntersect(const vector<T,4>& r, const vector<T,2>& p)
    {
        return r.x <= p.x && r.y <= p.y && (r.x + r.z) > p.x && (r.y + r.w) > p.y;
    }

    template<typename T> vector<T,4> rectToMinMax(const vector<T,4>& r)
    {
        return vector<T,4>(r.x, r.y, r.x + r.z, r.y + r.w);
    }

    template<typename T> vector<T,4> rectFromMinMax(const vector<T,4>& m)
    {
        return vector<T,4>(m.x, m.y, m.z - m.x, m.w - m.y);
    }

    template<typename T> vector<T,4> rectFromLine(const vector<T,2>& p0, const vector<T,2>& p1)
    {
        const auto rmax = max(p0, p1);
        const auto rmin = min(p0, p1);
        return { rmin, rmax - rmin };
    }

    template<typename T> vector<T,4> rectPad(const vector<T,4>& r, T padding)
    {
        return vector<T,4>(r.x + padding, r.y + padding, r.z - padding * static_cast<T>(2), r.w - padding * static_cast<T>(2));
    }

    template<typename T> vector<T,4> rectPad(const vector<T,4>& r, const vector<T,2>& padding)
    {
        return vector<T, 4>(r.x + padding.x, r.y + padding.y, r.z - padding.x * static_cast<T>(2), r.w - padding.y * static_cast<T>(2));
    }

    template<typename T> vector<T,4> rectPad(const vector<T,4>& r, const vector<T,4>& padding)
    {
        return vector<T, 4>(r.x + padding.x, r.y + padding.y, r.z - padding.x - padding.z, r.w - padding.y - padding.w);
    }

    template<typename T> vector<T,4> rectClamp(const vector<T,4>& r, const vector<T,4>& b)
    {
        vector<T,4> o = r;
        o.z = min(o.z, b.z);
        o.w = min(o.w, b.w);
        o.x = clamp(o.x, b.x, static_cast<T>(b.x + b.z - o.z));
        o.y = clamp(o.y, b.y, static_cast<T>(b.y + b.w - o.w));
        return o;
    }

    template<typename T> vector<T,4> rectClip(const vector<T,4>& a, const vector<T,4>& b)
    {
        const T x0 = max(a.x, b.x);
        const T y0 = max(a.y, b.y);
        const T x1 = min(a.x + a.z, b.x + b.z);
        const T y1 = min(a.y + a.w, b.y + b.w);
        return vector<T,4>(x0, y0, max(static_cast<T>(0), static_cast<T>(x1 - x0)), max(static_cast<T>(0), static_cast<T>(y1 - y0)));
    }

    template<typename T> vector<T,4> rectMerge(const vector<T,4>& a, const vector<T,4>& b)
    {
        const T x0 = min(a.x, b.x);
        const T y0 = min(a.y, b.y);
        const T x1 = max(a.x + a.z, b.x + b.z);
        const T y1 = max(a.y + a.w, b.y + b.w);
        return vector<T,4>(x0, y0, max(static_cast<T>(0), static_cast<T>(x1 - x0)), max(static_cast<T>(0), static_cast<T>(y1 - y0)));
    }


    template<typename T> rectsplit<T> rectSplitMin(const vector<T,4>& r, const vector<T,2>& size)
    {
        const T w = size.x ? min(r.z, size.x) : r.z;
        const T h = size.y ? min(r.w, size.y) : r.w;
        const T x = size.x ? w : static_cast<T>(0);
        const T y = size.y ? h : static_cast<T>(0);
        const T mw = size.x ? r.z - w : r.z;
        const T mh = size.y ? r.w - h : r.w;
        return { vector<T,4>(r.x,r.y,w,h), vector<T,4>(r.x+x,r.y+y,mw,mh) };
    }

    template<typename T> rectsplit<T> rectSplitMax(const vector<T, 4>& r, const vector<T,2>& size)
    {
        const T w = size.x ? min(r.z, size.x) : r.z;
        const T h = size.y ? min(r.w, size.y) : r.w;
        const T mw = size.x ? r.z - w : r.z;
        const T mh = size.y ? r.w - h : r.w;
        const T x = size.x ? mw : static_cast<T>(0);
        const T y = size.y ? mh : static_cast<T>(0);
        return { vector<T,4>(r.x+x,r.y+y,w,h), vector<T,4>(r.x,r.y,mw,mh) };
    }

    template<typename T> vector<T,4> rectGrid(const vector<T,4>& rect, const vector<uint32_t,2> size, uint32_t index)
    {
        index = index % (size.x * size.y);
        const auto x = index % size.x;
        const auto y = index / size.x;
        const auto ws = rect.z / size.x;
        const auto hs = rect.w / size.y;
        const auto w = lerp(ws, rect.z - x * ws, x == size.x - 1u);
        const auto h = lerp(hs, rect.w - y * hs, y == size.y - 1u);
        return vector<T, 4>(rect.x + x * ws, rect.y + y * hs, w, h);
    }
}
