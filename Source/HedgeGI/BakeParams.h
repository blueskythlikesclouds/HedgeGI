#pragma once

class PropertyBag;

enum class TargetEngine
{
    HE1,
    HE2
};

enum class DenoiserType
{
    None,
    Optix,
    Oidn
};

enum class EnvironmentColorMode
{
    Color,
    Sky
};

struct BakeParams
{
    TargetEngine targetEngine;

    EnvironmentColorMode environmentColorMode;
    Color3 environmentColor;
    float environmentColorIntensity;
    float skyIntensity;

    uint32_t lightBounceCount {};
    uint32_t lightSampleCount {};
    uint32_t russianRouletteMaxDepth {};

    uint32_t shadowSampleCount {};
    float shadowSearchRadius {};
    float shadowBias {};

    uint32_t aoSampleCount {};
    float aoFadeConstant {};
    float aoFadeLinear {};
    float aoFadeQuadratic {};
    float aoStrength {};

    float diffuseStrength {};
    float diffuseSaturation {};
    float lightStrength {};
    float emissionStrength {};

    float resolutionBase {};
    float resolutionBias {};
    int16_t resolutionOverride {};
    uint16_t resolutionMinimum {};
    uint16_t resolutionMaximum {};

    bool denoiseShadowMap {};
    bool optimizeSeams {};
    DenoiserType denoiserType {};

    float lightFieldMinCellRadius {};
    float lightFieldAabbSizeMultiplier {};

    BakeParams() : targetEngine(TargetEngine::HE1) {}
    BakeParams(const TargetEngine targetEngine) : targetEngine(targetEngine) {}

    void load(const std::string& filePath);

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;
};