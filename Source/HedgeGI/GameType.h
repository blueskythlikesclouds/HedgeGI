#pragma once

enum class GameType
{
    Unknown,
    Generations,
    LostWorld,
    Forces
};

extern const char* const GAME_NAMES[];
extern GameType detectGameFromStageDirectory(const std::string& directoryPath);