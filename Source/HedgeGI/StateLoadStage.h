#pragma once

#include "Document.h"
#include "State.h"

class StateLoadStage : public State<Document>
{
    Document document;

    std::string directoryPath;
    std::future<void> future;

public:
    StateLoadStage(const std::string& directoryPath);

    void enter() override;
    void update(float deltaTime) override;
    void leave() override;
};
