#include "BakingFactory.h"
#include "BitmapPainter.h"
#include "GIBaker.h"
#include "Scene.h"
#include "SceneFactory.h"
#include "SGGIBaker.h"

int32_t main(int32_t argc, const char* argv[])
{
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

    //const auto scene = SceneFactory::createFromGenerations("D:\\Steam\\steamapps\\common\\Sonic Generations\\mods\\Water Palace\\disk\\bb\\Packed\\ghz200");
    const auto scenePair = scene->createRaytracingContext();

    for (auto& instance : scene->instances)
    {
        printf("Baking for %s...\n", instance->name.c_str());

        //const auto bitmaps = SGGIBaker::bake(scenePair, *instance, 512, { { 0.5f, 0.5f, 1.0f }, 10, 128, 64, 0.01f });
        //bitmaps.first->save(instance->name + "_sg.dds", DXGI_FORMAT_BC1_UNORM);
        //bitmaps.second->save(instance->name + "_occlusion.dds", DXGI_FORMAT_BC1_UNORM);

        const auto bitmaps = GIBaker::bake(scenePair, *instance, 4048, { { 0.5f, 0.5f, 1.0f }, 10, 128, 64, 0.01f });
        BitmapPainter::dilate(*bitmaps.first)->save(instance->name + "_lightmap.png");
        BitmapPainter::dilate(*bitmaps.second)->save(instance->name + "_shadowmap.png");
    }

    return 0;
}
