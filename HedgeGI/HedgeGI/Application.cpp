#include "Application.h"
#include "Input.h"
#include "Viewport.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "FileDialog.h"
#include "SceneFactory.h"

const ImGuiWindowFlags ImGuiWindowFlags_Static = 
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings;

GLFWwindow* Application::createGLFWwindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow * window = glfwCreateWindow(1600, 900, "HedgeGI", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    return window;
}

void Application::initializeUI()
{
    // Initialize ImGui
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();

        ImGuiIO& io = ImGui::GetIO();
        {
            // courtesy of samyuu, love ya! <3

            ImGuiStyle* style = &ImGui::GetStyle();
            style->WindowPadding = ImVec2(2.0f, 2.0f);
            style->FrameBorderSize = 0.0f;
            style->ItemSpacing = ImVec2(8.0f, 2.0f);
            style->ItemInnerSpacing = ImVec2(2.0f, 4.0f);
            style->ScrollbarSize = 14.0f;
            style->IndentSpacing = 14.0f;
            style->GrabMinSize = 12.0f;

            if (const int minWindowWidth = ::GetSystemMetrics(SM_CXMIN); minWindowWidth != 0)
                style->WindowMinSize = ImVec2(static_cast<float>(minWindowWidth), 32.0f);

            ImVec4* colors = style->Colors;
            colors[ImGuiCol_Text] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_Border] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_BorderShadow] = ImVec4(0.36f, 0.36f, 0.36f, 0.21f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
            colors[ImGuiCol_SeparatorHovered] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
            colors[ImGuiCol_SeparatorActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
            colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            colors[ImGuiCol_ResizeGripActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_DockingPreview] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_PlotLines] = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.27f, 0.37f, 0.13f, 1.00f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.34f, 0.47f, 0.17f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.41f, 0.56f, 0.20f, 0.99f);
            colors[ImGuiCol_TextSelectedBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.27f);
            colors[ImGuiCol_DragDropTarget] = ImVec4(0.59f, 0.59f, 0.59f, 0.98f);
            colors[ImGuiCol_NavHighlight] = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);
        }

        {
            WCHAR windowsDirectoryPath[MAX_PATH];
            GetWindowsDirectory(windowsDirectoryPath, MAX_PATH);

            ImFontConfig config{};
            config.OversampleH = 2;
            config.OversampleV = 2;

            font = io.Fonts->AddFontFromFileTTF((wideCharToMultiByte(windowsDirectoryPath) + "\\Fonts\\segoeui.ttf").c_str(),
                16.0f, &config, io.Fonts->GetGlyphRangesDefault());

            io.Fonts->Build();
        }

        imGuiIniPath = getExecutableDirectoryPath() + "/ImGui.ini";
        io.IniFilename = imGuiIniPath.c_str();
    }
}

void Application::draw()
{
    ImGui::PushFont(font);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Stage"))
            {
                const std::string directoryPath = FileDialog::openFolder(L"Open Stage");

                if (!directoryPath.empty())
                {
                    destroyScene();

                    futureScene = std::async(std::launch::async, [this, directoryPath]()
                    {
                        propertyBagFilePath = directoryPath + "/" + getFileNameWithoutExtension(directoryPath) + ".hedgegi";
                        propertyBag.load(propertyBagFilePath);

                        camera.load(propertyBag);
                        bakeParams.load(propertyBag);

                        return SceneFactory::create(directoryPath);
                    });
                }
            }

            if (ImGui::MenuItem("Open Scene"))
            {
                const std::string filePath = FileDialog::openFile(L"HedgeGI Scene(.scene)\0*.scene\0", L"Open HedgeGI Scene");

                if (!filePath.empty())
                {
                    destroyScene();

                    futureScene = std::async(std::launch::async, [filePath]()
                        {
                            std::unique_ptr<Scene> scene = std::make_unique<Scene>();
                            scene->load(filePath);
                            scene->createRTCScene();

                            return scene;
                        });
                }
            }

            if (ImGui::MenuItem("Close"))
                destroyScene();

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                destroyScene();
                exit(0);
            }

            ImGui::EndMenu();
        }

        if (scene && ImGui::BeginMenu("Window"))
        {
            showScene ^= ImGui::MenuItem("Scene");
            showViewport ^= ImGui::MenuItem("Viewport");
            showSettings ^= ImGui::MenuItem("Settings");

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (futureScene.valid())
    {
        if (futureScene.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            scene = std::move(futureScene.get());
            futureScene = {};
        }

        else
            ImGui::OpenPopup("Loading");
    }

    if (ImGui::BeginPopupModal("Loading", nullptr, ImGuiWindowFlags_Static))
    {
        ImGui::Text("Loading...");
        ImGui::EndPopup();
    }

    if (scene == nullptr)
        return;

    if (showScene && ImGui::Begin("Scene", &showScene))
    {
        ImGui::End();
    }

    if (showViewport && ImGui::Begin("Viewport", &showViewport))
    {
        const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        const ImVec2 windowPos = ImGui::GetWindowPos();

        viewportWidth = std::max(1, (int)(contentMax.x - contentMin.x));
        viewportHeight = std::max(1, (int)(contentMax.y - contentMin.y));

        if (const Texture* texture = viewport.getTexture(); texture != nullptr)
        {
            ImGui::GetWindowDrawList()->AddImage((ImTextureID*)texture->id,
                { contentMin.x + windowPos.x, contentMin.y + windowPos.y }, { contentMax.x + windowPos.x, contentMax.y + windowPos.y });
        }

        ImGui::End();
    }

    if (showSettings && ImGui::Begin("Settings", &showSettings))
    {
        const BakeParams oldBakeParams = bakeParams;

        if (ImGui::BeginCombo("Target Engine", getTargetEngineString(bakeParams.targetEngine)))
        {
            if (ImGui::Selectable(getTargetEngineString(TARGET_ENGINE_HE1))) bakeParams.targetEngine = TARGET_ENGINE_HE1;
            if (ImGui::Selectable(getTargetEngineString(TARGET_ENGINE_HE2))) bakeParams.targetEngine = TARGET_ENGINE_HE2;

            ImGui::EndCombo();
        }

        if (ImGui::CollapsingHeader("Environment Color"))
        {
            ImGui::InputFloat("R", &bakeParams.environmentColor[0]);
            ImGui::InputFloat("G", &bakeParams.environmentColor[1]);
            ImGui::InputFloat("B", &bakeParams.environmentColor[2]);
        }

        if (ImGui::CollapsingHeader("Light Sampling"))
        {
            ImGui::InputScalar("Light Bounce Count", ImGuiDataType_U32, &bakeParams.lightBounceCount);
            ImGui::InputScalar("Light Sample Count", ImGuiDataType_U32, &bakeParams.lightSampleCount);
            ImGui::InputScalar("Russian Roulette Max Depth", ImGuiDataType_U32, &bakeParams.russianRouletteMaxDepth);
        }                                                 

        if (ImGui::CollapsingHeader("Shadow Sampling"))
        {
            ImGui::InputScalar("Shadow Sample Count", ImGuiDataType_U32, &bakeParams.shadowSampleCount);
            ImGui::InputFloat("Shadow Search Radius", &bakeParams.shadowSearchRadius);
            ImGui::InputFloat("Shadow Bias", &bakeParams.shadowBias);
        }

        if (ImGui::CollapsingHeader("Ambient Occlusion"))
        {
            ImGui::InputScalar("AO Sample Count", ImGuiDataType_U32, &bakeParams.aoSampleCount);
            ImGui::InputFloat("AO Fade Constant", &bakeParams.aoFadeConstant);
            ImGui::InputFloat("AO Fade Linear", &bakeParams.aoFadeLinear);
            ImGui::InputFloat("AO Fade Quadratic", &bakeParams.aoFadeQuadratic);
            ImGui::InputFloat("AO Fade Strength", &bakeParams.aoStrength);
        }

        if (ImGui::CollapsingHeader("Strength Modifiers"))
        {
            ImGui::InputFloat("Diffuse Strength", &bakeParams.diffuseStrength);
            ImGui::InputFloat("Diffuse Saturation", &bakeParams.diffuseSaturation);
            ImGui::InputFloat("Light Strength", &bakeParams.lightStrength);
            ImGui::InputFloat("Emission Strength", &bakeParams.emissionStrength);
        }

        if (ImGui::CollapsingHeader("Resolution"))
        {
            ImGui::InputFloat("Resolution Base", &bakeParams.resolutionBase);
            ImGui::InputFloat("Resolution Bias", &bakeParams.resolutionBias);
            ImGui::InputScalar("Resolution Override", ImGuiDataType_S16, &bakeParams.resolutionOverride);
            ImGui::InputScalar("Resolution Minimum", ImGuiDataType_U16, &bakeParams.resolutionMinimum);
            ImGui::InputScalar("Resolution Maximum", ImGuiDataType_U16, &bakeParams.resolutionMaximum);
        }

        if (ImGui::CollapsingHeader("Other"))
        {
            ImGui::Checkbox("Denoise Shadow Map", &bakeParams.denoiseShadowMap);
            ImGui::Checkbox("Optimize Seams", &bakeParams.optimizeSeams);

            if (ImGui::BeginCombo("Denoiser Type", getDenoiserTypeString(bakeParams.denoiserType)))
            {
                if (ImGui::Selectable(getDenoiserTypeString(DENOISER_TYPE_NONE))) bakeParams.denoiserType = DENOISER_TYPE_NONE;
                if (ImGui::Selectable(getDenoiserTypeString(DENOISER_TYPE_OPTIX))) bakeParams.denoiserType = DENOISER_TYPE_OPTIX;
                if (ImGui::Selectable(getDenoiserTypeString(DENOISER_TYPE_OIDN))) bakeParams.denoiserType = DENOISER_TYPE_OIDN;

                ImGui::EndCombo();
            }
        }

        if (ImGui::CollapsingHeader("Light Field"))
        {
            ImGui::InputFloat("Minimum Cell Radius", &bakeParams.lightFieldMinCellRadius);
            ImGui::InputFloat("AABB Size Multiplier", &bakeParams.lightFieldAabbSizeMultiplier);
        }

        dirty = memcmp(&oldBakeParams, &bakeParams, sizeof(BakeParams)) != 0;

        ImGui::End();
    }

    ImGui::PopFont();
}

void Application::drawFPS() const
{
    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_Static);
    ImGui::SetWindowPos(ImVec2(width - ImGui::GetWindowSize().x, 0));
    ImGui::Text("FPS: %.2f", 1.0f / elapsedTime);
    ImGui::Text("Time: %.2fms", elapsedTime * 1000);
    ImGui::End();
}

void Application::destroyScene()
{
    scene = nullptr;

    if (!propertyBagFilePath.empty())
    {
        camera.store(propertyBag);
        bakeParams.store(propertyBag);
        propertyBag.save(propertyBagFilePath);
    }

    propertyBagFilePath.clear();
}

Application::Application()
    : window(createGLFWwindow()), input(window), bakeParams(TARGET_ENGINE_HE1)
{
    initializeUI();
    bakeParams.load("D:/Repositories/HedgeGI/HedgeGI/HedgeGI/bin/Release/HedgeGI.ini");
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

int Application::getViewportWidth() const
{
    return viewportWidth;
}

int Application::getViewportHeight() const
{
    return viewportHeight;
}

bool Application::isDirty() const
{
    return dirty;
}

const RaytracingContext Application::getRaytracingContext() const
{
    return scene->getRaytracingContext();
}

const BakeParams Application::getBakeParams() const
{
    assert(scene != nullptr);
    return bakeParams;
}

PropertyBag& Application::getPropertyBag()
{
    return propertyBag;
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

    destroyScene();
}

void Application::update()
{
    glfwPollEvents();

    const double glfwTime = glfwGetTime();
    elapsedTime = (float)(glfwTime - currentTime);
    currentTime = glfwTime;

    glfwGetWindowSize(window, &width, &height);

    // Viewport
    if (scene && showViewport)
    {
        camera.update(*this);
        viewport.update(*this);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    glClearColor(0.22f, 0.22f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // GUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        draw();
        //drawFPS();
    }
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    input.postUpdate();
    dirty = false;

    glfwSwapBuffers(window);
}
