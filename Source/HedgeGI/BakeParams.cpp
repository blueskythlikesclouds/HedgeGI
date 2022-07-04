#include "BakeParams.h"
#include "PropertyBag.h"

#include "OidnDenoiserDevice.h"
#include "OptixDenoiserDevice.h"

void BakeParams::load(const PropertyBag& propertyBag)
{
    // Outdated property names remain in this function for backwards compatibility with old .hgi files.

    targetEngine = propertyBag.get(PROP("bakeParams.targetEngine"), TargetEngine::HE1);

    environment.mode = propertyBag.get(PROP("bakeParams.environmentColorMode"), EnvironmentMode::Sky);
    environment.color.x() = propertyBag.get(PROP("bakeParams.environmentColor.x()"), 106.0f / 255.0f);
    environment.color.y() = propertyBag.get(PROP("bakeParams.environmentColor.y()"), 113.0f / 255.0f);
    environment.color.z() = propertyBag.get(PROP("bakeParams.environmentColor.z()"), 179.0f / 255.0f);
    environment.colorIntensity = propertyBag.get(PROP("bakeParams.environmentColorIntensity"), 1.0f);
    environment.skyIntensity = propertyBag.get(PROP("bakeParams.skyIntensity"), 1.0f);
    environment.secondaryColor.x() = propertyBag.get(PROP("bakeParams.secondaryEnvironmentColor.x()"), 106.0f / 255.0f);
    environment.secondaryColor.y() = propertyBag.get(PROP("bakeParams.secondaryEnvironmentColor.y()"), 113.0f / 255.0f);
    environment.secondaryColor.z() = propertyBag.get(PROP("bakeParams.secondaryEnvironmentColor.z()"), 179.0f / 255.0f);

    light.bounceCount = propertyBag.get(PROP("bakeParams.lightBounceCount"), 10);
    light.sampleCount = propertyBag.get(PROP("bakeParams.lightSampleCount"), 32);
    light.maxRussianRouletteDepth = propertyBag.get(PROP("bakeParams.russianRouletteMaxDepth"), 4);

    shadow.sampleCount = propertyBag.get(PROP("bakeParams.shadowSampleCount"), 64);
    shadow.radius = propertyBag.get(PROP("bakeParams.shadowSearchRadius"), 0.01f);
    shadow.bias = propertyBag.get(PROP("bakeParams.shadowBias"), 0.001f);

    material.diffuseIntensity = propertyBag.get(PROP("bakeParams.diffuseStrength"), 1.0f);
    material.diffuseSaturation = propertyBag.get(PROP("bakeParams.diffuseSaturation"), 1.0f);
    material.lightIntensity = propertyBag.get(PROP("bakeParams.lightStrength"), 1.0f);
    material.emissionIntensity = propertyBag.get(PROP("bakeParams.emissionStrength"), 1.0f);

    resolution.base = propertyBag.get(PROP("bakeParams.resolutionBase"), 2.0f);
    resolution.bias = propertyBag.get(PROP("bakeParams.resolutionBias"), 3.0f);
    resolution.override = propertyBag.get(PROP("bakeParams.resolutionOverride"), -1);
    resolution.min = propertyBag.get(PROP("bakeParams.resolutionMinimum"), 16);
    resolution.max = propertyBag.get(PROP("bakeParams.resolutionMaximum"), 2048);

    postProcess.denoiseShadowMap = propertyBag.get(PROP("bakeParams.denoiseShadowMap"), true);
    postProcess.optimizeSeams = propertyBag.get(PROP("bakeParams.optimizeSeams"), true);
    postProcess.denoiserType = propertyBag.get(PROP("bakeParams.denoiserType"), 
        OptixDenoiserDevice::available ? DenoiserType::Optix : OidnDenoiserDevice::available ? DenoiserType::Oidn : DenoiserType::None);

    lightField.minCellRadius = propertyBag.get(PROP("bakeParams.lightFieldMinCellRadius"), 5.0f);
    lightField.aabbSizeMultiplier = propertyBag.get(PROP("bakeParams.lightFieldAabbSizeMultiplier"), 1.0f);
}

void BakeParams::store(PropertyBag& propertyBag) const
{
    propertyBag.set(PROP("bakeParams.targetEngine"), targetEngine);

    propertyBag.set(PROP("bakeParams.environmentColorMode"), environment.mode);
    propertyBag.set(PROP("bakeParams.environmentColor.x()"), environment.color.x());
    propertyBag.set(PROP("bakeParams.environmentColor.y()"), environment.color.y());
    propertyBag.set(PROP("bakeParams.environmentColor.z()"), environment.color.z());
    propertyBag.set(PROP("bakeParams.environmentColorIntensity"), environment.colorIntensity);
    propertyBag.set(PROP("bakeParams.skyIntensity"), environment.skyIntensity);
    propertyBag.set(PROP("bakeParams.secondaryEnvironmentColor.x()"), environment.secondaryColor.x());
    propertyBag.set(PROP("bakeParams.secondaryEnvironmentColor.y()"), environment.secondaryColor.y());
    propertyBag.set(PROP("bakeParams.secondaryEnvironmentColor.z()"), environment.secondaryColor.z());

    propertyBag.set(PROP("bakeParams.lightBounceCount"), light.bounceCount);
    propertyBag.set(PROP("bakeParams.lightSampleCount"), light.sampleCount);
    propertyBag.set(PROP("bakeParams.russianRouletteMaxDepth"), light.maxRussianRouletteDepth);

    propertyBag.set(PROP("bakeParams.shadowSampleCount"), shadow.sampleCount);
    propertyBag.set(PROP("bakeParams.shadowSearchRadius"), shadow.radius);
    propertyBag.set(PROP("bakeParams.shadowBias"), shadow.bias);

    propertyBag.set(PROP("bakeParams.diffuseStrength"), material.diffuseIntensity);
    propertyBag.set(PROP("bakeParams.diffuseSaturation"), material.diffuseSaturation);
    propertyBag.set(PROP("bakeParams.lightStrength"), material.lightIntensity);
    propertyBag.set(PROP("bakeParams.emissionStrength"), material.emissionIntensity);

    propertyBag.set(PROP("bakeParams.resolutionBase"), resolution.base);
    propertyBag.set(PROP("bakeParams.resolutionBias"), resolution.bias);
    propertyBag.set(PROP("bakeParams.resolutionOverride"), resolution.override);
    propertyBag.set(PROP("bakeParams.resolutionMinimum"), resolution.min);
    propertyBag.set(PROP("bakeParams.resolutionMaximum"), resolution.max);

    propertyBag.set(PROP("bakeParams.denoiseShadowMap"), postProcess.denoiseShadowMap);
    propertyBag.set(PROP("bakeParams.optimizeSeams"), postProcess.optimizeSeams);
    propertyBag.set(PROP("bakeParams.denoiserType"), postProcess.denoiserType);

    propertyBag.set(PROP("bakeParams.lightFieldMinCellRadius"), lightField.minCellRadius);
    propertyBag.set(PROP("bakeParams.lightFieldAabbSizeMultiplier"), lightField.aabbSizeMultiplier);
}

DenoiserType BakeParams::getDenoiserType() const
{
    if (!OidnDenoiserDevice::available && postProcess.denoiserType == DenoiserType::Oidn) return DenoiserType::None;
    if (!OptixDenoiserDevice::available && postProcess.denoiserType == DenoiserType::Optix) return DenoiserType::None;

    return postProcess.denoiserType;
}