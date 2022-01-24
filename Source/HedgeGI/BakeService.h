#pragma once

#include "Component.h"

class Instance;
class SHLightField;

class BakeService final : public Component
{
    std::atomic<size_t> progress{};
    std::atomic<const Instance*> lastBakedInstance{};
    std::atomic<const SHLightField*> lastBakedShlf{};
    std::atomic<bool> cancel{};

public:
    size_t getProgress() const;
    const Instance* getLastBakedInstance() const;
    const SHLightField* getLastBakedShlf() const;

    bool isPendingCancel() const;
    void requestCancel();

    void bake();
    void bakeGI();
    void bakeLightField();
};
