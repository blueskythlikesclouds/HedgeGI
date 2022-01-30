#include "Stage.h"
#include "AppData.h"
#include "StageParams.h"
#include "Logger.h"
#include "SceneFactory.h"

Stage::~Stage()
{
    destroyStage();
}

const std::string& Stage::getName() const
{
    return name;
}

const std::string& Stage::getDirectoryPath() const
{
    return directoryPath;
}

GameType Stage::getGameType() const
{
    return gameType;
}

Scene* Stage::getScene() const
{
    return scene.get();
}

void Stage::loadStage(const std::string& directoryPath)
{
    name = getFileNameWithoutExtension(directoryPath);
    this->directoryPath = directoryPath;
    get<AppData>()->addRecentStage(directoryPath);
    gameType = detectGameFromStageDirectory(directoryPath);

    const auto params = get<StageParams>();
    params->propertyBag.load(directoryPath + "/" + name + ".hgi");
    params->loadProperties();

    scene = SceneFactory::create(directoryPath);
    params->bakeParams.skyIntensityScale = scene->effect.def.skyIntensityScale;
}

void Stage::destroyStage()
{
    scene = nullptr;

    if (!directoryPath.empty() && !name.empty())
    {
        const auto params = get<StageParams>();
        params->storeProperties();
        params->propertyBag.save(directoryPath + "/" + name + ".hgi");
    }

    directoryPath.clear();
    name.clear();
}

void Stage::clean()
{
    const auto params = get<StageParams>();
    if (!params->validateOutputDirectoryPath(false))
        return;

    for (auto& file : std::filesystem::directory_iterator(params->outputDirectoryPath))
    {
        if (!file.is_regular_file())
            continue;

        std::string extension = file.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

        if (extension != ".png" && extension != ".dds" && extension != ".lft" && extension != ".shlf")
            continue;

        std::filesystem::remove(file);
        Logger::logFormatted(LogType::Normal, "Deleted %s", toUtf8(file.path().filename().c_str()));
    }
}
