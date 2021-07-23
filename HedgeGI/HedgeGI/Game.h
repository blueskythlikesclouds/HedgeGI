#pragma once

enum Game
{
    GAME_UNKNOWN,
    GAME_GENERATIONS,
    GAME_LOST_WORLD,
    GAME_FORCES
};

extern const char* const GAME_NAMES[];
extern Game detectGameFromStageDirectory(const std::string& directoryPath);