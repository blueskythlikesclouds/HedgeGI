#pragma once

#include "Mesh.h"

#define LOG2E 1.44269504088896340736f
#define PI    3.14159265358979323846264338327950288f

template<typename T>
struct EigenHash
{
    size_t operator()(const T& matrix) const
    {
        size_t seed = 0;

        for (size_t i = 0; i < (size_t)matrix.size(); i++)
            seed ^= std::hash<typename T::Scalar>()(*(matrix.data() + i)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        
        return seed;
    }
};

static Eigen::Vector2f getBarycentricCoords(const Eigen::Vector3f& point, const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c)
{
    const Eigen::Vector3f v0 = c - a;
    const Eigen::Vector3f v1 = b - a;
    const Eigen::Vector3f v2 = point - a;

    const float dot00 = v0.dot(v0);
    const float dot01 = v0.dot(v1);
    const float dot02 = v0.dot(v2);
    const float dot11 = v1.dot(v1);
    const float dot12 = v1.dot(v2);

    const float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);

    return { (dot11 * dot02 - dot01 * dot12) * invDenom, (dot00 * dot12 - dot01 * dot02) * invDenom };
}

template <typename T>
static T barycentricLerp(const T& a, const T& b, const T& c, const Eigen::Vector2f& uv)
{
    return a + (c - a) * uv[0] + (b - a) * uv[1];
}

template <typename T>
static T lerp(const T& a, const T& b, float factor)
{
    return a + (b - a) * factor;
}

static Eigen::Vector2f clampUV(const Eigen::Vector2f& uv)
{
    float temp;

    Eigen::Vector2f value { std::modf(uv[0], &temp), std::modf(uv[1], &temp) };

    if (value[0] < 0)
        value[0] += 1.0f;

    if (value[1] < 0)
        value[1] += 1.0f;

    return value;
}

// https://github.com/TheRealMJP/BakingLab/blob/master/SampleFramework11/v1.02/Shaders/Sampling.hlsl

static Eigen::Vector2f squareToConcentricDiskMapping(const float x, const float y)
{
    float phi = 0.0f;
    float r = 0.0f;

    float a = 2.0f * x - 1.0f;
    float b = 2.0f * y - 1.0f;

    if (a > -b)
    {
        if (a > b)
        {
            r = a;
            phi = (PI / 4.0f) * (b / a);
        }
        else
        {
            r = b;
            phi = (PI / 4.0f) * (2.0f - (a / b));
        }
    }
    else
    {
        if (a < b)
        {
            r = -a;
            phi = (PI / 4.0f) * (4.0f + (b / a));
        }
        else
        {
            r = -b;
            if (b != 0)
                phi = (PI / 4.0f) * (6.0f - (a / b));
            else
                phi = 0;
        }
    }

    return { r * std::cos(phi), r * std::sin(phi) };
}

static Eigen::Vector3f sampleCosineWeightedHemisphere(const float u1, const float u2)
{
    const Eigen::Vector2f uv = squareToConcentricDiskMapping(u1, u2);
    const float u = uv[0];
    const float v = uv[1];

    Eigen::Vector3f dir;
    const float r = u * u + v * v;
    return { u, v, std::sqrt(std::max(0.0f, 1.0f - r)) };
}

static Eigen::Vector3f sampleStratifiedCosineWeightedHemisphere(const size_t sX, const size_t sY, const size_t sqrtNumSamples, const float u1, const float u2)
{
    float jitteredX = (sX + u1) / sqrtNumSamples;
    float jitteredY = (sY + u2) / sqrtNumSamples;

    Eigen::Vector2f uv = squareToConcentricDiskMapping(jitteredX, jitteredY);
    float u = uv[0];
    float v = uv[1];

    Eigen::Vector3f dir;
    float r = u * u + v * v;
    dir[0] = u;
    dir[1] = v;
    dir[2] = std::sqrt(std::max(0.0f, 1.0f - r));

    return dir;
}

static Eigen::Vector3f sampleStratifiedCosineWeightedHemisphere(const size_t sampleIndex, const size_t sqrtNumSamples, const float u1, const float u2)
{
    return sampleStratifiedCosineWeightedHemisphere(sampleIndex % sqrtNumSamples, sampleIndex / sqrtNumSamples, u1, u2);
}

static Eigen::Vector3f sampleDirectionHemisphere(const float u1, const float u2)
{
    float z = u1;
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2 * PI * u2;
    float x = r * std::cos(phi);
    float y = r * std::sin(phi);

    return { x, y, z };
}

static Eigen::Vector3f sampleDirectionSphere(const float u1, const float u2)
{
    float z = u1 * 2.0f - 1.0f;
    float r = sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2 * PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);

    return { x, y, z };
}

const float GOLDEN_ANGLE = PI * (3 - sqrtf(5));

static Eigen::Vector3f sampleSphere(const size_t index, const size_t sampleCount)
{
    const float y = 1 - (float)index / (float)(sampleCount - 1) * 2;
    const float radius = std::sqrt(1 - y * y);

    const float theta = GOLDEN_ANGLE * (float)index;

    const float x = std::cos(theta) * radius;
    const float z = std::sin(theta) * radius;

    return { x, y, z };
}

static Eigen::Vector2f sampleVogelDisk(const size_t index, const size_t sampleCount, const float phi)
{
    const float radius = std::sqrt(index + 0.5f) / std::sqrt((float)sampleCount);
    const float theta = index * GOLDEN_ANGLE + phi;

    return { radius * std::cos(theta), radius * std::sin(theta) };
}

// https://gist.github.com/pixnblox/5e64b0724c186313bc7b6ce096b08820

static Eigen::Vector3f getSmoothPosition(const Vertex& a, const Vertex& b, const Vertex& c, const Eigen::Vector2f& baryUV)
{
    const Eigen::Vector3f position = barycentricLerp(a.position, b.position, c.position, baryUV);
    const Eigen::Vector3f normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV);

    const Eigen::Vector3f vecProj0 = position - (position - a.position).dot(a.normal) * a.normal;
    const Eigen::Vector3f vecProj1 = position - (position - b.position).dot(b.normal) * b.normal;
    const Eigen::Vector3f vecProj2 = position - (position - c.position).dot(c.normal) * c.normal;

    const Eigen::Vector3f smoothPosition = barycentricLerp(vecProj0, vecProj1, vecProj2, baryUV);
    return (smoothPosition - position).dot(normal) > 0.0f ? smoothPosition : position;
}

static float saturate(const float value)
{
    return std::min(1.0f, std::max(0.0f, value));
}

// https://github.com/DarioSamo/libgens-sonicglvl/blob/master/src/LibGens/MathGens.cpp

static Eigen::Vector3f getAabbCorner(const Eigen::AlignedBox3f& aabb, const size_t index)
{
    switch (index)
    {
    case 0: return Eigen::Vector3f(aabb.min().x(), aabb.min().y(), aabb.min().z());
    case 1: return Eigen::Vector3f(aabb.min().x(), aabb.min().y(), aabb.max().z());
    case 2: return Eigen::Vector3f(aabb.min().x(), aabb.max().y(), aabb.min().z());
    case 3: return Eigen::Vector3f(aabb.min().x(), aabb.max().y(), aabb.max().z());
    case 4: return Eigen::Vector3f(aabb.max().x(), aabb.min().y(), aabb.min().z());
    case 5: return Eigen::Vector3f(aabb.max().x(), aabb.min().y(), aabb.max().z());
    case 6: return Eigen::Vector3f(aabb.max().x(), aabb.max().y(), aabb.min().z());
    case 7: return Eigen::Vector3f(aabb.max().x(), aabb.max().y(), aabb.max().z());

    default:
        return Eigen::Vector3f();
    }
}

static Eigen::AlignedBox3f getAabbHalf(const Eigen::AlignedBox3f& aabb, const size_t axis, const size_t side)
{
    Eigen::AlignedBox3f result = aabb;

    switch (axis)
    {
    case 0:
        if (side == 0) result.max().x() = result.center().x();
        else if (side == 1) result.min().x() = result.center().x();
        break;
    case 1:
        if (side == 0) result.max().y() = result.center().y();
        else if (side == 1) result.min().y() = result.center().y();
        break;
    case 2:
        if (side == 0) result.max().z() = result.center().z();
        else if (side == 1) result.min().z() = result.center().z();
        break;
    default:
        result.min() = Eigen::Vector3f();
        result.max() = Eigen::Vector3f();
        break;
    }

    return result;
}

static size_t relativeCorner(const Eigen::Vector3f& left, const Eigen::Vector3f& right)
{
    if (right.x() <= left.x()) 
    {
        if (right.y() <= left.y()) 
        {
            if (right.z() <= left.z()) 
                return 0;

            return 1;
        }

        if (right.z() <= left.z()) 
            return 2;

        return 3;
    }
    if (right.y() <= left.y()) 
    {
        if (right.z() <= left.z()) 
            return 4;

        return 5;
    }
    if (right.z() <= left.z())
        return 6;

    return 7;
}

template<typename T>
static float dot(const T& a, const T& b)
{
    return a.dot(b);
}

// https://github.com/embree/embree/blob/master/tutorials/common/math/closest_point.h

static Eigen::Vector3f closestPointTriangle(const Eigen::Vector3f& p, const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c)
{
    const Eigen::Vector3f ab = b - a;
    const Eigen::Vector3f ac = c - a;
    const Eigen::Vector3f ap = p - a;

    const float d1 = dot(ab, ap);
    const float d2 = dot(ac, ap);
    if (d1 <= 0.f && d2 <= 0.f) return a;

    const Eigen::Vector3f bp = p - b;
    const float d3 = dot(ab, bp);
    const float d4 = dot(ac, bp);
    if (d3 >= 0.f && d4 <= d3) return b;

    const Eigen::Vector3f cp = p - c;
    const float d5 = dot(ab, cp);
    const float d6 = dot(ac, cp);
    if (d6 >= 0.f && d5 <= d6) return c;

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
    {
        const float v = d1 / (d1 - d3);
        return a + v * ab;
    }

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
    {
        const float v = d2 / (d2 - d6);
        return a + v * ac;
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
    {
        const float v = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + v * (c - b);
    }

    const float denom = 1.f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    return a + v * ab + w * ac;
}

static Eigen::Vector3f fresnelSchlick(Eigen::Vector3f F0, float cosTheta)
{
    float p = (-5.55473f * cosTheta - 6.98316f) * cosTheta;
    return F0 + (Eigen::Vector3f::Ones() - F0) * exp2(p);
}

static float ndfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * alphaSq - cosLh) * cosLh + 1;
    return alphaSq / (PI * denom * denom);
}

static float visSchlick(float roughness, float cosLo, float cosLi)
{
    float r = roughness + 1;
    float k = (r * r) / 8;
    float schlickV = cosLo * (1 - k) + k;
    float schlickL = cosLi * (1 - k) + k;
    return 0.25f / (schlickV * schlickL);
}

// https://stackoverflow.com/a/60047308

static uint32_t as_uint(const float x)
{
    return *(uint32_t*)&x;
}

static float as_float(const uint32_t x)
{
    return *(float*)&x;
}

static float half_to_float(const uint16_t x)
{
    const uint32_t e = (x & 0x7C00) >> 10;
    const uint32_t m = (x & 0x03FF) << 13;
    const uint32_t v = as_uint((float)m) >> 23;
    return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000)));
}