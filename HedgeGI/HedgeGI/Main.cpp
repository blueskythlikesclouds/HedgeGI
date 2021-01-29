#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "GIBaker.h"
#include "Scene.h"
#include "SceneFactory.h"
#include "SGGIBaker.h"
#include "LightField.h"
#include <fstream>

#include "LightFieldBaker.h"

int32_t main(int32_t argc, const char* argv[])
{
    BakeParams bakeParams{};
    bakeParams.load(getDirectoryPath(argv[0]) + "/HedgeGI.ini");

    Eigen::initParallel();
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    std::unique_ptr<Scene> scene;

    const std::string path = argv[1];
    const std::string outputPath = std::string(argv[1]) + "-HedgeGI/";

    const bool isGenerations = std::filesystem::exists(path + "/Stage.pfd");

    if (std::filesystem::exists(path + ".scene"))
    {
        scene = std::make_unique<Scene>();
        scene->load(path + ".scene");
    }
    else
    {
        scene = isGenerations ? SceneFactory::createFromGenerations(path) : SceneFactory::createFromLostWorldOrForces(path);
        scene->save(path + ".scene");
    }

    std::filesystem::create_directory(outputPath);

    const auto raytracingContext = scene->createRaytracingContext();

    // GI Test
    if (true)
    {
        phmap::parallel_flat_hash_map<std::string, uint16_t> resolutions;
        std::ifstream stream(path + ".txt");

        if (stream.is_open())
        {
            uint16_t resolution;
            std::string name;

            while (stream >> resolution >> name)
                resolutions[name] = resolution;
        }

        size_t i = 0;
        std::for_each(std::execution::par_unseq, scene->instances.begin(), scene->instances.end(),
            [&i, &resolutions, &bakeParams, &raytracingContext, &isGenerations, &outputPath](const std::unique_ptr<const Instance>& instance)
        {
            const uint16_t resolution = resolutions.find(instance->name) != resolutions.end() ? std::max<uint16_t>(64, resolutions[instance->name]) : bakeParams.defaultResolution;

            // GI Test
            auto pair = GIBaker::bake(raytracingContext, *instance, resolution, bakeParams);

            // Dilate
            pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
            pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

            // Denoise
            pair.lightMap = BitmapHelper::denoise(*pair.lightMap);

            // Combine
            auto combined = BitmapHelper::combine(*pair.lightMap, *pair.shadowMap);

            // Make ready for encoding
            combined = BitmapHelper::makeEncodeReady(*combined, (EncodeReadyFlags)(ENCODE_READY_FLAGS_SRGB | ENCODE_READY_FLAGS_SQRT));

            if (isGenerations)
            {
                combined->save(outputPath + instance->name + "_lightmap.png", Bitmap::transformToLightMap);
                combined->save(outputPath + instance->name + "_shadowmap.png", Bitmap::transformToShadowMap);
            }
            else
            {
                combined->save(outputPath + instance->name + ".dds", DXGI_FORMAT_BC3_UNORM);
            }
           
            printf("(%llu/%llu): Saved %s (%dx%d)\n", InterlockedIncrement(&i), raytracingContext.scene->instances.size(), instance->name.c_str(), resolution, resolution);
        });
    }
    // Light Field Test
    else
    {
        auto lightField = LightFieldBaker::bake(raytracingContext, bakeParams);
        lightField->save("light-field.lft");
    }

    printf("Completed!\n");
    getchar();

    return 0;
}
