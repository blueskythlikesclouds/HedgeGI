#include <xmmintrin.h>
#include <pmmintrin.h>

#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "GIBaker.h"
#include "Scene.h"
#include "SceneFactory.h"
#include "SGGIBaker.h"
#include "LightField.h"
#include <fstream>

#include "LightFieldBaker.h"
#include "SHLightFieldBaker.h"

enum Game
{
    GAME_UNKNOWN,
    GAME_GENERATIONS,
    GAME_LOST_WORLD,
    GAME_FORCES
};

const char* const GAME_NAMES[] =
{
    "Unknown",
    "Sonic Generations",
    "Sonic Lost World",
    "Sonic Forces"
};

int nextPowerOfTwo(int value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

int32_t main(int32_t argc, const char* argv[])
{
    const char* inputDirectoryPath = nullptr;
    bool generateLightField = false;
    bool isPbrMod = false;

    for (size_t i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--lft") == 0)
            generateLightField = true;

        else if (strcmp(argv[i], "--pbr") == 0)
            isPbrMod = true;

        else if (inputDirectoryPath == nullptr)
            inputDirectoryPath = argv[i];
    }


    if (inputDirectoryPath == nullptr)
    {
        printf("Insufficient amount of arguments given.\n");
        getchar();
        return -1;
    }

    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Eigen::initParallel();
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Detect accidental drag and drop of -HedgeGI
    char argv1[1024];
    strcpy(argv1, inputDirectoryPath);

    char* suffix = strstr(argv1, "-HedgeGI");
    if (suffix != nullptr)
        suffix[0] = '\0';

    std::unique_ptr<Scene> scene;

    const std::string path = argv1;
    const std::string outputPath = std::string(argv1) + "-HedgeGI/";

    Game game = GAME_UNKNOWN;

    if (std::filesystem::exists(path + "/Stage.pfd"))
        game = GAME_GENERATIONS;

    else
    {
        const FileStream fileStream((path + "/" + getFileName(path) + "_trr_cmn.pac").c_str(), "rb");
        if (fileStream.isOpen())
        {
            fileStream.seek(4, SEEK_SET);
            const char version = fileStream.read<char>();

            game = version == '3' ? GAME_FORCES : version == '2' ? GAME_LOST_WORLD : GAME_UNKNOWN;
        }
    }

    if (game == GAME_UNKNOWN)
    {
        printf("Failed to detect the game.\n");
        getchar();
        return -1;
    }

    printf("Mode: %s\n", generateLightField ? "Light Field" : "Global Illumination");
    printf("Detected game: %s%s\n", GAME_NAMES[game], isPbrMod ? " (PBR Shaders)" : "");

    if (std::filesystem::exists(path + ".scene"))
    {
        printf("Found scene file, loading...\n");

        scene = std::make_unique<Scene>();
        scene->load(path + ".scene");

        printf("Successfully loaded scene file.\n");
    }
    else
    {
        printf("Creating scene file...\n");

        scene = game == GAME_GENERATIONS ? SceneFactory::createFromGenerations(path) : SceneFactory::createFromLostWorldOrForces(path);
        scene->save(path + ".scene");

        printf("Successfully created scene file.\n");
    }

    std::filesystem::create_directory(outputPath);

    const auto raytracingContext = scene->createRaytracingContext();

    BakeParams bakeParams(game == GAME_FORCES || isPbrMod ? TARGET_ENGINE_HE2 : TARGET_ENGINE_HE1);
    bakeParams.load(getDirectoryPath(argv[0]) + "/HedgeGI.ini");

    // GI Test
    if (!generateLightField)
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
        std::for_each(std::execution::par_unseq, scene->instances.begin(), scene->instances.end(), [&](const std::unique_ptr<const Instance>& instance)
        {
            float radius = 0.0f;

            for (size_t j = 0; j < 8; j++)
                radius = std::max<float>(radius, (instance->aabb.center() - instance->aabb.corner((Eigen::AlignedBox3f::CornerType)j)).norm());

            const uint16_t resolution = std::max<uint16_t>(16, std::min<uint16_t>(2048, 
                resolutions.find(instance->name) != resolutions.end() ? resolutions[instance->name] : 
                nextPowerOfTwo((int)exp2f(bakeParams.resolutionBias + logf(radius) / logf(bakeParams.resolutionBase)))));

            if (game == GAME_FORCES || isPbrMod)
            {
                // SGGI Test
                auto pair = SGGIBaker::bake(raytracingContext, *instance, resolution, bakeParams);

                // Dilate
                pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
                pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

                // Denoise
                pair.lightMap = BitmapHelper::denoise(*pair.lightMap);

                if (bakeParams.denoiseShadowMap)
                    pair.shadowMap = BitmapHelper::denoise(*pair.shadowMap);

                if (bakeParams.optimizeSeams)
                {
                    pair.lightMap = BitmapHelper::optimizeSeams(*pair.lightMap, *instance);
                    pair.shadowMap = BitmapHelper::optimizeSeams(*pair.shadowMap, *instance);
                }

                pair.lightMap->save(outputPath + instance->name + "_sg.dds", isPbrMod ? DXGI_FORMAT_R32G32B32A32_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT);
                pair.shadowMap->save(outputPath + instance->name + "_occlusion.dds", isPbrMod ? DXGI_FORMAT_R32G32B32A32_FLOAT : SGGIBaker::SHADOW_MAP_FORMAT);
            }
            else 
            {
                // GI Test
                auto pair = GIBaker::bake(raytracingContext, *instance, resolution, bakeParams);

                // Dilate
                pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
                pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

                // Combine
                auto combined = BitmapHelper::combine(*pair.lightMap, *pair.shadowMap);

                // Denoise
                combined = BitmapHelper::denoise(*combined, bakeParams.denoiseShadowMap);

                // Optimize seams
                if (bakeParams.optimizeSeams)
                    combined = BitmapHelper::optimizeSeams(*combined, *instance);

                // Make ready for encoding
                if (bakeParams.targetEngine == TARGET_ENGINE_HE1)
                    combined = BitmapHelper::makeEncodeReady(*combined, ENCODE_READY_FLAGS_SQRT);

                if (game == GAME_GENERATIONS && !isPbrMod)
                {
                    combined->save(outputPath + instance->name + "_lightmap.png", Bitmap::transformToLightMap);
                    combined->save(outputPath + instance->name + "_shadowmap.png", Bitmap::transformToShadowMap);
                }
                else if (game == GAME_LOST_WORLD)
                {
                    combined->save(outputPath + instance->name + ".dds", DXGI_FORMAT_BC3_UNORM);
                }
                else if (game == GAME_FORCES || isPbrMod)
                {
                    combined->save(outputPath + instance->name + ".dds", isPbrMod ? DXGI_FORMAT_R32G32B32A32_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT, Bitmap::transformToLightMap);
                    combined->save(outputPath + instance->name + "_occlusion.dds", isPbrMod ? DXGI_FORMAT_R32G32B32A32_FLOAT : SGGIBaker::SHADOW_MAP_FORMAT, Bitmap::transformToShadowMap);
                }
            }
           
            printf("(%llu/%llu): Saved %s (%dx%d)\n", InterlockedIncrement(&i), raytracingContext.scene->instances.size(), instance->name.c_str(), resolution, resolution);
        });
    }
    // Light Field Test
    else
    {
        // SHLF Test
        if (game == GAME_FORCES || isPbrMod)
        {
            size_t i = 0;
            std::for_each(std::execution::par_unseq, scene->shLightFields.begin(), scene->shLightFields.end(), [&](const std::unique_ptr<const SHLightField>& shlf)
            {
                auto bitmap = SHLightFieldBaker::bake(raytracingContext, *shlf, bakeParams);
                BitmapHelper::denoise(*bitmap)->save(outputPath + shlf->name + ".dds", DXGI_FORMAT_R16G16B16A16_FLOAT);

                printf("(%llu/%llu): Saved %s (%dx%dx%d)\n", InterlockedIncrement(&i), 
                    raytracingContext.scene->shLightFields.size(), shlf->name.c_str(), shlf->resolution.x(), shlf->resolution.y(), shlf->resolution.z());
            });
        }
        else
        {
            auto lightField = LightFieldBaker::bake(raytracingContext, bakeParams);

            printf("Saving...\n");

            lightField->save("light-field.lft");
        }
    }

    printf("Completed!\n");
    getchar();

    return 0;
}
