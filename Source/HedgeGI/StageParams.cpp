#include "StageParams.h"

#include "CameraController.h"
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
}
