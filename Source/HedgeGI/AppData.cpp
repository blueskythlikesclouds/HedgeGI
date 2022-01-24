#include "AppData.h"

#include <shlobj_core.h>

void AppData::loadRecentStages()
{
    std::ifstream file(directoryPath + "recent_stages.txt", std::ios::in);
    if (!file.is_open())
        return;

    std::string path;
    while (std::getline(file, path))
    {
        if (std::filesystem::exists(path))
            recentStages.push_back(path);
    }
}

void AppData::saveRecentStages() const
{
    std::ofstream file(directoryPath + "recent_stages.txt", std::ios::out);
    if (!file.is_open())
        return;

    for (auto& path : recentStages)
        file << path << std::endl;
}

AppData::~AppData()
{
    propertyBag.save(directoryPath + "settings.hgi");
    saveRecentStages();
}

PropertyBag& AppData::getPropertyBag()
{
    return propertyBag;
}

const std::string& AppData::getDirectoryPath() const
{
    return directoryPath;
}

const std::list<std::string>& AppData::getRecentStages() const
{
    return recentStages;
}

void AppData::addRecentStage(const std::string& path)
{
    recentStages.remove(path);

    if (recentStages.size() >= 10)
        recentStages.pop_back();

    recentStages.push_front(path);
}

void AppData::initialize()
{
    PWSTR path = nullptr;

    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &path) == S_OK)
    {
        WCHAR subPath[0x400];
        wcscpy(subPath, path);
        wcscat(subPath, L"/HedgeGI/");

        CoTaskMemFree(path);

        CreateDirectoryW(subPath, NULL);

        directoryPath = wideCharToMultiByte(subPath);
    }
    else
    {
        directoryPath = getExecutableDirectoryPath();
    }

    propertyBag.load(directoryPath + "settings.hgi");
    loadRecentStages();
}