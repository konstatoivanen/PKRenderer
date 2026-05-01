#pragma once
#include "Vector.h"

namespace PK::math
{
    template<typename T>
    struct quaternion : public vector<T,4>
    {
        constexpr quaternion() = default;
        constexpr quaternion(const quaternion<T>&v) = default;
        constexpr quaternion(const vector<T,4>& v) : vector<T,4>(v) {}
        constexpr quaternion(T x, T y, T z, T w) : vector<T,4>(x, y, z, w) {}

        // From euler
        explicit quaternion(const vector<T, 3>& euler) : quaternion()
        {
            auto s = sin(euler * static_cast<T>(0.5));
            auto c = cos(euler * static_cast<T>(0.5));
            vector<T,4>::x = s.x * c.y * c.z - s.y * s.z * c.x, 
            vector<T,4>::y = s.y * c.x * c.z + s.x * s.z * c.y;
            vector<T,4>::z = s.z * c.x * c.y - s.x * s.y * c.z;
            vector<T,4>::w = c.x * c.y * c.z + s.y * s.z * s.x;
        }

        // From matrix3x3
        explicit quaternion(const matrix<T,3,3>& m) : quaternion()
        {
            const auto x = m[0][0] - m[1][1] - m[2][2];
            const auto y = m[1][1] - m[0][0] - m[2][2];
            const auto z = m[2][2] - m[0][0] - m[1][1];
            const auto w = m[0][0] + m[1][1] + m[2][2];

            auto i = 0;
            auto v = x;

            if (y > v) { v = y; i = 1; }
            if (z > v) { v = z; i = 2; }
            if (w > v) { v = w; i = 3; }

            const auto vf = sqrt(v + static_cast<T>(1)) * static_cast<T>(0.5);
            const auto cf = static_cast<T>(0.25) / vf;

            switch (i)
            {
                default:
                case 0: *this = quaternion(vf, (m[1][2] - m[2][1]) * cf, (m[2][0] - m[0][2]) * cf, (m[0][1] - m[1][0]) * cf);
                case 1: *this = quaternion((m[1][2] - m[2][1]) * cf, vf, (m[0][1] + m[1][0]) * cf, (m[2][0] + m[0][2]) * cf);
                case 2: *this = quaternion((m[2][0] - m[0][2]) * cf, (m[0][1] + m[1][0]) * cf, vf, (m[1][2] + m[2][1]) * cf);
                case 3: *this = quaternion((m[0][1] - m[1][0]) * cf, (m[2][0] + m[0][2]) * cf, (m[1][2] + m[2][1]) * cf, vf);
            }
        }

        // From matrix4x4
        explicit quaternion(const matrix<T,4,4>& m) : quaternion(matrix<T,3,3>(m)) {}

        // From tangent basis
        explicit quaternion(const vector<T, 3>& f, const vector<T, 3>& b) : quaternion()
        {
            const auto t = normalize(f - b * dot(f, b) / dot(b, b));
            const auto n = cross(b, t);
            const auto w = sqrt(1.0f + n.x + b.y + t.z) * 0.5f;
            vector<T,4>::x = (b.z - t.y) / (4.0f * w);
            vector<T,4>::y = (t.x - n.z) / (4.0f * w);
            vector<T,4>::z = (n.y - b.x) / (4.0f * w);
            vector<T,4>::w = w;
        }
    };

    template<typename T> quaternion<T> conjugate(const quaternion<T>& q) { return quaternion<T>(-q.x,-q.y,-q.z,q.w); }

    template<typename T> quaternion<T> inverse(const quaternion<T>& q) { return quaternion<T>(rcp(dot(q,q)) * q * static_cast<T>(-1)); }
   
    template<typename T> quaternion<T> mul(const quaternion<T>& q, const quaternion<T>& p)
    {
        return quaternion<T>(p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
                             p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z,
                             p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x,
                             p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z);
    }

    template<typename T> vector<T,3> mul(const quaternion<T>& q, const vector<T,3>& v)
    {
        const vector<T,3> qv(q.x, q.y, q.z);
        const vector<T,3> uv(cross(qv, v));
        const vector<T,3> uuv(cross(qv, uv));
        return v + ((uv * q.w) + uuv) * static_cast<T>(2);
    }

    template<typename T> vector<T,3> mul(const vector<T,3>& v, const quaternion<T>& q) { return mul(inverse(q), v); }

    template<typename T> quaternion<T> nlerp(const quaternion<T>& a, const quaternion<T>& b, T t) { return quaternion<T>(normalize(lerp(a,b,t))); }

    template<typename T> quaternion<T> slerp(const quaternion<T>& x, const quaternion<T>& y, T a)
    {
        vector<T,4> z = y;
        auto cosTheta = dot(x, y);

        // If cosTheta < 0, the interpolation will take the long way around the sphere.
        // To fix this, one quat must be negated.
        if (cosTheta < static_cast<T>(0))
        {
            z = -y;
            cosTheta = -cosTheta;
        }

        // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
        if (cosTheta > static_cast<T>(1) - static_cast<T>(1e-4f))
        {
            // Linear interpolation
            return quaternion<T>(
                lerp(x.x, z.x, a),
                lerp(x.y, z.y, a),
                lerp(x.z, z.z, a),
                lerp(x.w, z.w, a));
        }
        else
        {
            // Essential Mathematics, page 467
            T angle = acos(cosTheta);
            return quaternion<T>((sin((static_cast<T>(1) - a) * angle) * x + sin(a * angle) * z) / sin(angle));
        }
    }

    template<typename T> T roll(const quaternion<T>& q) { return static_cast<T>(atan2(static_cast<T>(2) * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z)); }

    template<typename T> T pitch(const quaternion<T>& q)
    {
        const auto y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
        const auto x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
        return x == static_cast<T>(0) && y == static_cast<T>(0) ? static_cast<T>(static_cast<T>(2) * atan2(q.x, q.w)) : static_cast<T>(atan2(y, x));
    }

    template<typename T> T yaw(const quaternion<T>& q) { return asin(clamp(static_cast<T>(-2) * (q.x * q.z - q.w * q.y), static_cast<T>(-1), static_cast<T>(1))); }

    template<typename T> quaternion<T> lookAt(const vector<T,3>& v) { return quaternion(v, vector<T,3>(static_cast<T>(0),static_cast<T>(1), static_cast<T>(0))); }

    template<typename T> matrix<T,3,3> rotation3x3(const quaternion<T>& q)
    {
        return matrix<T,3,3>(
            T(1) - T(2) * (q.y * q.y + q.z * q.z),
            T(2) * (q.x * q.y + q.w * q.z),
            T(2) * (q.x * q.z - q.w * q.y),
            T(2) * (q.x * q.y - q.w * q.z),
            T(1) - T(2) * (q.x * q.x + q.z * q.z),
            T(2) * (q.y * q.z + q.w * q.x),
            T(2) * (q.x * q.z + q.w * q.y),
            T(2) * (q.y * q.z - q.w * q.x),
            T(1) - T(2) * (q.x * q.x + q.y * q.y));
    }

    template<typename T> vector<T,3> euler(const quaternion<T>& q) { return vector<T,3>(pitch(q), yaw(q), roll(q)); }
}
