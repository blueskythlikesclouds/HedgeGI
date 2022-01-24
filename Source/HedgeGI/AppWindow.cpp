#include "AppWindow.h"

#include "resource.h"
#include "Utilities.h"

void AppWindow::initializeGLFW()
{
    glfwInit();

    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1600, 900, "HedgeGI", nullptr, nullptr);
    glfwMaximizeWindow(window);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSwapInterval(1);
}

void AppWindow::initializeIcons()
{
    const HRSRC hIconRes = FindResource(nullptr, MAKEINTRESOURCE(ResRawData_WindowIcon), TEXT("TEXT"));
    const HGLOBAL hIconGlobal = LoadResource(nullptr, hIconRes);

    DirectX::ScratchImage image;
    DirectX::TexMetadata metadata;

    DirectX::LoadFromWICMemory(LockResource(hIconGlobal), SizeofResource(nullptr, hIconRes), DirectX::WIC_FLAGS_NONE, &metadata, image);

    if (metadata.format != DXGI_FORMAT_R8G8B8A8_UNORM)
    {
        DirectX::ScratchImage tmpImage;
        DirectX::Convert(image.GetImages(), image.GetImageCount(), metadata,
            DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, tmpImage);

        std::swap(image, tmpImage);
        metadata = image.GetMetadata();
    }

    const int resolutions[] = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };

    DirectX::ScratchImage scratchImages[_countof(resolutions)];
    GLFWimage glfwImages[_countof(resolutions)];

    for (size_t i = 0; i < _countof(resolutions); i++)
    {
        DirectX::Resize(image.GetImages(), image.GetImageCount(), metadata,
            resolutions[i], resolutions[i], DirectX::TEX_FILTER_BOX, scratchImages[i]);

        glfwImages[i].width = resolutions[i];
        glfwImages[i].height = resolutions[i];
        glfwImages[i].pixels = scratchImages[i].GetPixels();
    }

    glfwSetWindowIcon(window, _countof(glfwImages), glfwImages);

    FreeResource(hIconGlobal);
}

AppWindow::AppWindow() : window(nullptr), width(1), height(1), focused(false)
{
}

AppWindow::~AppWindow()
{
    glfwTerminate();
}

GLFWwindow* AppWindow::getWindow() const
{
    return window;
}

bool AppWindow::getWindowShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void AppWindow::setWindowShouldClose(const bool value) const
{
    glfwSetWindowShouldClose(window, value);
}

int AppWindow::getWidth() const
{
    return width;
}

int AppWindow::getHeight() const
{
    return height;
}

bool AppWindow::isFocused() const
{
    return focused;
}

void AppWindow::initialize()
{
    initializeGLFW();
    initializeIcons();
}

void AppWindow::update(const float deltaTime)
{
    glfwPollEvents();
    glfwGetWindowSize(window, &width, &height);
    focused = (bool)glfwGetWindowAttrib(window, GLFW_FOCUSED);
}

void AppWindow::alert() const
{
    return ::alert(window);
}
