#pragma once

#include "Document.h"
#include "State.h"

class StateEditStage : public State<Document>
{
    Document document;

public:
    StateEditStage(Document&& document);

    void enter() override;
    void update(float deltaTime) override;
    void leave() override;
};
