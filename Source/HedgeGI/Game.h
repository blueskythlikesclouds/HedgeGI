#pragma once

enum class Game
{
    Unknown,
    Unleashed,
    Generations,
    LostWorld,
    Forces
};

extern const char* const GAME_NAMES[];
extern Game detectGameFromStageDirectory(const std::string& directoryPath);