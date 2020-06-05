#pragma once

#define PI 3.14159265358979323846264338327950288f

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

static Eigen::Vector4f gammaCorrect(const Eigen::Vector4f& color)
{
    return { std::pow(color[0], 2.2f), std::pow(color[1], 2.2f), std::pow(color[2], 2.2f), color[3] };
}
