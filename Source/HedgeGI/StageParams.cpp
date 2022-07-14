#include "StageParams.h"

#include "CameraController.h"
#include "Logger.h"
#include "Stage.h"

void StageParams::loadProperties()
{
    const auto stage = get<Stage>();

    get<CameraController>()->load(propertyBag);
    bakeParams.load(propertyBag);
    viewportResolutionInvRatio = propertyBag.get(PROP("viewportResolutionInvRatio"), 2.0f);
    gammaCorrectionFlag = propertyBag.get(PROP("gammaCorrectionFlag"), false);
    colorCorrectionFlag = propertyBag.get(PROP("colorCorrectionFlag"), true);
    outputDirectoryPath = propertyBag.getString(PROP("outputDirectoryPath"), stage->getDirectoryPath() + "-HedgeGI");
    mode = propertyBag.get(PROP("mode"), BakingFactoryMode::GI);
    skipExistingFiles = propertyBag.get(PROP("skipExistingFiles"), false);
    resolutionSuperSampleScale = propertyBag.get(PROP("resolutionSuperSampleScale"), 1);
    useExistingLightField = propertyBag.get(PROP("useExistingLightField"), false);

    if (stage->getGameType() == GameType::Forces)
        bakeParams.targetEngine = TargetEngine::HE2;
}

void StageParams::storeProperties()
{
    get<CameraController>()->store(propertyBag);
    bakeParams.store(propertyBag);
    propertyBag.set(PROP("viewportResolutionInvRatio"), viewportResolutionInvRatio);
    propertyBag.set(PROP("gammaCorrectionFlag"), gammaCorrectionFlag);
    propertyBag.set(PROP("colorCorrectionFlag"), colorCorrectionFlag);
    propertyBag.setString(PROP("outputDirectoryPath"), outputDirectoryPath);
    propertyBag.set(PROP("mode"), mode);
    propertyBag.set(PROP("skipExistingFiles"), skipExistingFiles);
    propertyBag.set(PROP("resolutionSuperSampleScale"), resolutionSuperSampleScale);
    propertyBag.set(PROP("useExistingLightField"), useExistingLightField);
}

bool StageParams::validateOutputDirectoryPath(const bool create) const
{
    if (create)
    {
        std::error_code errorCode;
        std::filesystem::create_directory(outputDirectoryPath, errorCode);
    }

    // The directory might fail to be created if the HGI file was received from a different computer
    if (std::filesystem::exists(outputDirectoryPath))
        return true;

    Logger::log(LogType::Error, "Unable to locate output directory path");
    return false;
}
