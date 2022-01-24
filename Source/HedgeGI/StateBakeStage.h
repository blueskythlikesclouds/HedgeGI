#pragma once

#include "State.h"

class Document;

class StateBakeStage : public State<Document>
{
    bool packAfterFinish;
    std::future<void> future;

public:
    StateBakeStage(bool packAfterFinish);

    void enter() override;
    void update(float deltaTime) override;
    void leave() override;
};
