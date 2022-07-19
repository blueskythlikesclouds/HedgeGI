#pragma once

#include "Component.h"

enum class PackResourceMode
{
    Light,
    LightField,
    MetaInstancer
};

class PackService final : public Component
{
public:
    void pack();

    void packResources(PackResourceMode mode);
    void packGenerationsGI();
    void packLostWorldOrForcesGI();
};
