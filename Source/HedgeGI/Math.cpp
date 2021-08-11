#include "Math.h"

constexpr std::array<float, 256> computeSrgbToLinearLUT()
{
    std::array<float, 256> v{};

    for (size_t i = 0; i < v.size(); i++)
        v[i] = pow((float)i / 255.0f, 2.2f);

    return v;
}

alignas(64) std::array<float, 256> srgbToLinearLUT = computeSrgbToLinearLUT();