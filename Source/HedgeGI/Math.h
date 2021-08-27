#pragma once

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

inline Vector2 getBarycentricCoords(const Vector3& point, const Vector3& a, const Vector3& b, const Vector3& c)
{
    const Vector3 v0 = c - a;
    const Vector3 v1 = b - a;
    const Vector3 v2 = point - a;

    const float dot00 = v0.dot(v0);
    const float dot01 = v0.dot(v1);
    const float dot02 = v0.dot(v2);
    const float dot11 = v1.dot(v1);
    const float dot12 = v1.dot(v2);

    const float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);

    return { (dot00 * dot12 - dot01 * dot02) * invDenom, (dot11 * dot02 - dot01 * dot12) * invDenom };
}

template <typename T>
inline T barycentricLerp(const T& a, const T& b, const T& c, const float u, const float v)
{
    return a + (b - a) * u + (c - a) * v;
}

template<typename T>
inline T barycentricLerp(const T& a, const T& b, const T& c, const Vector2& baryUV)
{
    return barycentricLerp(a, b, c, baryUV.x(), baryUV.y());
}

template <typename T>
inline T lerp(const T& a, const T& b, float factor)
{
    return a + (b - a) * factor;
}

inline float saturate(const float value)
{
    return std::min(1.0f, std::max(0.0f, value));
}

template<typename T>
inline T saturate(const T& value)
{
    return value.cwiseMax(0).cwiseMin(1);
}

template<typename T>
inline T square(const T& value)
{
    return value * value;
}

// https://github.com/TheRealMJP/BakingLab

inline Vector2 squareToConcentricDiskMapping(const float x, const float y)
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

inline Vector3 sampleCosineWeightedHemisphere(const float u1, const float u2)
{
    const Vector2 uv = squareToConcentricDiskMapping(u1, u2);
    const float u = uv[0];
    const float v = uv[1];

    Vector3 dir;
    const float r = u * u + v * v;
    return { u, v, std::sqrt(std::max(0.0f, 1.0f - r)) };
}

inline Vector3 sampleStratifiedCosineWeightedHemisphere(const size_t sX, const size_t sY, const size_t sqrtNumSamples, const float u1, const float u2)
{
    float jitteredX = (sX + u1) / sqrtNumSamples;
    float jitteredY = (sY + u2) / sqrtNumSamples;

    Vector2 uv = squareToConcentricDiskMapping(jitteredX, jitteredY);
    float u = uv[0];
    float v = uv[1];

    Vector3 dir;
    float r = u * u + v * v;
    dir[0] = u;
    dir[1] = v;
    dir[2] = std::sqrt(std::max(0.0f, 1.0f - r));

    return dir;
}

inline Vector3 sampleStratifiedCosineWeightedHemisphere(const size_t sampleIndex, const size_t sqrtNumSamples, const float u1, const float u2)
{
    return sampleStratifiedCosineWeightedHemisphere(sampleIndex % sqrtNumSamples, sampleIndex / sqrtNumSamples, u1, u2);
}

inline Vector3 sampleDirectionHemisphere(const float u1, const float u2)
{
    float z = u1;
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2 * PI * u2;
    float x = r * std::cos(phi);
    float y = r * std::sin(phi);

    return { x, y, z };
}

inline Vector3 sampleDirectionSphere(const float u1, const float u2)
{
    float z = u1 * 2.0f - 1.0f;
    float r = sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = 2 * PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);

    return { x, y, z };
}

const float GOLDEN_ANGLE = PI * (3 - sqrtf(5));

inline Vector3 sampleSphere(const size_t index, const size_t sampleCount)
{
    const float y = 1 - (float)index / (float)(sampleCount - 1) * 2;
    const float radius = std::sqrt(1 - y * y);

    const float theta = GOLDEN_ANGLE * (float)index;

    const float x = std::cos(theta) * radius;
    const float z = std::sin(theta) * radius;

    return { x, y, z };
}

inline Vector2 sampleVogelDisk(const size_t index, const size_t sampleCount, const float phi)
{
    const float radius = std::sqrt(index + 0.5f) / std::sqrt((float)sampleCount);
    const float theta = index * GOLDEN_ANGLE + phi;

    return { radius * std::cos(theta), radius * std::sin(theta) };
}

inline Color3 fresnelSchlick(Color3 F0, float cosTheta)
{
    float p = (-5.55473f * cosTheta - 6.98316f) * cosTheta;
    return F0 + (Color3::Ones() - F0) * exp2(p);
}

inline float ndfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * alphaSq - cosLh) * cosLh + 1;
    return alphaSq / std::max(1e-8f, PI * denom * denom);
}

inline float visSchlick(float roughness, float cosLo, float cosLi)
{
    float r = roughness + 1;
    float k = (r * r) / 8;
    float schlickV = cosLo * (1 - k) + k;
    float schlickL = cosLi * (1 - k) + k;
    return 0.25f / std::max(1e-8f, schlickV * schlickL);
}

// http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html

inline Vector3 microfacetGGX(float roughness, float u1, float u2, 
    const Vector3& tangent, const Vector3& binormal, const Vector3& normal)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float cosThetaH = sqrt(std::max(0.0f, (1.0f - u1) / ((a2 - 1.0f) * u1 + 1)));
    const float sinThetaH = sqrt(std::max(0.0f, 1.0f - cosThetaH * cosThetaH));
    const float phiH = u2 * PI * 2.0f;

    return tangent * (sinThetaH * cos(phiH)) + binormal * (sinThetaH * sin(phiH)) + normal * cosThetaH;
}

// https://github.com/DarioSamo/libgens-sonicglvl/blob/master/src/LibGens/MathGens.cpp

inline Vector3 getAabbCorner(const AABB& aabb, const size_t index)
{
    switch (index)
    {
    case 0: return Vector3(aabb.min().x(), aabb.min().y(), aabb.min().z());
    case 1: return Vector3(aabb.min().x(), aabb.min().y(), aabb.max().z());
    case 2: return Vector3(aabb.min().x(), aabb.max().y(), aabb.min().z());
    case 3: return Vector3(aabb.min().x(), aabb.max().y(), aabb.max().z());
    case 4: return Vector3(aabb.max().x(), aabb.min().y(), aabb.min().z());
    case 5: return Vector3(aabb.max().x(), aabb.min().y(), aabb.max().z());
    case 6: return Vector3(aabb.max().x(), aabb.max().y(), aabb.min().z());
    case 7: return Vector3(aabb.max().x(), aabb.max().y(), aabb.max().z());

    default:
        return Vector3();
    }
}

inline AABB getAabbHalf(const AABB& aabb, const size_t axis, const size_t side)
{
    AABB result = aabb;

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
        result.min() = Vector3();
        result.max() = Vector3();
        break;
    }

    return result;
}

inline size_t relativeCorner(const Vector3& left, const Vector3& right)
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
inline float dot(const T& a, const T& b)
{
    return a.dot(b);
}

// https://github.com/embree/embree/blob/master/tutorials/common/math/closest_point.h

inline Vector3 closestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
{
    const Vector3 ab = b - a;
    const Vector3 ac = c - a;
    const Vector3 ap = p - a;

    const float d1 = dot(ab, ap);
    const float d2 = dot(ac, ap);
    if (d1 <= 0.f && d2 <= 0.f) return a;

    const Vector3 bp = p - b;
    const float d3 = dot(ab, bp);
    const float d4 = dot(ac, bp);
    if (d3 >= 0.f && d4 <= d3) return b;

    const Vector3 cp = p - c;
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

// https://stackoverflow.com/a/60047308

inline uint32_t as_uint(const float x)
{
    return *(uint32_t*)&x;
}

inline float as_float(const uint32_t x)
{
    return *(float*)&x;
}

inline float half_to_float(const uint16_t x)
{
    const uint32_t e = (x & 0x7C00) >> 10;
    const uint32_t m = (x & 0x03FF) << 13;
    const uint32_t v = as_uint((float)m) >> 23;
    return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000)));
}

template<typename T>
inline bool nearlyEqual(const T& a, const T& b)
{
    return (a - b).cwiseAbs().maxCoeff() < 0.0001f;
}

inline bool nearlyEqual(const float a, const float b)
{
    return abs(a - b) < 0.0001f;
}

inline Color3 rgb2Hsv(const Color3& rgb)
{
    const float r = rgb.x();
    const float g = rgb.y();
    const float b = rgb.z();

    const float max = rgb.maxCoeff();
    const float min = rgb.minCoeff();

    const float v = max;
    float h, s;

    if (max == 0.0f) {
        s = 0;
        h = 0;
    }

    else if (max - min == 0.0f) {
        s = 0;
        h = 0;
    }

    else 
    {
        s = (max - min) / max;

        if (max == r) {
            h = 60 * ((g - b) / (max - min)) + 0;
        }
        else if (max == g) {
            h = 60 * ((b - r) / (max - min)) + 120;
        }
        else {
            h = 60 * ((r - g) / (max - min)) + 240;
        }
    }

    if (h < 0) h += 360.0f;

    return { h / 2, s, v };
}

inline Color3 hsv2Rgb(const Color3& hsv)
{
    const float h = hsv.x() * 2;
    const float s = hsv.y();
    const float v = hsv.z();

    float r, g, b;

    int   hi = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - hi;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (hi) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    }

    return { r, g, b };
}

inline Vector3 transform(const Vector3& v, const Matrix4& m)
{
    return (m * Vector4(v.x(), v.y(), v.z(), 1.0f)).head<3>();
}

inline Vector3 transformNormal(const Vector3& v, const Matrix4& m)
{
    return (m * Vector4(v.x(), v.y(), v.z(), 0.0f)).head<3>();
}

inline int nextPowerOfTwo(int value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

inline float getRadius(const AABB& aabb)
{
    float radius = 0.0f;

    for (size_t j = 0; j < 8; j++)
        radius = std::max<float>(radius, (aabb.center() - aabb.corner((AABB::CornerType)j)).norm());

    return radius;
}

inline float computeAttenuationHE1(const float distance, const Vector4& range)
{
    return 1 - saturate((distance - range.z()) / (range.w() - range.z()));
}

inline void computeDirectionAndAttenuationHE1(const Vector3& position, const Vector3& lightPosition, const Vector4& range, Vector3& lightDirection, float& attenuation, float* distOpt = nullptr)
{
    lightDirection = position - lightPosition;
    const float distance = lightDirection.norm();
    lightDirection /= distance;
    attenuation = computeAttenuationHE1(distance, range);
    if (distOpt) *distOpt = distance;
}

inline float computeAttenuationHE2(const float distance, const Vector4& range)
{
    float lightMask = (distance * distance) / (range.w() * range.w());
    lightMask = saturate(1 - lightMask * lightMask);
    lightMask *= lightMask;

    const float attenuation = 1.0f / std::max(1.0f, distance * distance);
    return attenuation * lightMask / (4 * PI);
}

inline void computeDirectionAndAttenuationHE2(const Vector3& position, const Vector3& lightPosition, const Vector4& range, Vector3& lightDirection, float& attenuation)
{
    lightDirection = position - lightPosition;
    const float distance = lightDirection.norm();
    lightDirection /= distance;
    attenuation = computeAttenuationHE2(distance, range);
}

namespace Eigen
{
    template<typename Scalar>
    Matrix<Scalar, 4, 4> CreatePerspectiveMatrix(const Scalar fieldOfView, const Scalar aspectRatio, const Scalar zNear, const Scalar zFar)
    {
        const Scalar yScale = (Scalar)1 / std::tan(fieldOfView / (Scalar)2);
        const Scalar xScale = yScale / aspectRatio;

        Matrix<Scalar, 4, 4> matrix;

        matrix <<
            xScale, 0, 0, 0,
            0, yScale, 0, 0,
            0, 0, -(zFar + zNear) / (zFar - zNear), -2 * zNear * zFar / (zFar - zNear),
            0, 0, -1, 0;

        return matrix;
    }
}

extern alignas(64) std::array<float, 256> srgbToLinearLUT;

inline Color3 srgbToLinear(const Color3& value)
{
    return
    {
        srgbToLinearLUT[static_cast<uint8_t>(value.x() * 255.0f)],
        srgbToLinearLUT[static_cast<uint8_t>(value.y() * 255.0f)],
        srgbToLinearLUT[static_cast<uint8_t>(value.z() * 255.0f)],
    };
}

inline Vector3 tangentToWorld(const Vector3& value, const Vector3& tangent, const Vector3& binormal, const Vector3& normal)
{
    return value.x() * tangent + value.y() * binormal + value.z() * normal;
}

inline Matrix3 getTangentToWorldMatrix(const Vector3& normal)
{
    const Vector3 t1 = normal.cross(Vector3(0, 0, 1));
    const Vector3 t2 = normal.cross(Vector3(0, 1, 0));
    const Vector3 tangent = (t1.norm() > t2.norm() ? t1 : t2).normalized();
    const Vector3 binormal = tangent.cross(normal).normalized();

    Matrix3 tangentToWorld;
    tangentToWorld <<
        tangent[0], binormal[0], normal[0],
        tangent[1], binormal[1], normal[1],
        tangent[2], binormal[2], normal[2];

    return tangentToWorld;
}