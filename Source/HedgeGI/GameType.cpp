#include "GameType.h"

#include "FileStream.h"
#include "Utilities.h"

const char* const GAME_NAMES[] =
{
    "Unknown",
    "Sonic Generations",
    "Sonic Lost World",
    "Sonic Forces"
};

GameType detectGameFromStageDirectory(const std::string& directoryPath)
{
    GameType game = GameType::Unknown;

    if (std::filesystem::exists(directoryPath + "/Stage.pfd"))
        game = GameType::Generations;

    else
    {
        const FileStream fileStream((directoryPath + "/" + getFileName(directoryPath) + "_trr_cmn.pac").c_str(), "rb");
        if (fileStream.isOpen())
        {
            fileStream.seek(4, SEEK_SET);
            const char version = fileStream.read<char>();

            game = version == '3' ? GameType::Forces : version == '2' ? GameType::LostWorld : GameType::Unknown;
        }
    }

    return game;
}
