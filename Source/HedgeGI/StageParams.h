#pragma once

#include "BakeParams.h"
#include "Component.h"
#include "PropertyBag.h"

enum class BakingFactoryMode
{
    GI,
    LightField
};

class StageParams final : public Component
{
public:
    float viewportResolutionInvRatio{};
    bool gammaCorrectionFlag{};
    bool colorCorrectionFlag{};
    BakeParams bakeParams;

    BakingFactoryMode mode{};
    std::string outputDirectoryPath;

    bool skipExistingFiles{ true };

    PropertyBag propertyBag;

    bool dirty{ false };
    bool dirtyBVH{ false };

    void loadProperties();
    void storeProperties();

    bool validateOutputDirectoryPath(bool create) const;
};
