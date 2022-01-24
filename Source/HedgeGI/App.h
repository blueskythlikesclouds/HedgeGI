#pragma once

#include "Document.h"

class App
{
    Document document;
    double currentTime;

public:
    App();

    void update();
    void run();
};
