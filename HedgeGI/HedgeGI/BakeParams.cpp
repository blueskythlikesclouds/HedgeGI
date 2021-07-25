#include "BakeParams.h"
#include "PropertyBag.h"

void BakeParams::load(const std::string& filePath)
{
    INIReader reader(filePath);

    if (reader.ParseError() != 0)
        return;

    if (targetEngine == TargetEngine::HE2)
    {
        environmentColor.x() = reader.GetFloat("EnvironmentColorHE2", "R", 1.0f);
        environmentColor.y() = reader.GetFloat("EnvironmentColorHE2", "G", 1.0f);
        environmentColor.z() = reader.GetFloat("EnvironmentColorHE2", "B", 1.0f);
    }
    else
    {
        environmentColor.x() = reader.GetFloat("EnvironmentColorHE1", "R", 255.0f) / 255.0f;
        environmentColor.y() = reader.GetFloat("EnvironmentColorHE1", "G", 255.0f) / 255.0f;
        environmentColor.z() = reader.GetFloat("EnvironmentColorHE1", "B", 255.0f) / 255.0f;
    }

    lightBounceCount = reader.GetInteger("Baker", "LightBounceCount", 10);
    lightSampleCount = reader.GetInteger("Baker", "LightSampleCount", 32);
    russianRouletteMaxDepth = reader.GetInteger("Baker", "RussianRouletteMaxDepth", 4);

    shadowSampleCount = reader.GetInteger("Baker", "ShadowSampleCount", 64);
    shadowSearchRadius = reader.GetFloat("Baker", "ShadowSearchRadius", 0.01f);
    shadowBias = reader.GetFloat("Baker", "ShadowBias", 0.001f);

    aoSampleCount = reader.GetInteger("Baker", "AoSampleCount", 64);
    aoFadeConstant = reader.GetFloat("Baker", "AoFadeConstant", 1.0f);
    aoFadeLinear = reader.GetFloat("Baker", "AoFadeLinear", 0.01f);
    aoFadeQuadratic = reader.GetFloat("Baker", "AoFadeQuadratic", 0.01f);
    aoStrength = reader.GetFloat("Baker", "AoStrength", 0.0f);

    diffuseStrength = reader.GetFloat("Baker", "DiffuseStrength", 1.0f);
    diffuseSaturation = reader.GetFloat("Baker", "DiffuseSaturation", 1.0f);
    lightStrength = reader.GetFloat("Baker", "LightStrength", 1.0f);
    emissionStrength = reader.GetFloat("Baker", "EmissionStrength", 1.0f);

    resolutionBase = reader.GetFloat("Baker", "ResolutionBase", 2.0f);
    resolutionBias = reader.GetFloat("Baker", "ResolutionBias", 3.0f);
    resolutionOverride = (uint16_t)reader.GetInteger("Baker", "ResolutionOverride", -1);
    resolutionMinimum = (uint16_t)reader.GetInteger("Baker", "ResolutionMinimum", 16);
    resolutionMaximum = (uint16_t)reader.GetInteger("Baker", "ResolutionMaximum", 2048);

    denoiseShadowMap = reader.GetBoolean("Baker", "DenoiseShadowMap", true);
    optimizeSeams = reader.GetBoolean("Baker", "OptimizeSeams", true);
    denoiserType = (DenoiserType)reader.GetInteger("Baker", "DenoiserType", (long)DenoiserType::Optix);

    lightFieldMinCellRadius = reader.GetFloat("LightField", "MinCellRadius", 5.0f);
    lightFieldAabbSizeMultiplier = reader.GetFloat("LightField", "AabbSizeMultiplier", 1.0f);
}

void BakeParams::load(const PropertyBag& propertyBag)
{
    targetEngine = propertyBag.get("bakeParams.targetEngine", TargetEngine::HE1);

    environmentColor.x() = propertyBag.get("bakeParams.environmentColor.x()", 106.0f / 255.0f);
    environmentColor.y() = propertyBag.get("bakeParams.environmentColor.y()", 113.0f / 255.0f);
    environmentColor.z() = propertyBag.get("bakeParams.environmentColor.z()", 179.0f / 255.0f);

    lightBounceCount = propertyBag.get("bakeParams.lightBounceCount", 10);
    lightSampleCount = propertyBag.get("bakeParams.lightSampleCount", 32);
    russianRouletteMaxDepth = propertyBag.get("bakeParams.russianRouletteMaxDepth", 4);

    shadowSampleCount = propertyBag.get("bakeParams.shadowSampleCount", 64);
    shadowSearchRadius = propertyBag.get("bakeParams.shadowSearchRadius", 0.01f);
    shadowBias = propertyBag.get("bakeParams.shadowBias", 0.001f);

    aoSampleCount = propertyBag.get("bakeParams.aoSampleCount", 64);
    aoFadeConstant = propertyBag.get("bakeParams.aoFadeConstant", 1.0f);
    aoFadeLinear = propertyBag.get("bakeParams.aoFadeLinear", 0.01f);
    aoFadeQuadratic = propertyBag.get("bakeParams.aoFadeQuadratic", 0.01f);
    aoStrength = propertyBag.get("bakeParams.aoStrength", 0.0f);

    diffuseStrength = propertyBag.get("bakeParams.diffuseStrength", 1.0f);
    diffuseSaturation = propertyBag.get("bakeParams.diffuseSaturation", 1.0f);
    lightStrength = propertyBag.get("bakeParams.lightStrength", 1.0f);
    emissionStrength = propertyBag.get("bakeParams.emissionStrength", 1.0f);

    resolutionBase = propertyBag.get("bakeParams.resolutionBase", 2.0f);
    resolutionBias = propertyBag.get("bakeParams.resolutionBias", 3.0f);
    resolutionOverride = propertyBag.get("bakeParams.resolutionOverride", -1);
    resolutionMinimum = propertyBag.get("bakeParams.resolutionMinimum", 16);
    resolutionMaximum = propertyBag.get("bakeParams.resolutionMaximum", 2048);

    denoiseShadowMap = propertyBag.get("bakeParams.denoiseShadowMap", true);
    optimizeSeams = propertyBag.get("bakeParams.optimizeSeams", true);
    denoiserType = propertyBag.get("bakeParams.denoiserType", DenoiserType::Optix);

    lightFieldMinCellRadius = propertyBag.get("bakeParams.lightFieldMinCellRadius", 5.0f);
    lightFieldAabbSizeMultiplier = propertyBag.get("bakeParams.lightFieldAabbSizeMultiplier", 1.0f);
}

void BakeParams::store(PropertyBag& propertyBag) const
{
    propertyBag.set("bakeParams.targetEngine", targetEngine);

    propertyBag.set("bakeParams.environmentColor.x()", environmentColor.x());
    propertyBag.set("bakeParams.environmentColor.y()", environmentColor.y());
    propertyBag.set("bakeParams.environmentColor.z()", environmentColor.z());

    propertyBag.set("bakeParams.lightBounceCount", lightBounceCount);
    propertyBag.set("bakeParams.lightSampleCount", lightSampleCount);
    propertyBag.set("bakeParams.russianRouletteMaxDepth", russianRouletteMaxDepth);

    propertyBag.set("bakeParams.shadowSampleCount", shadowSampleCount);
    propertyBag.set("bakeParams.shadowSearchRadius", shadowSearchRadius);
    propertyBag.set("bakeParams.shadowBias", shadowBias);

    propertyBag.set("bakeParams.aoSampleCount", aoSampleCount);
    propertyBag.set("bakeParams.aoFadeConstant", aoFadeConstant);
    propertyBag.set("bakeParams.aoFadeLinear", aoFadeLinear);
    propertyBag.set("bakeParams.aoFadeQuadratic", aoFadeQuadratic);
    propertyBag.set("bakeParams.aoStrength", aoStrength);

    propertyBag.set("bakeParams.diffuseStrength", diffuseStrength);
    propertyBag.set("bakeParams.diffuseSaturation", diffuseSaturation);
    propertyBag.set("bakeParams.lightStrength", lightStrength);
    propertyBag.set("bakeParams.emissionStrength", emissionStrength);

    propertyBag.set("bakeParams.resolutionBase", resolutionBase);
    propertyBag.set("bakeParams.resolutionBias", resolutionBias);
    propertyBag.set("bakeParams.resolutionOverride", resolutionOverride);
    propertyBag.set("bakeParams.resolutionMinimum", resolutionMinimum);
    propertyBag.set("bakeParams.resolutionMaximum", resolutionMaximum);

    propertyBag.set("bakeParams.denoiseShadowMap", denoiseShadowMap);
    propertyBag.set("bakeParams.optimizeSeams", optimizeSeams);
    propertyBag.set("bakeParams.denoiserType", denoiserType);

    propertyBag.set("bakeParams.lightFieldMinCellRadius", lightFieldMinCellRadius);
    propertyBag.set("bakeParams.lightFieldAabbSizeMultiplier", lightFieldAabbSizeMultiplier);
}