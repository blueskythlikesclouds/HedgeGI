#pragma once

class PropertyBag;

enum class EnvironmentMode
{
    Color,
    Sky,
    TwoColor
};

struct EnvironmentParams
{
    EnvironmentMode mode;

    Color3 color;
    Color3 secondaryColor;
    float colorIntensity;

    float skyIntensity;
    float skyIntensityScale;
};

struct LightParams
{
    uint32_t sampleCount;
    uint32_t bounceCount;
    uint32_t maxRussianRouletteDepth;
};

struct ShadowParams
{
    uint32_t sampleCount;
    float radius;
    float bias;
};

struct MaterialParams
{
    float diffuseIntensity;
    float diffuseSaturation;
    float lightIntensity;
    float emissionIntensity;
};

struct ResolutionParams
{
    float base;
    float bias;
    uint16_t min;
    uint16_t max;
    int16_t override;
};

enum class DenoiserType
{
    None,
    Optix,
    Oidn
};

struct PostProcessParams
{
    DenoiserType denoiserType;
    bool denoiseShadowMap;
    bool optimizeSeams;
};

struct LightFieldParams
{
    float minCellRadius;
    float aabbSizeMultiplier;
};

enum class TargetEngine
{
    HE1,
    HE2
};

struct BakeParams
{
    TargetEngine targetEngine;
    EnvironmentParams environment;
    LightParams light;
    ShadowParams shadow;
    MaterialParams material;
    ResolutionParams resolution;
    PostProcessParams postProcess;
    LightFieldParams lightField;

    void load(const PropertyBag& propertyBag);
    void store(PropertyBag& propertyBag) const;

    DenoiserType getDenoiserType() const;
};