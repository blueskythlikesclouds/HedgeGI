#include "Viewport.h"
#include "BakingFactory.h"
#include "Bitmap.h"
#include "Input.h"
#include "Camera.h"

Viewport::Viewport() : previousAverageLuminance(0), time(0), elapsedTime(0),
   focused(false), enableBakeParamsWindow(true), texture(0), framebuffer(0)
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    window = glfwCreateWindow(800, 450, "HedgeGI", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    input = std::make_unique<Input>(window);
    camera = std::make_unique<Camera>();

    glGenFramebuffers(1, &framebuffer);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

Viewport::~Viewport()
{
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &framebuffer);

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

    const int bitmapWidth = width >> 2;
    const int bitmapHeight = height >> 2;

    if (!bitmap || bitmap->width != bitmapWidth || bitmap->height != bitmapHeight)
    {
        bitmap = std::make_unique<Bitmap>(bitmapWidth, bitmapHeight);
        pixels = std::make_unique<Color4i[]>(bitmap->width * bitmap->height);

        glDeleteTextures(1, &texture);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    }

    if (!focused)
        camera->update(*input, elapsedTime);

    camera->aspectRatio = (float)width / (float)height;
    camera->fieldOfView = PI / 4.0f;

    if (camera->hasChanged())
        bitmap->clear();

    BakingFactory::bake(raytracingContext, *bitmap, *camera, bakeParams);

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

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bitmap->width, bitmap->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, bitmap->width, bitmap->height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

        if (input->tappedKeys[GLFW_KEY_ESCAPE])
            enableBakeParamsWindow ^= true;

        if (enableBakeParamsWindow && ImGui::Begin("Bake Params", &enableBakeParamsWindow))
        {
            focused = ImGui::IsWindowFocused();

            ImGui::InputFloat3("Environment Color", bakeParams.environmentColor.data());
            ImGui::InputFloat("Diffuse Strength", &bakeParams.diffuseStrength);
            ImGui::InputFloat("Diffuse Saturation", &bakeParams.diffuseSaturation);
            ImGui::InputFloat("Light Strength", &bakeParams.lightStrength);
        }
        else focused = false;

        ImGui::End();

        if (memcmp(&bakeParamsTmp, &bakeParams, sizeof(BakeParams)) != 0)
            bitmap->clear();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);

    previousAverageLuminance = averageLuminance;
    input->postUpdate();
}

bool Viewport::isOpen() const
{
    return !glfwWindowShouldClose(window);
}
