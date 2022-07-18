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

struct GIBakerContext
{
    const Instance* instance{};
    std::string lightMapFileName;
    std::string shadowMapFileName;
    uint16_t resolution{};
    GIPair pair;
    std::unique_ptr<Bitmap> combined;
};

typedef std::shared_ptr<GIBakerContext> GIBakerContextPtr;
typedef tbb::flow::function_node<GIBakerContextPtr, GIBakerContextPtr> GIBakerFunctionNode;

struct SHLFBakerContext
{
    const SHLightField* shlf{};
    std::unique_ptr<Bitmap> bitmap;

    SHLFBakerContext(const SHLightField* shlf)
        : shlf(shlf)
    {
    }
};

typedef std::shared_ptr<SHLFBakerContext> SHLFBakerContextPtr;
typedef tbb::flow::function_node<SHLFBakerContextPtr, SHLFBakerContextPtr> SHLFBakerFunctionNode;

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
    g.cancel();
    cancel = true;
}

void BakeService::bake()
{
    g.reset();
    progress = 0;
    lastBakedInstance = nullptr;
    lastBakedShlf = nullptr;
    cancel = false;

    get<Viewport>()->waitForBake();

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
    const auto scene = stage->getScene();
    const auto params = get<StageParams>();

    //====// 
    // GI //
    //====//

    GIBakerFunctionNode bake(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->pair = GIBaker::bake(scene->getRaytracingContext(), *context->instance, context->resolution, params->bakeParams);

        ++progress;
        lastBakedInstance = context->instance;

        return std::move(context);
    });

    GIBakerFunctionNode dilate(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->pair.lightMap = BitmapHelper::dilate(*context->pair.lightMap);
        context->pair.shadowMap = BitmapHelper::dilate(*context->pair.shadowMap);

        return std::move(context);
    });

    GIBakerFunctionNode combine(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->combined = BitmapHelper::combine(*context->pair.lightMap, *context->pair.shadowMap);
        return std::move(context);
    });

    GIBakerFunctionNode denoise(g, 1, [=](GIBakerContextPtr context)
    {
        context->combined = BitmapHelper::denoise(*context->combined, params->bakeParams.getDenoiserType(), params->bakeParams.postProcess.denoiseShadowMap);
        return std::move(context);
    });

    GIBakerFunctionNode optimizeSeams(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->combined = BitmapHelper::optimizeSeams(*context->combined, *context->instance);
        return std::move(context);
    });

    GIBakerFunctionNode encodeReady(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->combined = BitmapHelper::makeEncodeReady(*context->combined, ENCODE_READY_FLAGS_SQRT);
        return std::move(context);
    });

    GIBakerFunctionNode saveSeparated(g, 1, [=](GIBakerContextPtr context)
    {
        context->combined->save(context->lightMapFileName, stage->getGameType() == GameType::Generations ? DXGI_FORMAT_R16G16B16A16_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT,
            Bitmap::transformToLightMap, params->resolutionSuperSampleScale);

        context->combined->save(context->shadowMapFileName, stage->getGameType() == GameType::Generations ? DXGI_FORMAT_R8_UNORM : SGGIBaker::SHADOW_MAP_FORMAT,
            Bitmap::transformToShadowMap, params->resolutionSuperSampleScale);

        return std::move(context);
    });

    GIBakerFunctionNode save(g, tbb::flow::unlimited, [=, &saveSeparated](GIBakerContextPtr context)
    {
        if (stage->getGameType() == GameType::Generations && params->bakeParams.targetEngine == TargetEngine::HE1)
        {
            context->combined->save(context->lightMapFileName, Bitmap::transformToLightMap, params->resolutionSuperSampleScale);
            context->combined->save(context->shadowMapFileName, Bitmap::transformToShadowMap, params->resolutionSuperSampleScale);
        }
        else if (stage->getGameType() == GameType::LostWorld)
        {
            context->combined->save(context->lightMapFileName, DXGI_FORMAT_BC3_UNORM, nullptr, params->resolutionSuperSampleScale);
        }
        else if (params->bakeParams.targetEngine == TargetEngine::HE2)
            saveSeparated.try_put(context);

        return std::move(context);
    });

    // bake -> dilate -> combine -> denoise -> optimizeSeams -> encodeReady -> save
    tbb::flow::make_edge(bake, dilate);
    tbb::flow::make_edge(dilate, combine);

    GIBakerFunctionNode* output = &combine;

    if (params->bakeParams.getDenoiserType() != DenoiserType::None)
    {
        tbb::flow::make_edge(*output, denoise);
        output = &denoise;
    }

    if (params->bakeParams.postProcess.optimizeSeams)
    {
        tbb::flow::make_edge(*output, optimizeSeams);
        output = &optimizeSeams;
    }

    if (params->bakeParams.targetEngine == TargetEngine::HE1)
    {
        tbb::flow::make_edge(*output, encodeReady);
        output = &encodeReady;
    }

    tbb::flow::make_edge(*output, save);

    //======//
    // SGGI //
    //======//

    GIBakerFunctionNode bakeSg(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->pair = SGGIBaker::bake(scene->getRaytracingContext(), *context->instance, context->resolution, params->bakeParams);

        ++progress;
        lastBakedInstance = context->instance;

        return std::move(context);
    });

    GIBakerFunctionNode dilateSg(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        context->pair.lightMap = BitmapHelper::dilate(*context->pair.lightMap);
        context->pair.shadowMap = BitmapHelper::dilate(*context->pair.shadowMap);

        return std::move(context);
    });

    GIBakerFunctionNode denoiseSg(g, 1, [=](GIBakerContextPtr context)
    {
        context->pair.lightMap = BitmapHelper::denoise(*context->pair.lightMap, params->bakeParams.getDenoiserType());

        if (params->bakeParams.postProcess.denoiseShadowMap)
            context->pair.shadowMap = BitmapHelper::denoise(*context->pair.shadowMap, params->bakeParams.getDenoiserType());

        return std::move(context);
    });
	
    GIBakerFunctionNode optimizeSeamsSg(g, tbb::flow::unlimited, [=](GIBakerContextPtr context)
    {
        const SeamOptimizer seamOptimizer(*context->instance);
        context->pair.lightMap = seamOptimizer.optimize(*context->pair.lightMap);
        context->pair.shadowMap = seamOptimizer.optimize(*context->pair.shadowMap);

        return std::move(context);
    });
	
    GIBakerFunctionNode saveSgCompressed(g, 1, [=](GIBakerContextPtr context)
    {
        context->pair.lightMap->save(context->lightMapFileName, SGGIBaker::LIGHT_MAP_FORMAT, nullptr, params->resolutionSuperSampleScale);
        context->pair.shadowMap->save(context->shadowMapFileName, SGGIBaker::SHADOW_MAP_FORMAT, nullptr, params->resolutionSuperSampleScale);

        return std::move(context);
    });

    GIBakerFunctionNode saveSg(g, tbb::flow::unlimited, [=, &saveSgCompressed](GIBakerContextPtr context)
    {
        if (stage->getGameType() == GameType::Generations)
        {
            context->pair.lightMap->save(context->lightMapFileName, DXGI_FORMAT_R16G16B16A16_FLOAT, nullptr, params->resolutionSuperSampleScale);
            context->pair.shadowMap->save(context->shadowMapFileName, DXGI_FORMAT_R8_UNORM, nullptr, params->resolutionSuperSampleScale);
        }

        else
            saveSgCompressed.try_put(context);

        return std::move(context);
    });

    // bakeSg -> dilateSg -> denoiseSg -> optimizeSeamsSg -> saveSg
    tbb::flow::make_edge(bakeSg, dilateSg);

    output = &dilateSg;

    if (params->bakeParams.getDenoiserType() != DenoiserType::None)
    {
        tbb::flow::make_edge(*output, denoiseSg);
        output = &denoiseSg;
    }

    if (params->bakeParams.postProcess.optimizeSeams)
    {
        tbb::flow::make_edge(*output, optimizeSeamsSg);
        output = &optimizeSeamsSg;
    }

    tbb::flow::make_edge(*output, saveSg);

    tbb::flow::function_node<const Instance*> root(g, tbb::flow::unlimited, [=, &bake, &bakeSg](const Instance* instance)
    {
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
            lastBakedInstance = instance;
            return;
        }

        auto context = std::make_shared<GIBakerContext>();

        context->instance = instance;

        context->resolution = (uint16_t)((params->bakeParams.resolution.override > 0 ? params->bakeParams.resolution.override :
            instance->getResolution(params->propertyBag)) * params->resolutionSuperSampleScale);

        context->lightMapFileName = std::move(lightMapFileName);
        context->shadowMapFileName = std::move(shadowMapFileName);

        (isSg ? bakeSg : bake).try_put(std::move(context));
    });

    for (auto& instance : scene->instances)
        root.try_put(instance.get());

    g.wait_for_all();
}

void BakeService::bakeLightField()
{
    const auto stage = get<Stage>();
    const auto scene = stage->getScene();
    const auto params = get<StageParams>();

    if (params->bakeParams.targetEngine == TargetEngine::HE2)
    {
        SHLFBakerFunctionNode bake(g, tbb::flow::unlimited, [=](SHLFBakerContextPtr context)
        {
            context->bitmap = SHLightFieldBaker::bake(scene->getRaytracingContext(), *context->shlf, params->bakeParams);

            ++progress;
            lastBakedShlf = context->shlf;

            return std::move(context);
        });      

        SHLFBakerFunctionNode denoise(g, 1, [=](SHLFBakerContextPtr context)
        {
            context->bitmap = BitmapHelper::denoise(*context->bitmap, params->bakeParams.getDenoiserType());
            return std::move(context);
        });

        SHLFBakerFunctionNode save(g, tbb::flow::unlimited, [=](SHLFBakerContextPtr context)
        {
            context->bitmap->save(params->outputDirectoryPath + "/" + context->shlf->name + ".dds", DXGI_FORMAT_R16G16B16A16_FLOAT);
            return std::move(context);
        });

        if (params->bakeParams.getDenoiserType() != DenoiserType::None)
        {
            tbb::flow::make_edge(bake, denoise);
            tbb::flow::make_edge(denoise, save);
        }
        else
            tbb::flow::make_edge(bake, save);

        for (auto& shlf : scene->shLightFields)
            bake.try_put(std::make_shared<SHLFBakerContext>(shlf.get()));

        g.wait_for_all();
    }

    else if (params->bakeParams.targetEngine == TargetEngine::HE1)
    {
        if (cancel)
            return;

        LightFieldBaker::bake(scene->lightField, scene->getRaytracingContext(), params->bakeParams, !params->useExistingLightField);

        Logger::log(LogType::Normal, "Saving...\n");

        if (!cancel)
            scene->lightField.save(params->outputDirectoryPath + "/light-field.lft");

        // Keep cells for future baking processes
        scene->lightField.clear(false);

        if (!params->useExistingLightField)
            Logger::log(LogType::Warning, "Pre-generated light field tree data has been replaced in the current HedgeGI session");
    }
}
