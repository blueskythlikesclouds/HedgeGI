#pragma once

#include "Document.h"
#include "State.h"

class StateEditMaterial : public State<Document>
{
    Document document;

public:
    void enter() override;
    void update(float deltaTime) override;
};
