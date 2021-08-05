#include "Logger.h"

std::list<Logger::Listener> Logger::listeners;

void Logger::addListener(void* owner, void(*function)(void*, LogType, const char*))
{
    listeners.push_back({ owner, function });
}

void Logger::removeListener(void* owner)
{
    for (auto it = listeners.begin(); it != listeners.end(); ++it)
    {
        if ((*it).owner != owner)
            continue;

        listeners.erase(it);
        return;
    }
}

void Logger::log(const LogType logType, const char* text)
{
    for (auto& listener : listeners)
        listener.function(listener.owner, logType, text);
}

void Logger::logFormatted(const LogType logType, const char* format, ...)
{
    char text[1024];

    va_list args;
    va_start(args, format);
    vsprintf(text, format, args);
    va_end(args);

    log(logType, text);
}
