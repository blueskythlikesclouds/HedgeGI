#include "ModelProcessor.h"

#include "FileStream.h"
#include "Logger.h"
#include "Utilities.h"

bool ModelProcessor::processArchive(hl::archive& archive, ProcModelFunc function)
{
    bool any = false;

    for (size_t i = 0; i < archive.size(); i++)
    {
        hl::archive_entry& entry = archive[i];
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".terrain-model")))
            continue;

        Logger::logFormatted(LogType::Normal, "Processing %s...", toUtf8(entry.name()).data());

        hl::hh::mirage::terrain_model::fix(entry.file_data());
        hl::hh::mirage::terrain_model model(entry.file_data());

        function(model);

        model.save(HL_NTEXT("temp.bin"));
        hl::blob blob(HL_NTEXT("temp.bin"));

        entry = hl::archive_entry::make_regular_file(entry.name(), blob.size(), blob.data());
        any = true;
    }

    return any;
}

void ModelProcessor::processArchive(const std::string& filePath, ProcModelFunc function, const std::string& displayName)
{
    if (!std::filesystem::exists(filePath))
        return;

    const std::string fileName = displayName.empty() ? getFileName(filePath) : displayName;
    Logger::logFormatted(LogType::Normal, "Loading %s...", fileName.c_str());

    auto nArchiveFilePath = toNchar(filePath.c_str());

    if (hl::text::strstr(nArchiveFilePath.data(), HL_NTEXT(".pac")))
    {
        hl::bina::ver version{0, 0, 0};
        hl::bina::endian_flag endianFlag;

        {
            const FileStream fileStream(filePath.c_str(), "rb");
            if (!fileStream.isOpen())
                return;

            if (fileStream.read<hl::u32>() != hl::pacx::sig)
                return;

            fileStream.read(&version, 1);
            fileStream.read(&endianFlag, 1);
        }

        hl::archive archive = hl::pacx::load(nArchiveFilePath.data());
        if (!processArchive(archive, function))
            return;

        switch (version.major)
        {
        case '2': 
            hl::pacx::v2::save(archive, endianFlag, hl::pacx::lw_exts, hl::pacx::lw_ext_count, nArchiveFilePath.data());
            break;

        case '3':
            hl::pacx::v3::save(archive, endianFlag, hl::pacx::forces_exts, hl::pacx::forces_ext_count, nArchiveFilePath.data());
            break;

        default:
            return;
        }
    }
    else
    {
        hl::archive archive = hl::hh::ar::load(nArchiveFilePath.data());
        if (!processArchive(archive, function))
            return;

        hl::hh::ar::save(archive, nArchiveFilePath.data(), 0, 0x10, hl::compress_type::none, false);
    }

    Logger::logFormatted(LogType::Normal, "Saved %s", fileName.c_str());
}

void ModelProcessor::processGenerationsStage(const std::string& directoryPath, ProcModelFunc function)
{
    hl::packed_file_info pfi;
    {
        const std::string pfdFilePath = directoryPath + "/Stage.pfd";

        auto nPfdFilePath = toNchar(pfdFilePath.c_str());

        hl::archive pfd = hl::hh::pfd::load(nPfdFilePath.data());
        for (size_t i = 0; i < pfd.size(); i++)
        {
            hl::archive_entry& entry = pfd[i];
            if (!hl::text::strstr(entry.name(), HL_NTEXT("tg-")))
                continue;

            {
                const FileStream stream("temp.cab", "wb");
                stream.write(entry.file_data<hl::u8>(), entry.size());
            }

            TCHAR args1[] = TEXT("expand temp.cab temp.ar");
            if (!executeCommand(args1))
                continue;

            const std::string displayName = hl::text::conv<hl::text::native_to_utf8>(entry.name());
            processArchive("temp.ar", function, displayName);

            Logger::logFormatted(LogType::Normal, "Compressing %s...", displayName.c_str());

            TCHAR args2[] = TEXT("makecab temp.ar temp.ar");
            executeCommand(args2);

            hl::blob blob(HL_NTEXT("temp.ar"));
            entry = hl::archive_entry::make_regular_file(entry.name(), blob.size(), blob.data());
        }

        hl::hh::pfd::save(pfd, nPfdFilePath.data(), hl::hh::pfd::default_alignment, &pfi);
    }

    {
        const std::string resourcesFilePath = directoryPath + "/" + getFileName(directoryPath) + ".ar.00";
        if (!std::filesystem::exists(resourcesFilePath))
            return;

        auto nResourcesFilePath = toNchar(resourcesFilePath.c_str());

        hl::archive archive = hl::hh::ar::load(nResourcesFilePath.data());
        for (size_t i = 0; i < archive.size(); i++)
        {
            hl::archive_entry& entry = archive[i];
            if (!hl::text::equal(entry.name(), HL_NTEXT("Stage.pfi")))
                continue;

            hl::hh::pfi::save(pfi, 0, HL_NTEXT("temp.bin"));

            hl::blob blob(HL_NTEXT("temp.bin"));
            entry = hl::archive_entry::make_regular_file(entry.name(), blob.size(), blob.data());
            break;
        }

        hl::hh::ar::save(archive, nResourcesFilePath.data());
    }
}

void ModelProcessor::processLostWorldOrForcesStage(const std::string& directoryPath, ProcModelFunc function)
{
    const std::string baseFilePath = directoryPath + "/" + getFileNameWithoutExtension(directoryPath);

    processArchive(baseFilePath + "_trr_cmn.pac", function);
    processArchive(baseFilePath + "_far.pac", function);

    for (uint32_t i = 0; i < 100; i++)
    {
        char slot[4];
        sprintf(slot, "%02d", i);

        processArchive(baseFilePath + "_trr_s" + slot + ".pac", function);
    }
}

void ModelProcessor::processStage(const std::string& directoryPath, ProcModelFunc function)
{
    if (std::filesystem::exists(directoryPath + "/Stage.pfd"))
        processGenerationsStage(directoryPath, function);
    else
        processLostWorldOrForcesStage(directoryPath, function);
}
