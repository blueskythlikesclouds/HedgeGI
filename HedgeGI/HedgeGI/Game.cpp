#include "Game.h"

const char* const GAME_NAMES[] =
{
    "Unknown",
    "Sonic Generations",
    "Sonic Lost World",
    "Sonic Forces"
};

Game detectGameFromStageDirectory(const std::string& directoryPath)
{
    Game game = GAME_UNKNOWN;

    if (std::filesystem::exists(directoryPath + "/Stage.pfd"))
        game = GAME_GENERATIONS;

    else
    {
        const FileStream fileStream((directoryPath + "/" + getFileName(directoryPath) + "_trr_cmn.pac").c_str(), "rb");
        if (fileStream.isOpen())
        {
            fileStream.seek(4, SEEK_SET);
            const char version = fileStream.read<char>();

            game = version == '3' ? GAME_FORCES : version == '2' ? GAME_LOST_WORLD : GAME_UNKNOWN;
        }
    }

    return game;
}
