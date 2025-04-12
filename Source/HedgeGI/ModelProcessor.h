﻿#pragma once

typedef std::function<void(hl::hh::mirage::model&)> ProcModelFunc;

class ModelProcessor
{
public:
    static bool processArchive(hl::archive& archive, ProcModelFunc function);
    static void processArchive(const std::string& filePath, ProcModelFunc function, const std::string& displayName = std::string());

    static void processGenerationsOrUnleashedStage(const std::string& directoryPath, ProcModelFunc function);
    static void processLostWorldOrForcesStage(const std::string& directoryPath, ProcModelFunc function);

    static void processStage(const std::string& directoryPath, ProcModelFunc function);
};
