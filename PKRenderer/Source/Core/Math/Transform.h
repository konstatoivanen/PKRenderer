#pragma once
#include "Vector.h"

namespace PK::math
{
    template<typename T> matrix<T,4,4> transformTRS(const vector<T,3>& position, const quaternion<T>& rotation, const vector<T,3>& scale)
    {
        const auto qxx(rotation.x * rotation.x);
        const auto qyy(rotation.y * rotation.y);
        const auto qzz(rotation.z * rotation.z);
        const auto qxz(rotation.x * rotation.z);
        const auto qxy(rotation.x * rotation.y);
        const auto qyz(rotation.y * rotation.z);
        const auto qwx(rotation.w * rotation.x);
        const auto qwy(rotation.w * rotation.y);
        const auto qwz(rotation.w * rotation.z);
        matrix<T,4,4> m(static_cast<T>(1));
        m[3].xyz = position;
        m[0].x = scale[0] * (static_cast<T>(1) - static_cast<T>(2) * (qyy + qzz));
        m[0].y = scale[0] * (static_cast<T>(2) * (qxy + qwz));
        m[0].z = scale[0] * (static_cast<T>(2) * (qxz - qwy));
        m[1].x = scale[1] * (static_cast<T>(2) * (qxy - qwz));
        m[1].y = scale[1] * (static_cast<T>(1) - static_cast<T>(2) * (qxx + qzz));
        m[1].z = scale[1] * (static_cast<T>(2) * (qyz + qwx));
        m[2].x = scale[2] * (static_cast<T>(2) * (qxz + qwy));
        m[2].y = scale[2] * (static_cast<T>(2) * (qyz - qwx));
        m[2].z = scale[2] * (static_cast<T>(1) - static_cast<T>(2) * (qxx + qyy));
        return m;
    }

    template<typename T> matrix<T,3,4> transformTRS3x4(const vector<T,3>& position, const quaternion<T>& rotation, const vector<T,3>& scale)
    {
        const auto qxx(rotation.x * rotation.x);
        const auto qyy(rotation.y * rotation.y);
        const auto qzz(rotation.z * rotation.z);
        const auto qxz(rotation.x * rotation.z);
        const auto qxy(rotation.x * rotation.y);
        const auto qyz(rotation.y * rotation.z);
        const auto qwx(rotation.w * rotation.x);
        const auto qwy(rotation.w * rotation.y);
        const auto qwz(rotation.w * rotation.z);
        matrix<T,3,4> m(static_cast<T>(0));
        m[0].x = scale[0] * (static_cast<T>(1) - static_cast<T>(2) * (qyy + qzz));
        m[1].x = scale[0] * (static_cast<T>(2) * (qxy + qwz));
        m[2].x = scale[0] * (static_cast<T>(2) * (qxz - qwy));
        m[0].y = scale[1] * (static_cast<T>(2) * (qxy - qwz));
        m[1].y = scale[1] * (static_cast<T>(1) - static_cast<T>(2) * (qxx + qzz));
        m[2].y = scale[1] * (static_cast<T>(2) * (qyz + qwx));
        m[0].z = scale[2] * (static_cast<T>(2) * (qxz + qwy));
        m[1].z = scale[2] * (static_cast<T>(2) * (qyz - qwx));
        m[2].z = scale[2] * (static_cast<T>(1) - static_cast<T>(2) * (qxx + qyy));
        m[0].w = position.x;
        m[1].w = position.y;
        m[2].w = position.z;
        return m;
    }
 
    template<typename T> matrix<T,4,4> transformTR(const vector<T,3>& position, const quaternion<T>& rotation)
    {
        const auto qxx(rotation.x * rotation.x);
        const auto qyy(rotation.y * rotation.y);
        const auto qzz(rotation.z * rotation.z);
        const auto qxz(rotation.x * rotation.z);
        const auto qxy(rotation.x * rotation.y);
        const auto qyz(rotation.y * rotation.z);
        const auto qwx(rotation.w * rotation.x);
        const auto qwy(rotation.w * rotation.y);
        const auto qwz(rotation.w * rotation.z);
        matrix<T,4,4> m(static_cast<T>(1));
        m[3].xyz = position;
        m[0][0] = static_cast<T>(1) - static_cast<T>(2) * (qyy + qzz);
        m[0][1] = static_cast<T>(2) * (qxy + qwz);
        m[0][2] = static_cast<T>(2) * (qxz - qwy);
        m[1][0] = static_cast<T>(2) * (qxy - qwz);
        m[1][1] = static_cast<T>(1) - static_cast<T>(2) * (qxx + qzz);
        m[1][2] = static_cast<T>(2) * (qyz + qwx);
        m[2][0] = static_cast<T>(2) * (qxz + qwy);
        m[2][1] = static_cast<T>(2) * (qyz - qwx);
        m[2][2] = static_cast<T>(1) - static_cast<T>(2) * (qxx + qyy);
        return m;
    }

    template<typename T> matrix<T,4,4> transformTRS(const vector<T,3>& position, const vector<T,3>& euler, const vector<T,3>& scale) { return transformTRS(position, quaternion(euler), scale); }
    template<typename T> matrix<T,3,4> transformTRS3x4(const vector<T,3>& position, const vector<T,3>& euler, const vector<T,3>& scale) { return transformTRS3x4(position, quaternion(euler), scale); }
    template<typename T> matrix<T,4,4> transformTRSInverse(const vector<T,3>& position, const quaternion<T>& quaternion, const vector<T,3>& scale) { return affineInverse(transformTRS(position, quaternion, scale)); }
    template<typename T> matrix<T,4,4> transformTRSInverse(const vector<T,3>& position, const vector<T,3>& euler, const vector<T,3>& scale){ return transformTRSInverse(position, quaternion(euler), scale); }

    template<typename T> matrix<T,4,4> affineInverseTranspose(const matrix<T,3,4>& m)
    {
        const auto invd = rcp(m[0].x * (m[1].y * m[2].z - m[1].z * m[2].y) - m[0].y * (m[1].x * m[2].z - m[1].z * m[2].x) + m[0].z * (m[1].x * m[2].y - m[1].y * m[2].x));
        const auto offs = vector<T,3>(m[0].w, m[1].w, m[2].w);
        matrix<T,3,3> inv;
        inv[0].x = +(m[1].y * m[2].z - m[1].z * m[2].y) * invd;
        inv[1].x = -(m[0].y * m[2].z - m[0].z * m[2].y) * invd;
        inv[2].x = +(m[0].y * m[1].z - m[0].z * m[1].y) * invd;
        inv[0].y = -(m[1].x * m[2].z - m[1].z * m[2].x) * invd;
        inv[1].y = +(m[0].x * m[2].z - m[0].z * m[2].x) * invd;
        inv[2].y = -(m[0].x * m[1].z - m[0].z * m[1].x) * invd;
        inv[0].z = +(m[1].x * m[2].y - m[1].y * m[2].x) * invd;
        inv[1].z = -(m[0].x * m[2].y - m[0].y * m[2].x) * invd;
        inv[2].z = +(m[0].x * m[1].y - m[0].y * m[1].x) * invd;
        return matrix<T,4,4>(vector<T,4>(inv[0], static_cast<T>(0)), vector<T,4>(inv[1], static_cast<T>(0)), vector<T,4>(inv[2], static_cast<T>(0)), vector<T,4>(-(inv * offs), static_cast<T>(1)));
    }
}
