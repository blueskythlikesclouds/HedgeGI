#include "BakeService.h"

#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "GIBaker.h"
#include "Logger.h"
#include "Stage.h"
#include "StageParams.h"
#include "Viewport.h"
#include "SHLightField.h"
#include "LightField.h"
#include "Instance.h"
#include "LightFieldBaker.h"
#include "SeamOptimizer.h"
#include "SGGIBaker.h"
#include "SHLightFieldBaker.h"

#define TRY_CANCEL() if (cancel) return;

size_t BakeService::getProgress() const
{
    return progress;
}

const Instance* BakeService::getLastBakedInstance() const
{
    return lastBakedInstance;
}

const SHLightField* BakeService::getLastBakedShlf() const
{
    return lastBakedShlf;
}

bool BakeService::isPendingCancel() const
{
    return cancel;
}

void BakeService::requestCancel()
{
    cancel = true;
}

void BakeService::bake()
{
    get<Viewport>()->waitForBake();

    progress = 0;
    lastBakedInstance = nullptr;
    lastBakedShlf = nullptr;
    cancel = false;

    const auto params = get<StageParams>();
    if (!params->validateOutputDirectoryPath(true))
        return;

    const auto begin = std::chrono::high_resolution_clock::now();

    if (params->mode == BakingFactoryMode::GI)
        bakeGI();

    else if (params->mode == BakingFactoryMode::LightField)
        bakeLightField();

    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin);

    const int seconds = (int)(duration.count() % 60);
    const int minutes = (int)((duration.count() / 60) % 60);
    const int hours = (int)(duration.count() / (60 * 60));

    Logger::logFormatted(LogType::Success, "Bake completed in %02dh:%02dm:%02ds!", hours, minutes, seconds);
}

void BakeService::bakeGI()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    std::for_each(std::execution::par_unseq, stage->getScene()->instances.begin(), stage->getScene()->instances.end(), [&](const std::unique_ptr<const Instance>& instance)
    {
        TRY_CANCEL()

        bool skip = instance->name.find("_NoGI") != std::string::npos || instance->name.find("_noGI") != std::string::npos;
    
        const bool isSg = !skip && params->bakeParams.targetEngine == TargetEngine::HE2 && params->propertyBag.get(instance->name + ".isSg", true);
    
        std::string lightMapFileName;
        std::string shadowMapFileName;
    
        if (!skip)
        {
            if (params->bakeParams.targetEngine == TargetEngine::HE2)
            {
                lightMapFileName = isSg ? instance->name + "_sg.dds" : instance->name + ".dds";
                shadowMapFileName = instance->name + "_occlusion.dds";
            }
            else if (stage->getGameType() == GameType::LostWorld)
            {
                lightMapFileName = instance->name + ".dds";
            }
            else
            {
                lightMapFileName = instance->name + "_lightmap.png";
                shadowMapFileName = instance->name + "_shadowmap.png";
            }
        }
    
        lightMapFileName = params->outputDirectoryPath + "/" + lightMapFileName;
        skip |= params->skipExistingFiles && std::filesystem::exists(lightMapFileName);
    
        if (!shadowMapFileName.empty())
        {
            shadowMapFileName = params->outputDirectoryPath + "/" + shadowMapFileName;
            skip |= params->skipExistingFiles && std::filesystem::exists(shadowMapFileName);
        }
    
        if (skip)
        {
            Logger::logFormatted(LogType::Normal, "Skipped %s", instance->name.c_str());

            ++progress;
            lastBakedInstance = instance.get();
            return;
        }

        const uint16_t resolution = params->bakeParams.resolutionOverride > 0 ? params->bakeParams.resolutionOverride :
            params->propertyBag.get(instance->name + ".resolution", 256);
    
        if (isSg)
        {
            GIPair pair;
            {
                const std::lock_guard<CriticalSection> lock = BakingFactory::lock();
                {
                    ++progress;
                    lastBakedInstance = instance.get();
                }

                TRY_CANCEL()

                pair = SGGIBaker::bake(stage->getScene()->getRaytracingContext(), *instance, resolution, params->bakeParams);
            }

            TRY_CANCEL()
    
            // Dilate
            pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
            pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

            TRY_CANCEL()

            // Denoise
            if (params->bakeParams.getDenoiserType() != DenoiserType::None)
            {
                pair.lightMap = BitmapHelper::denoise(*pair.lightMap, params->bakeParams.getDenoiserType());
    
                if (params->bakeParams.denoiseShadowMap)
                    pair.shadowMap = BitmapHelper::denoise(*pair.shadowMap, params->bakeParams.getDenoiserType());
            }

            TRY_CANCEL()
    
            if (params->bakeParams.optimizeSeams)
            {
                const SeamOptimizer seamOptimizer(*instance);
                pair.lightMap = seamOptimizer.optimize(*pair.lightMap);
                pair.shadowMap = seamOptimizer.optimize(*pair.shadowMap);
            }

            TRY_CANCEL()
    
            pair.lightMap->save(lightMapFileName, stage->getGameType() == GameType::Generations ? DXGI_FORMAT_R16G16B16A16_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT);
            pair.shadowMap->save(shadowMapFileName, stage->getGameType() == GameType::Generations ? DXGI_FORMAT_R8_UNORM : SGGIBaker::SHADOW_MAP_FORMAT);
        }
        else
        {
            GIPair pair;
            {
                const std::lock_guard<CriticalSection> lock = BakingFactory::lock();
                {
                    ++progress;
                    lastBakedInstance = instance.get();
                }

                TRY_CANCEL()

                pair = GIBaker::bake(stage->getScene()->getRaytracingContext(), *instance, resolution, params->bakeParams);
            }

            TRY_CANCEL()
    
            // Dilate
            pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
            pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

            TRY_CANCEL()

            // Combine
            auto combined = BitmapHelper::combine(*pair.lightMap, *pair.shadowMap);

            TRY_CANCEL()

            // Denoise
            if (params->bakeParams.getDenoiserType() != DenoiserType::None)
                combined = BitmapHelper::denoise(*combined, params->bakeParams.getDenoiserType(), params->bakeParams.denoiseShadowMap);

            TRY_CANCEL()

            // Optimize seams
            if (params->bakeParams.optimizeSeams)
                combined = BitmapHelper::optimizeSeams(*combined, *instance);

            TRY_CANCEL()

            // Make ready for encoding
            if (params->bakeParams.targetEngine == TargetEngine::HE1)
                combined = BitmapHelper::makeEncodeReady(*combined, ENCODE_READY_FLAGS_SQRT);

            TRY_CANCEL()

            if (stage->getGameType() == GameType::Generations && params->bakeParams.targetEngine == TargetEngine::HE1)
            {
                combined->save(lightMapFileName, Bitmap::transformToLightMap);
                combined->save(shadowMapFileName, Bitmap::transformToShadowMap);
            }
            else if (stage->getGameType() == GameType::LostWorld)
            {
                combined->save(lightMapFileName, DXGI_FORMAT_BC3_UNORM);
            }
            else if (params->bakeParams.targetEngine == TargetEngine::HE2)
            {
                combined->save(lightMapFileName, stage->getGameType() == GameType::Generations ? DXGI_FORMAT_R16G16B16A16_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT, Bitmap::transformToLightMap);
                combined->save(shadowMapFileName, stage->getGameType() == GameType::Generations ? DXGI_FORMAT_R8_UNORM : SGGIBaker::SHADOW_MAP_FORMAT, Bitmap::transformToShadowMap);
            }
        }
    });

    ++progress;
    lastBakedInstance = nullptr;
}

void BakeService::bakeLightField()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    if (params->bakeParams.targetEngine == TargetEngine::HE2)
    {
        std::for_each(std::execution::par_unseq, stage->getScene()->shLightFields.begin(), stage->getScene()->shLightFields.end(), [&](const std::unique_ptr<SHLightField>& shlf)
        {            
            ++progress;
            lastBakedShlf = shlf.get();

            TRY_CANCEL()

            auto bitmap = SHLightFieldBaker::bake(stage->getScene()->getRaytracingContext(), *shlf, params->bakeParams);

            TRY_CANCEL()

            if (params->bakeParams.getDenoiserType() != DenoiserType::None)
                bitmap = BitmapHelper::denoise(*bitmap, params->bakeParams.getDenoiserType());

            TRY_CANCEL()

            bitmap->save(params->outputDirectoryPath + "/" + shlf->name + ".dds", DXGI_FORMAT_R16G16B16A16_FLOAT);
        });

        ++progress;
        lastBakedShlf = nullptr;
    }

    else if (params->bakeParams.targetEngine == TargetEngine::HE1)
    {
        TRY_CANCEL()

        std::unique_ptr<LightField> lightField = LightFieldBaker::bake(stage->getScene()->getRaytracingContext(), params->bakeParams);

        Logger::log(LogType::Normal, "Saving...\n");

        TRY_CANCEL()

        lightField->save(params->outputDirectoryPath + "/light-field.lft");
    }
}
