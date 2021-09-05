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
    Sky,
    TwoColor
};

struct BakeParams
{
    TargetEngine targetEngine;

    EnvironmentColorMode environmentColorMode;
    Color3 environmentColor;
    float environmentColorIntensity;
    float skyIntensity;
    float skyIntensityScale;
    Color3 secondaryEnvironmentColor;

    uint32_t lightBounceCount {};
    uint32_t lightSampleCount {};
    uint32_t russianRouletteMaxDepth {};

    uint32_t shadowSampleCount {};
    float shadowSearchRadius {};
    float shadowBias {};

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

    BakeParams() : targetEngine(TargetEngine::HE1), skyIntensityScale(1) {}
    BakeParams(const TargetEngine targetEngine) : targetEngine(targetEngine), skyIntensityScale(1) {}

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;
};