#include "Application.h"
#include "Input.h"
#include "Viewport.h"
#include "Camera.h"
#include "Framebuffer.h"

GLFWwindow* Application::createGLFWwindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow * window = glfwCreateWindow(800, 450, "HedgeGI", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    return window;
}

void Application::drawFrameRateWindow() const
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

Application::Application(const RaytracingContext& raytracingContext, const BakeParams& bakeParams)
    : window(createGLFWwindow()), input(window), raytracingContext(raytracingContext), bakeParams(bakeParams)
{
    // Initialize ImGUI
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }
}

Application::~Application()
{
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
}

const Input& Application::getInput() const
{
    return input;
}

const Camera& Application::getCamera() const
{
    return camera;
}

float Application::getElapsedTime() const
{
    return elapsedTime;
}

int Application::getWidth() const
{
    return width;
}

int Application::getHeight() const
{
    return height;
}

const RaytracingContext& Application::getRaytracingContext() const
{
    return raytracingContext;
}

const BakeParams& Application::getBakeParams() const
{
    return bakeParams;
}

void Application::drawQuad() const
{
    quad.vertexArray.bind();
    quad.elementArray.bind();
    quad.elementArray.draw();
}

void Application::run()
{
    while (!glfwWindowShouldClose(window))
        update();
}

void Application::update()
{
    glfwPollEvents();

    const double glfwTime = glfwGetTime();
    elapsedTime = (float)(glfwTime - currentTime);
    currentTime = glfwTime;

    glfwGetWindowSize(window, &width, &height);

    camera.update(*this);

    // Viewport
    {
        viewport.update(*this);

        const FramebufferTexture& framebufferTex = viewport.getFramebufferTexture();

        glViewport(0, 0, framebufferTex.width, framebufferTex.height);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferTex.id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
        glBlitFramebuffer(0, 0, framebufferTex.width, framebufferTex.height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    }

    // GUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        drawFrameRateWindow();
    }
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    input.postUpdate();

    glfwSwapBuffers(window);
}
