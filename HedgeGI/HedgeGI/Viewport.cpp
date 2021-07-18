#include "Viewport.h"
#include "BakingFactory.h"
#include "Bitmap.h"

void Viewport::keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
    Viewport* viewport = (Viewport*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
        viewport->key[key] = action == GLFW_PRESS;
}

void Viewport::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Viewport* viewport = (Viewport*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
        viewport->mouse[button] = action == GLFW_PRESS;
}

void Viewport::cursorPosCallback(GLFWwindow* window, const double cursorX, const double cursorY)
{
    Viewport* viewport = (Viewport*)glfwGetWindowUserPointer(window);

    if (viewport->mouse[GLFW_MOUSE_BUTTON_LEFT] && !viewport->focused)
    {
        const float pitch = (float)(cursorY - viewport->previousCursorY) * 0.25f * -viewport->elapsedTime;
        const float yaw = (float)(cursorX - viewport->previousCursorX) * 0.25f * -viewport->elapsedTime;

        const Eigen::AngleAxisf x(pitch, viewport->cameraRotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        viewport->cameraRotation = (x * viewport->cameraRotation).normalized();
        viewport->cameraRotation = (y * viewport->cameraRotation).normalized();
    }

    viewport->previousCursorX = cursorX;
    viewport->previousCursorY = cursorY;
}

Viewport::Viewport() : cameraRotation(Quaternion::Identity()),
    previousCursorX(0), previousCursorY(0), previousAverageLuminance(0), time(0), elapsedTime(0),
    dirty(false), focused(false), enableBakeParamsWindow(true)
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    window = glfwCreateWindow(800, 450, "HedgeGI", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

Viewport::~Viewport()
{
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
}

void Viewport::update(const RaytracingContext& raytracingContext, BakeParams& bakeParams)
{
    const double currentTime = glfwGetTime();
    elapsedTime = (float)(currentTime - time);
    time = currentTime;

    glfwPollEvents();

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    const int bitmapWidth = width >> 1;
    const int bitmapHeight = height >> 1;

    if (!bitmap || bitmap->width != bitmapWidth || bitmap->height != bitmapHeight)
    {
        bitmap = std::make_unique<Bitmap>(bitmapWidth, bitmapHeight);
        pixels = std::make_unique<Color4i[]>(bitmap->width * bitmap->height);
    }

    if (!focused)
    {
        if (key['Q'] ^ key['E'])
        {
            const Eigen::AngleAxisf z((key['E'] ? -2.0f : 2.0f) * elapsedTime, cameraRotation * Eigen::Vector3f::UnitZ());
            cameraRotation = (z * cameraRotation).normalized();
        }

        const float speed = key[GLFW_KEY_LEFT_SHIFT] ? 120.0f : 30.0f;

        if (key['W'] ^ key['S'])
            cameraPosition += (cameraRotation * Eigen::Vector3f::UnitZ()).normalized() * (key['W'] ? -speed : speed) * elapsedTime;

        if (key['A'] ^ key['D'])
            cameraPosition += (cameraRotation * Eigen::Vector3f::UnitX()).normalized() * (key['A'] ? -speed : speed) * elapsedTime;

        if (key[GLFW_KEY_LEFT_CONTROL] ^ key[GLFW_KEY_SPACE])
            cameraPosition += Eigen::Vector3f::UnitY() * (key[GLFW_KEY_LEFT_CONTROL] ? -speed : speed) * elapsedTime;
    }

    if (dirty || previousCameraPosition != cameraPosition || previousCameraRotation != cameraRotation)
        bitmap->clear();

    dirty = true;

    glViewport(0, 0, width, height);
    glPixelZoom((float)width / bitmap->width, (float)height / bitmap->height);

    BakingFactory::bake(raytracingContext, *bitmap, cameraPosition, cameraRotation, PI / 4.0f, (float)width / height, bakeParams);

    float averageLuminance = 0.0f;

    for (size_t i = 0; i < bitmap->width; i++)
    {
        for (size_t j = 0; j < bitmap->height; j++)
            averageLuminance += (bitmap->data[i * bitmap->height + j].head<3>() * Color3(0.2126f, 0.7152f, 0.0722f)).sum();
    }

    averageLuminance /= (float)(bitmap->width * bitmap->height);
    averageLuminance *= 2.0f;
    averageLuminance = lerp(averageLuminance, previousAverageLuminance, exp2f(elapsedTime * -2.62317109f));

    for (size_t i = 0; i < bitmap->width; i++)
    {
        for (size_t j = 0; j < bitmap->height; j++)
        {
            const size_t index = i * bitmap->height + j;

            Color4 srcColor = (bitmap->data[index] / averageLuminance).cwiseMin(1.0f).cwiseMax(0.0f);
            if (bakeParams.targetEngine == TARGET_ENGINE_HE2)
                srcColor.head<3>() = srcColor.head<3>().pow(1.0f / 2.2f);

            auto& dstColor = pixels[index];

            dstColor.x() = (uint8_t)(saturate(srcColor.x()) * 255);
            dstColor.y() = (uint8_t)(saturate(srcColor.y()) * 255);
            dstColor.z() = (uint8_t)(saturate(srcColor.z()) * 255);
            dstColor.w() = 255;
        }
    }

    glDrawPixels(bitmap->width, bitmap->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
    glPixelZoom(1, 1);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Display FPS
    {
        ImGuiWindowFlags flags = 0;
        flags |= ImGuiWindowFlags_NoTitleBar;
        flags |= ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoCollapse;
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
        flags |= ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("FPS", nullptr, flags);
        ImGui::Text("FPS: %.2f", 1.0f / elapsedTime);
        ImGui::Text("Time: %.2fms", elapsedTime * 1000);
        ImGui::End();
    }

    // BakeParams
    {
        const BakeParams bakeParamsTmp = bakeParams;

        if (ImGui::Begin("Bake Params", &enableBakeParamsWindow))
        {
            focused = ImGui::IsWindowFocused();

            ImGui::InputFloat3("Environment Color", bakeParams.environmentColor.data());
            ImGui::InputFloat("Diffuse Strength", &bakeParams.diffuseStrength);
            ImGui::InputFloat("Diffuse Saturation", &bakeParams.diffuseSaturation);
            ImGui::InputFloat("Light Strength", &bakeParams.lightStrength);
        }
        ImGui::End();

        dirty = memcmp(&bakeParamsTmp, &bakeParams, sizeof(BakeParams)) != 0;
    }
    

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);

    previousCameraPosition = cameraPosition;
    previousCameraRotation = cameraRotation;
    previousAverageLuminance = averageLuminance;
}

bool Viewport::isOpen() const
{
    return !glfwWindowShouldClose(window);
}
