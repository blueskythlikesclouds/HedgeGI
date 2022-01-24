#pragma once

#include "State.h"

class Document;

class StateProcess : public State<Document>
{
    std::function<void()> function;
    std::future<void> future;
    bool clearLogs;

public:
    StateProcess(std::function<void()> function, bool clearLogs = true);

    void enter() override;
    void update(float deltaTime) override;
    void leave() override;
};
