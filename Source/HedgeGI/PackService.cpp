#include "PackService.h"

#include "Logger.h"
#include "Stage.h"
#include "StageParams.h"
#include "AppWindow.h"
#include "SHLightField.h"
#include "Light.h"
#include "PostRender.h"
#include "Instance.h"

void PackService::pack()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    if (params->mode == BakingFactoryMode::LightField)
        packResources(PackResourceMode::LightField);

    else if (params->mode == BakingFactoryMode::GI && 
        params->validateOutputDirectoryPath(false))
    {
        switch (stage->getGameType())
        {
        case GameType::Generations:
            packGenerationsGI();
            break;

        case GameType::LostWorld:
        case GameType::Forces:
            packLostWorldOrForcesGI();
            break;

        default: break;
        }
    }
}

void PackService::packResources(PackResourceMode mode)
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    if (!params->validateOutputDirectoryPath(false))
        return;

    std::string archiveFileName;

    if (stage->getGameType() == GameType::Generations)
        archiveFileName = stage->getName() + ".ar.00";

    else if (stage->getGameType() == GameType::LostWorld || stage->getGameType() == GameType::Forces)
        archiveFileName = stage->getName() + "_trr_cmn.pac";

    else return;

    auto nArchiveFilePath = toNchar((stage->getDirectoryPath() + "/" + archiveFileName).c_str());

    hl::archive archive = stage->getGameType() == GameType::Generations ? hl::hh::ar::load(nArchiveFilePath.data()) : hl::pacx::load(nArchiveFilePath.data());

    if (mode == PackResourceMode::LightField)
    {
        if (params->bakeParams.targetEngine == TargetEngine::HE1)
        {
            const std::string lightFieldFilePath = params->outputDirectoryPath + "/light-field.lft";
            if (!std::filesystem::exists(lightFieldFilePath))
            {
                Logger::log(LogType::Error, "Failed to find light-field.lft");
                return;
            }

            Logger::log(LogType::Normal, "Packed light-field.lft");

            addOrReplace(archive, HL_NTEXT("light-field.lft"), hl::blob(toNchar(lightFieldFilePath.c_str()).data()));
        }

        else if (params->bakeParams.targetEngine == TargetEngine::HE2)
        {
            // Save and pack SHLF
            const std::string shlfFileName = stage->getName() + ".shlf";
            Logger::logFormatted(LogType::Normal, "Packing %s", shlfFileName.c_str());

            const std::string shlfFilePath = params->outputDirectoryPath + "/" + shlfFileName;
            SHLightField::save(shlfFilePath, stage->getScene()->shLightFields);
            addOrReplace(archive, toNchar(shlfFileName.c_str()).data(), hl::blob(toNchar(shlfFilePath.c_str()).data()));

            // Pack light field textures
            bool any = false;

            for (auto& shLightField : stage->getScene()->shLightFields)
            {
                const std::string textureFileName = shLightField->name + ".dds";
                const std::string textureFilePath = params->outputDirectoryPath + "/" + textureFileName;
                if (!std::filesystem::exists(textureFilePath))
                    continue;

                Logger::logFormatted(LogType::Normal, "Packing %s", textureFileName.c_str());

                addOrReplace(archive, toNchar(textureFileName.c_str()).data(), hl::blob(toNchar(textureFilePath.c_str()).data()));
                any = true;
            }

            if (!any)
                Logger::log(LogType::Warning, "Failed to find light field textures");
        }

        else return;
    }

    else if (mode == PackResourceMode::Light)
    {
        // Clean any lights from the archive
        for (auto it = archive.begin(); it != archive.end();)
        {
            if (hl::text::strstr((*it).name(), HL_NTEXT(".light")))
                it = archive.erase(it);

            else
                ++it;
        }

        if (stage->getGameType() == GameType::Generations)
        {
            Logger::log(LogType::Normal, "Packing light-list.light-list");

            hl::mem_stream stream;
            Light::saveLightList(stream, stage->getScene()->lights);

            addOrReplace(archive, HL_NTEXT("light-list.light-list"), stream.get_size(), stream.get_data_ptr());
        }

        for (auto& light : stage->getScene()->lights)
        {
            Logger::logFormatted(LogType::Normal, "Packing %s.light", light->name.c_str());

            hl::mem_stream stream;
            light->save(stream);

            addOrReplace(archive, toNchar((light->name + ".light").c_str()).data(), stream.get_size(), stream.get_data_ptr());
        }
    }

    else return;

    switch (stage->getGameType())
    {
    case GameType::Generations:
        hl::hh::ar::save(archive, nArchiveFilePath.data());
        break;

    case GameType::LostWorld:
        hl::pacx::v2::save(archive, hl::bina::endian_flag::little, hl::pacx::lw_exts, hl::pacx::lw_ext_count, nArchiveFilePath.data());
        break;

    case GameType::Forces:
        hl::pacx::v3::save(archive, hl::bina::endian_flag::little, hl::pacx::forces_exts, hl::pacx::forces_ext_count, nArchiveFilePath.data());
        break;

    default: return;
    }

    Logger::logFormatted(LogType::Normal, "Saved %s", archiveFileName.c_str());
}

void PackService::packGenerationsGI()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    PostRender::process(stage->getDirectoryPath(), params->outputDirectoryPath, params->bakeParams.targetEngine);
}

void PackService::packLostWorldOrForcesGI()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    static const char* lwSuffixes[] = { ".dds" };
    static const char* forcesSuffixes[] = { ".dds", "_sg.dds", "_occlusion.dds" };

    const char** suffixes;
    size_t suffixCount;

    if (stage->getGameType() == GameType::LostWorld)
    {
        suffixes = lwSuffixes;
        suffixCount = _countof(lwSuffixes);
    }
    else if (stage->getGameType() == GameType::Forces)
    {
        suffixes = forcesSuffixes;
        suffixCount = _countof(forcesSuffixes);
    }
    else return;

    auto processArchive = [&](const std::string& archiveFileName)
    {
        const std::string archiveFilePath = stage->getDirectoryPath() + "/" + archiveFileName;
        if (!std::filesystem::exists(archiveFilePath))
            return;

        auto nArchiveFilePath = toNchar(archiveFilePath.c_str());

        hl::archive archive = hl::pacx::load(nArchiveFilePath.data());

        bool any = false;

        for (auto& instance : stage->getScene()->instances)
        {
            for (size_t i = 0; i < suffixCount; i++)
            {
                // If the texture exists, clean it from the archive, 
                // then add it to the archive if the corresponding terrain model/instance info exists.
                const std::string textureFileName = instance->name + suffixes[i];
                const std::string textureFilePath = params->outputDirectoryPath + "/" + textureFileName;

                if (!std::filesystem::exists(textureFilePath))
                    continue;

                auto nTextureFileName = toNchar(textureFileName.c_str());
                auto nTextureFilePath = toNchar(textureFilePath.c_str());

                for (size_t j = 0; j < archive.size();)
                {
                    const hl::archive_entry& entry = archive[j];
                    if (!hl::text::iequal(entry.name(), nTextureFileName.data()))
                    {
                        j++;
                        continue;
                    }

                    archive.erase(archive.begin() + j);
                    any = true;
                }

                auto instanceFileName = toNchar((instance->name + ".terrain-instanceinfo").c_str());
                auto modelFileName = toNchar((instance->name + ".terrain-model").c_str());

                for (size_t j = 0; j < archive.size(); j++)
                {
                    const hl::archive_entry& entry = archive[j];
                    if (!hl::text::iequal(entry.name(), instanceFileName.data()) &&
                        !hl::text::iequal(entry.name(), modelFileName.data()))
                        continue;

                    Logger::logFormatted(LogType::Normal, "Packing %s", textureFileName.c_str());

                    hl::blob blob(nTextureFilePath.data());
                    archive.push_back(std::move(hl::archive_entry::make_regular_file(nTextureFileName.data(), blob.size(), blob.data())));

                    any = true;
                    break;
                }
            }
        }

        if (!any) return;

        if (stage->getGameType() == GameType::LostWorld)
            hl::pacx::v2::save(archive, hl::bina::endian_flag::little, hl::pacx::lw_exts, hl::pacx::lw_ext_count, nArchiveFilePath.data());

        else if (stage->getGameType() == GameType::Forces)
            hl::pacx::v3::save(archive, hl::bina::endian_flag::little, hl::pacx::forces_exts, hl::pacx::forces_ext_count, nArchiveFilePath.data());

        Logger::logFormatted(LogType::Normal, "Saved %s", archiveFileName.c_str());
    };

    processArchive(stage->getName() + "_trr_cmn.pac");

    for (size_t i = 0; i < 100; i++)
    {
        char sector[4];
        sprintf(sector, "%02d", (int)i);

        processArchive(stage->getName() + "_trr_s" + sector + ".pac");
    }
}
