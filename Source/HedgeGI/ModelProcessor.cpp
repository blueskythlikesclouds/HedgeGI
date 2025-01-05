#include "ModelProcessor.h"

#include "CabinetCompression.h"
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

        hl::mem_stream stream;

        // Try to detect .model files renamed to .terrain-model.
        bool mightBeModel = false;
        uint8_t* data = entry.file_data<uint8_t>();
        if ((HL_SWAP_U32(*data) & 0x80000000) != 0)
        {
            data += 0x10;

            while (memcmp(data + 0x8, "Contexts", 0x8) != 0)
                data += 0x10;

            data += 0x10;

            uint32_t offset = HL_SWAP_U32(*reinterpret_cast<uint32_t*>(data + 0x4)) + 0x10;
            mightBeModel = (offset - (data - entry.file_data<uint8_t>())) > 0x10;
        }
        else
        {
            mightBeModel = HL_SWAP_U32(*reinterpret_cast<uint32_t*>(data + 0x1C)) > 0x10;
        }

        if (mightBeModel)
        {
            Logger::logFormatted(LogType::Warning, "Detected \"%s\" to be renamed from a .model file", toUtf8(entry.name()).data());

            hl::hh::mirage::skeletal_model::fix(entry.file_data());
            hl::hh::mirage::skeletal_model model(entry.file_data(), "");

            function(model);

            model.save(stream);
        }
        else
        {
            hl::hh::mirage::terrain_model::fix(entry.file_data());
            hl::hh::mirage::terrain_model model(entry.file_data(), "");

            function(model);

            model.save(stream);
        }


        entry = hl::archive_entry::make_regular_file(entry.name(), stream.get_size(), stream.get_data_ptr());
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

        switch (version._major)
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
        hl::archive archive = loadArchive(nArchiveFilePath.data());
        if (!processArchive(archive, function))
            return;

        hl::hh::ar::save(archive, getFullPath(nArchiveFilePath).data(), 0, 0x10, hl::compress_type::none, false);
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

            auto name = toUtf8(entry.name());

            Logger::logFormatted(LogType::Normal, "Loading %s...", name.data());
            
            hl::archive archive = CabinetCompression::load(entry.file_data(), entry.size());
            processArchive(archive, function);

            Logger::logFormatted(LogType::Normal, "Compressing %s...", name.data());

            hl::mem_stream stream;
            CabinetCompression::save(archive, stream, name.data());

            entry = hl::archive_entry::make_regular_file(entry.name(), stream.get_size(), stream.get_data_ptr());
        }

        hl::hh::pfd::save(pfd, nPfdFilePath.data(), hl::hh::pfd::default_alignment, &pfi);
    }

    {
        const std::string resourcesFilePath = directoryPath + "/" + getFileName(directoryPath) + ".ar.00";
        if (!std::filesystem::exists(resourcesFilePath))
            return;

        auto nResourcesFilePath = toNchar(resourcesFilePath.c_str());

        hl::archive archive = loadArchive(nResourcesFilePath.data());
        for (size_t i = 0; i < archive.size(); i++)
        {
            hl::archive_entry& entry = archive[i];
            if (!hl::text::equal(entry.name(), HL_NTEXT("Stage.pfi")))
                continue;

            hl::mem_stream stream;
            savePfi(pfi, stream);

            entry = hl::archive_entry::make_regular_file(entry.name(), stream.get_size(), stream.get_data_ptr());
            break;
        }

        hl::hh::ar::save(archive, getFullPath(nResourcesFilePath).data());
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
