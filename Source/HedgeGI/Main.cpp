#include "Application.h"

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <xmmintrin.h>
#include <pmmintrin.h>

int32_t main(int32_t argc, const char* argv[])
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Application application;
    if (argc > 1)
        application.loadScene(argv[1]);

    application.run();
    return 0;
}