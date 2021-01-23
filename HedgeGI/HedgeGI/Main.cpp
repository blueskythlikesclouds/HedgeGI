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

    //const auto scene = SceneFactory::createFromGenerations("D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\Water Palace\\disk\\bb\\Packed\\ghz200");
    const auto raytracingContext = scene->createRaytracingContext();

    size_t i = 0;
    for (auto& instance : scene->instances)
    {
        const uint16_t resolution = resolutions.find(instance->name) != resolutions.end() ? std::max<uint16_t>(64, resolutions[instance->name]) : bakeParams.defaultResolution;

        printf("(%llu/%llu): %s at %dx%d\n", ++i, scene->instances.size(), instance->name.c_str(), resolution, resolution);

        // SGGI Test
        //const auto bitmaps = SGGIBaker::bake(scenePair, *instance, resolution, { { 0.5f * 10000, 0.5f * 10000, 1.0f * 10000 }, 10, 100, 64, 0.01f });
        //BitmapPainter::dilate(*bitmaps.first)->save(instance->name + ".dds", DXGI_FORMAT_R32G32B32A32_FLOAT);
        //BitmapPainter::dilate(*bitmaps.second)->save(instance->name + "_occlusion.dds", DXGI_FORMAT_R32G32B32A32_FLOAT);

        // GI Test (Generations)
        const auto bitmaps = GIBaker::bakeSeparate(raytracingContext, *instance, resolution, bakeParams);

        BitmapHelper::dilate(*bitmaps.first)->save(instance->name + "_lightmap.png");
        BitmapHelper::dilate(*bitmaps.second)->save(instance->name + "_shadowmap.png");

        // GI Test (Lost World)
        //const auto bitmap = BitmapPainter::dilate(*GIBaker::bakeCombined(scenePair, *instance, resolution, { { 88.0f / 255.0f, 148.0f / 255.0f, 214.0f / 255.0f }, 10, 100, 64, 0.01f }));
        //bitma(p->save(instance->name + ".dds", DXGI_FORMAT_BC3_UNORM);
    }

    return 0;
}
