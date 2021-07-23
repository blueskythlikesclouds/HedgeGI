#pragma once

class PropertyBag;

enum TargetEngine
{
    TARGET_ENGINE_HE1,
    TARGET_ENGINE_HE2
};

inline const char* getTargetEngineString(const TargetEngine targetEngine)
{
    switch (targetEngine)
    {
    case TARGET_ENGINE_HE1: return "Hedgehog Engine 1";
    case TARGET_ENGINE_HE2: return "Hedgehog Engine 2";
    default: return "Unknown";
    }
}

enum DenoiserType
{
    DENOISER_TYPE_NONE,
    DENOISER_TYPE_OPTIX,
    DENOISER_TYPE_OIDN
};

inline const char* getDenoiserTypeString(const DenoiserType denoiserType)
{
    switch (denoiserType)
    {
    case DENOISER_TYPE_NONE: return "None";
    case DENOISER_TYPE_OPTIX: return "Optix AI";
    case DENOISER_TYPE_OIDN: return "oidn";
    default: return "Unknown";
    }
}

struct BakeParams
{
    TargetEngine targetEngine;

    Color3 environmentColor;

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

    BakeParams() : targetEngine(TARGET_ENGINE_HE1) {}
    BakeParams(const TargetEngine targetEngine) : targetEngine(targetEngine) {}

    void load(const std::string& filePath);

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;
};