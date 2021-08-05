#pragma once

enum class LogType
{
    Success,
    Normal,
    Warning,
    Error
};

class Logger
{
    struct Listener
    {
        void* owner;
        void(*function)(void*, LogType, const char*);
    };

    static std::list<Listener> listeners;

public:
    static void addListener(void* owner, void(*function)(void*, LogType, const char*));
    static void removeListener(void* owner);

    static void log(LogType logType, const char* text);
    static void logFormatted(LogType logType, const char* format, ...);
};
