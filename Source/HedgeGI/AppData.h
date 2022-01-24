#pragma once

#include "Component.h"
#include "PropertyBag.h"

class AppData final : public Component
{
    PropertyBag propertyBag;
    std::string directoryPath;
    std::list<std::string> recentStages;

    void loadRecentStages();
    void saveRecentStages() const;

public:
    ~AppData() override;

    PropertyBag& getPropertyBag();
    const std::string& getDirectoryPath() const;

    const std::list<std::string>& getRecentStages() const;
    void addRecentStage(const std::string& path);

    void initialize() override;
};
