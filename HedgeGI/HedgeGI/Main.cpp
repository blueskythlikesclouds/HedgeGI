#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "GIBaker.h"
#include "Scene.h"
#include "SceneFactory.h"
#include "SGGIBaker.h"
#include <fstream>

int32_t main(int32_t argc, const char* argv[])
{
    BakeParams bakeParams{};
    bakeParams.load(getDirectoryPath(argv[0]) + "/HedgeGI.ini");

    Eigen::initParallel();
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    std::unique_ptr<Scene> scene;

    const std::string path = argv[1];

    if (std::filesystem::exists(path + ".scene"))
    {
        scene = std::make_unique<Scene>();
        scene->load(path + ".scene");
    }
    else
    {
        scene = SceneFactory::createFromGenerations(path);
        scene->save(path + ".scene");
    }

    std::unordered_map<std::string, uint16_t> resolutions;
    std::ifstream stream(path + ".txt");

    if (stream.is_open())
    {
        uint16_t resolution;
        std::string name;

        while (stream >> resolution >> name)
            resolutions[name] = resolution;
    }

    const auto raytracingContext = scene->createRaytracingContext();

    size_t i = 0;
    std::for_each(std::execution::par_unseq, scene->instances.begin(), scene->instances.end(), [&i, &resolutions, &bakeParams, &scene, &raytracingContext](const std::unique_ptr<const Instance>& instance)
    {
        const uint16_t resolution = resolutions.find(instance->name) != resolutions.end() ? std::max<uint16_t>(64, resolutions[instance->name]) : bakeParams.defaultResolution;

        // GI Test (Generations)
        const auto bitmaps = GIBaker::bakeSeparate(raytracingContext, *instance, resolution, bakeParams);

        BitmapHelper::encodeReady(*BitmapHelper::optimizeSeams(*BitmapHelper::denoise(*BitmapHelper::dilate(*bitmaps.first)), *instance), (EncodeReadyFlags)(ENCODE_READY_FLAGS_SRGB | ENCODE_READY_FLAGS_SQRT))->save(instance->name + "_lightmap.png");
        BitmapHelper::optimizeSeams(*BitmapHelper::dilate(*bitmaps.second), *instance)->save(instance->name + "_shadowmap.png");

        printf("(%llu/%llu): Saved %s (%dx%d)\n", InterlockedIncrement(&i), scene->instances.size(), instance->name.c_str(), resolution, resolution);
    });

    printf("Completed!\n");
    getchar();

    return 0;
}
