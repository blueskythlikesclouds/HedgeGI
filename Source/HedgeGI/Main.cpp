#include "App.h"

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <xmmintrin.h>
#include <pmmintrin.h>

int32_t main(int32_t argc, const char* argv[])
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Eigen::initParallel();

    DirectX::Initialize();
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    {
        App app;
        app.run();
    }

    // Calling exit forces any async tasks to quit
    exit(0);
}
