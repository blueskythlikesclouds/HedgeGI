#include "LogListener.h"

#include "Logger.h"

void LogListener::logListener(void* owner, LogType logType, const char* text)
{
    LogListener* logs = (LogListener*)owner;
    std::lock_guard lock(logs->criticalSection);

    const auto value = std::make_pair(logType, text);
    logs->logs.remove(value);
    logs->logs.push_back(value);
}

LogListener::LogListener() : previousSize(0)
{
}

LogListener::~LogListener()
{
    Logger::removeListener(this);
}

void LogListener::clear()
{
    std::lock_guard lock(criticalSection);
    logs.clear();
}

void LogListener::initialize()
{
    Logger::addListener(this, logListener);
}

bool LogListener::drawContainerUI(const ImVec2& size)
{
    std::lock_guard lock(criticalSection);
    if (logs.empty() || !ImGui::BeginListBox("##Logs", size))
        return false;

    ImGui::PushTextWrapPos();

    for (auto& log : logs)
    {
        const ImVec4 colors[] =
        {
            ImVec4(0.13f, 0.7f, 0.3f, 1.0f), // Success
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // Normal
            ImVec4(1.0f, 0.78f, 0.054f, 1.0f), // Warning
            ImVec4(1.0f, 0.02f, 0.02f, 1.0f) // Error
        };

        ImGui::TextColored(colors[(size_t)log.first], log.second.c_str());
    }

    ImGui::PopTextWrapPos();

    if (previousSize != logs.size())
        ImGui::SetScrollHereY();

    previousSize = logs.size();

    ImGui::EndListBox();
    return true;
}
