#pragma once

class Logger
{
    struct Listener
    {
        void* owner;
        void(*function)(void*, const char*);
    };

    static std::list<Listener> listeners;

public:
    static void addListener(void* owner, void(*function)(void*, const char*));
    static void removeListener(void* owner);

    static void log(const char* text);
    static void logFormatted(const char* format, ...);
};
