#pragma once

#include "ModelProcessor.h"
#include "StateProcess.h"

class StateProcessStage : public StateProcess
{
    std::string directoryPath;

public:
    StateProcessStage(const std::string& directoryPath, ProcModelFunc function, bool clearLogs = true);

    void leave() override;
};
