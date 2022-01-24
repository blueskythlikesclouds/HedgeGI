#pragma once

#include "Component.h"

enum class LogType;

class LogListener final : public Component
{
    std::list<std::pair<LogType, std::string>> logs;
    size_t previousSize;
    CriticalSection criticalSection;

    static void logListener(void* owner, LogType logType, const char* text);

public:
    LogListener();
    ~LogListener() override;

    void clear();

    void initialize() override;

    bool drawContainerUI(const ImVec2& size);
};
