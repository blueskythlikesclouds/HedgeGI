#include "Application.h"

#include "BakingFactory.h"
#include "BitmapHelper.h"
#include "Camera.h"
#include "FileDialog.h"
#include "Framebuffer.h"
#include "Game.h"
#include "GIBaker.h"
#include "Input.h"
#include "Instance.h"
#include "Light.h"
#include "LightField.h"
#include "LightFieldBaker.h"
#include "Logger.h"
#include "Math.h"
#include "PropertyBag.h"
#include "SceneFactory.h"
#include "SeamOptimizer.h"
#include "SGGIBaker.h"
#include "SHLightField.h"
#include "SHLightFieldBaker.h"
#include "Utilities.h"
#include "Viewport.h"
#include "ModelProcessor.h"
#include "VertexColorRemover.h"
#include "UV2Mapper.h"
#include "MeshOptimizer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include "PostRender.h"

#define TRY_CANCEL() if (cancelBake) return;

const ImGuiWindowFlags ImGuiWindowFlags_Static = 
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoDocking;

std::string Application::TEMP_FILE_PATH = []()
{
    WCHAR path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);

    return wideCharToMultiByte(path) + "/HedgeGI.txt";
}();

GLFWwindow* Application::createGLFWwindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1600, 900, "HedgeGI", nullptr, nullptr);
    glfwMaximizeWindow(window);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSwapInterval(1);

    return window;
}

void Application::logListener(void* owner, const LogType logType, const char* text)
{
    Application* application = (Application*)owner;
    std::lock_guard lock(application->logMutex);

    const auto value = std::make_pair(logType, text);
    application->logs.remove(value);
    application->logs.push_back(value);
}

void Application::clearLogs()
{
    std::lock_guard lock(logMutex);
    logs.clear();
}

void Application::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    imGuiIniPath = getExecutableDirectoryPath() + "/ImGui.ini";
    io.IniFilename = imGuiIniPath.c_str();

    initializeStyle();
    initializeFonts();
}

void Application::initializeStyle()
{
    // courtesy of samyuu, love ya! <3

    ImGuiStyle* style = &ImGui::GetStyle();
    style->WindowPadding = ImVec2(4.0f, 4.0f);
    style->FrameBorderSize = 0.0f;
    style->ItemSpacing = ImVec2(8.0f, 4.0f);
    style->ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style->ScrollbarSize = 14.0f;
    style->IndentSpacing = 14.0f;
    style->GrabMinSize = 12.0f;
    style->FramePadding = ImVec2(8.0f, 4.0f);

    style->ChildRounding = 4.0f;
    style->FrameRounding = 4.0f;
    style->GrabRounding = 4.0f;
    style->PopupRounding = 4.0f;
    style->ScrollbarRounding = 4.0f;
    style->TabRounding = 4.0f;
    style->WindowRounding = 4.0f;

    style->AntiAliasedFill = true;
    style->AntiAliasedLines = true;

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

void Application::initializeFonts()
{
    ImGuiIO& io = ImGui::GetIO();

    WCHAR windowsDirectoryPath[MAX_PATH];
    GetWindowsDirectory(windowsDirectoryPath, MAX_PATH);

    ImFontConfig config {};
    config.OversampleH = 2;
    config.OversampleV = 2;

    font = io.Fonts->AddFontFromFileTTF((wideCharToMultiByte(windowsDirectoryPath) + "\\Fonts\\segoeui.ttf").c_str(),
        16.0f, &config, io.Fonts->GetGlyphRangesDefault());

    io.Fonts->Build();
}

void Application::initializeDocks()
{
    const ImGuiID dockSpaceId = ImGui::GetID("MyDockSpace");
    if (ImGui::DockBuilderGetNode(dockSpaceId))
        return;

    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId);

    ImGuiID mainId = dockSpaceId;
    ImGuiID topLeftId;
    ImGuiID bottomLeftId;
    ImGuiID centerId;
    ImGuiID topRightId;
    ImGuiID bottomRightId;

    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, 0.21f, &topLeftId, &centerId);
    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.3f, &topRightId, &centerId);
    ImGui::DockBuilderSplitNode(topLeftId, ImGuiDir_Up, 0.5f, &topLeftId, &bottomLeftId);
    ImGui::DockBuilderSplitNode(topRightId, ImGuiDir_Up, 0.75f, &topRightId, &bottomRightId);

    ImGui::DockBuilderDockWindow("Scene", topLeftId);
    ImGui::DockBuilderDockWindow("Baking Factory", bottomLeftId);
    ImGui::DockBuilderDockWindow("Viewport", centerId);
    ImGui::DockBuilderDockWindow("Settings", topRightId);
    ImGui::DockBuilderDockWindow("Logs", bottomRightId);

    ImGui::DockBuilderFinish(dockSpaceId);
}

bool Application::beginProperties(const char* name)
{
    return ImGui::BeginTable(name, 2);
}

void Application::beginProperty(const char* label, const float width)
{
    ImGui::TableNextColumn();
    const ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ cursorPos.x + 4, cursorPos.y + 4 });
    ImGui::TextUnformatted(label);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(width);
}

bool Application::property(const char* label, const enum ImGuiDataType_ dataType, void* data)
{
    beginProperty(label);
    return ImGui::InputScalar((std::string("##") + label).c_str(), dataType, data);
}

bool Application::property(const char* label, bool& data)
{
    beginProperty(label);
    return ImGui::Checkbox((std::string("##") + label).c_str(), &data);
}

bool Application::property(const char* label, Color3& data)
{
    beginProperty(label);
    return ImGui::ColorEdit3((std::string("##") + label).c_str(), data.data());
}

bool Application::property(const char* label, char* data, size_t dataSize, const float width)
{
    beginProperty(label, width);
    return ImGui::InputText((std::string("##") + label).c_str(), data, dataSize);
}

template <typename T>
bool Application::property(const char* label, const std::initializer_list<std::pair<const char*, T>>& values, T& data)
{
    beginProperty(label);

    const char* preview = "None";
    for (auto& value : values)
    {
        if (value.second != data)
            continue;

        preview = value.first;
        break;
    }

    if (!ImGui::BeginCombo((std::string("##") + label).c_str(), preview))
        return false;

    bool any = false;

    for (auto& value : values)
    {
        if (!ImGui::Selectable(value.first))
            continue;

        data = value.second;
        any = true;
    }

    ImGui::EndCombo();
    return any;
}

void Application::endProperties()
{
    ImGui::EndTable();
}

void Application::updateViewport()
{
    camera.update(*this);
    dirty |= camera.hasChanged();

    // Halven viewport resolution if we are dirty
    if (dirty)
    {
        viewportWidth /= 2;
        viewportHeight /= 2;
    }

    if (viewport.isBaking())
        return;

    const BakeParams oldBakeParams = bakeParams;
    {
        bakeParams.skyIntensity *= scene->effect.def.skyIntensityScale;
    }

    viewport.update(*this);

    dirty = false;
    bakeParams = oldBakeParams;
}

void Application::draw()
{
    float dockSpaceY = 0.0f;

    if (ImGui::BeginMainMenuBar())
    {
        dockSpaceY = ImGui::GetWindowSize().y;

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open"))
            {
                const std::string directoryPath = FileDialog::openFolder(L"Enter into a stage directory.");

                if (!directoryPath.empty())
                    loadScene(directoryPath);
            }

            if (ImGui::BeginMenu("Open Recent"))
            {
                for (auto& directoryPath : recentStages)
                {
                    if (ImGui::MenuItem(directoryPath.c_str()))
                        loadScene(directoryPath);
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Close"))
                destroyScene();

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                destroyScene();
                glfwSetWindowShouldClose(window, true);
            }

            ImGui::EndMenu();
        }

        if (scene && ImGui::BeginMenu("Window"))
        {
            showScene ^= ImGui::MenuItem("Scene");
            showSettings ^= ImGui::MenuItem("Settings");
            showBakingFactory ^= ImGui::MenuItem("Baking Factory");
            showViewport ^= ImGui::MenuItem("Viewport");
            showLogs ^= ImGui::MenuItem("Logs");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Generate Lightmap UVs"))
                processStage(UV2Mapper::process);

            if (ImGui::MenuItem("Remove Vertex Colors"))
                processStage(VertexColorRemover::process);

            if (ImGui::MenuItem("Optimize Models"))
                processStage(MeshOptimizer::process);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (futureScene.valid() && futureScene.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        scene = std::move(futureScene.get());
        futureScene = {};
    }

    else if (futureBake.valid() && futureBake.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        futureBake = {};

    else if (futureProcess.valid() && futureProcess.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        futureProcess = {};

    drawLoadingPopupUI();
    drawBakingPopupUI();
    drawProcessingPopupUI();

    if (!scene || futureScene.valid() || futureBake.valid() || futureProcess.valid())
        return;

    ImGui::SetNextWindowPos({ 0, dockSpaceY });
    ImGui::SetNextWindowSize({ (float)width, (float)height - dockSpaceY });

    if (!ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_Static))
        return ImGui::End();

    if (!docksInitialized)
    {
        initializeDocks();
        docksInitialized = true;
    }

    ImGui::DockSpace(ImGui::GetID("MyDockSpace"));
    {
        drawSceneUI();
        drawSettingsUI();
        drawBakingFactoryUI();
        drawViewportUI();
        drawLogsUI();
    }
    ImGui::End();
}

void Application::drawLoadingPopupUI()
{
    if (futureScene.valid())
        ImGui::OpenPopup("Loading");

    if (!ImGui::BeginPopupModal("Loading", nullptr, ImGuiWindowFlags_Static))
        return;

    ImGui::Text("Loading...");
    ImGui::EndPopup();
}

void Application::drawSceneUI()
{
    if (!showScene)
        return;

    if (!ImGui::Begin("Scene", &showScene))
        return ImGui::End();

    drawInstancesUI();
    drawLightsUI();

    ImGui::End();
}

void Application::drawInstancesUI()
{
    if (!ImGui::CollapsingHeader("Instances"))
        return;

    char search[1024] {};

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchInstances", search, sizeof(search));

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginListBox("##Instances"))
    {
        for (auto& instance : scene->instances)
        {
            const uint16_t resolution = propertyBag.get<uint16_t>(instance->name + ".resolution", 256);

            char name[1024];
            sprintf(name, "%s (%dx%d)", instance->name.c_str(), resolution, resolution);

            if (doSearch && !strstr(name, search))
                continue;

            if (ImGui::Selectable(name, selectedInstance == instance.get()))
                selectedInstance = instance.get();
        }

        ImGui::EndListBox();
    }

    if (selectedInstance != nullptr)
    {
        ImGui::Separator();

        uint16_t resolution = propertyBag.get<uint16_t>(selectedInstance->name + ".resolution", 256);
        if (beginProperties("##Instance Settings"))
        {
            if (property("Selected Instance Resolution", ImGuiDataType_U16, &resolution))
                propertyBag.set(selectedInstance->name + ".resolution", resolution);

            endProperties();
        }
    }

    ImGui::Separator();

    if (beginProperties("##Resolution Settings"))
    {
        property("Resolution Base", ImGuiDataType_Float, &bakeParams.resolutionBase);
        property("Resolution Bias", ImGuiDataType_Float, &bakeParams.resolutionBias);
        property("Resolution Minimum", ImGuiDataType_U16, &bakeParams.resolutionMinimum);
        property("Resolution Maximum", ImGuiDataType_U16, &bakeParams.resolutionMaximum);
        endProperties();
    }

    if (ImGui::Button("Compute New Resolutions"))
    {
        for (auto& instance : scene->instances)
        {
            uint16_t resolution = nextPowerOfTwo((int)exp2f(bakeParams.resolutionBias + logf(getRadius(instance->aabb)) / logf(bakeParams.resolutionBase)));
            resolution = std::max(bakeParams.resolutionMinimum, std::min(bakeParams.resolutionMaximum, resolution));

            propertyBag.set(instance->name + ".resolution", resolution);
        }
    }
}

void Application::drawLightsUI()
{
    if (!ImGui::CollapsingHeader("Lights"))
        return;

    char search[1024] {};

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchLights", search, sizeof(search));

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginListBox("##Lights"))
    {
        for (auto& light : scene->lights)
        {
            if (doSearch && light->name.find(search) == std::string::npos)
                continue;

            if (ImGui::Selectable(light->name.c_str(), selectedLight == light.get()))
                selectedLight = light.get();
        }

        ImGui::EndListBox();
    }

    if (selectedLight != nullptr)
    {
        ImGui::Separator();

        dirty |= ImGui::InputFloat3(selectedLight->type == LightType::Point ? "Position" : "Direction", selectedLight->positionOrDirection.data());
        dirty |= ImGui::InputFloat3("Color", selectedLight->color.data());

        if (selectedLight->type == LightType::Point)
            dirty |= ImGui::InputFloat4("Range", selectedLight->range.data());
        else
            selectedLight->positionOrDirection.normalize();
    }
}

void Application::drawSettingsUI()
{
    if (!showSettings)
        return;

    if (!ImGui::Begin("Settings", &showSettings))
        return ImGui::End();

    const BakeParams oldBakeParams = bakeParams;

    if (beginProperties("##Viewport Settings"))
    {
        property("Viewport Resolution", ImGuiDataType_Float, &viewportResolutionInvRatio);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Environment Color") && beginProperties("##Environment Color"))
    {
        property("Mode",
            {
                { "Color", EnvironmentColorMode::Color },
                { "Sky", EnvironmentColorMode::Sky }
            }, bakeParams.environmentColorMode);

        if (bakeParams.environmentColorMode == EnvironmentColorMode::Color)
        {
            if (bakeParams.targetEngine == TargetEngine::HE2)
            {
                Color3 environmentColor = bakeParams.environmentColor.pow(1.0f / 2.2f);

                if (property("Color", environmentColor))
                    bakeParams.environmentColor = environmentColor.pow(2.2f);

                property("Color Intensity", ImGuiDataType_Float, &bakeParams.environmentColorIntensity);
            }
            
            else
            {
                property("Color", bakeParams.environmentColor);
            }
        }

        else if (bakeParams.environmentColorMode == EnvironmentColorMode::Sky)
        {
            property("Sky Intensity", ImGuiDataType_Float, &bakeParams.skyIntensity);
        }

        endProperties();
    }

    if (ImGui::CollapsingHeader("Light Sampling") && beginProperties("##Light Sampling"))
    {
        property("Light Bounce Count", ImGuiDataType_U32, &bakeParams.lightBounceCount);
        property("Light Sample Count", ImGuiDataType_U32, &bakeParams.lightSampleCount);
        property("Russian Roulette Max Depth", ImGuiDataType_U32, &bakeParams.russianRouletteMaxDepth);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Shadow Sampling") && beginProperties("##Shadow Sampling"))
    {
        property("Shadow Sample Count", ImGuiDataType_U32, &bakeParams.shadowSampleCount);
        property("Shadow Search Radius", ImGuiDataType_Float, &bakeParams.shadowSearchRadius);
        property("Shadow Bias", ImGuiDataType_Float, &bakeParams.shadowBias);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Ambient Occlusion") && beginProperties("##Ambient Occlusion"))
    {
        property("AO Sample Count", ImGuiDataType_U32, &bakeParams.aoSampleCount);
        property("AO Fade Constant", ImGuiDataType_Float, &bakeParams.aoFadeConstant);
        property("AO Fade Linear", ImGuiDataType_Float, &bakeParams.aoFadeLinear);
        property("AO Fade Quadratic", ImGuiDataType_Float, &bakeParams.aoFadeQuadratic);
        property("AO Fade Strength", ImGuiDataType_Float, &bakeParams.aoStrength);
        endProperties();
    }

    if (ImGui::CollapsingHeader("Strength Modifiers") && beginProperties("##Strength Modifiers"))
    {
        property("Diffuse Strength", ImGuiDataType_Float, &bakeParams.diffuseStrength);
        property("Diffuse Saturation", ImGuiDataType_Float, &bakeParams.diffuseSaturation);
        property("Light Strength", ImGuiDataType_Float, &bakeParams.lightStrength);
        property("Emission Strength", ImGuiDataType_Float, &bakeParams.emissionStrength);
        endProperties();
    }

    ImGui::End();

    dirty |= memcmp(&oldBakeParams, &bakeParams, sizeof(BakeParams)) != 0;
}

void Application::drawBakingFactoryUI()
{
    if (!showBakingFactory)
        return;

    if (!ImGui::Begin("Baking Factory", &showBakingFactory))
        return ImGui::End();

    if (ImGui::CollapsingHeader("Output"))
    {
        if (beginProperties("##Baking Factory Settings"))
        {
            char outputDirPath[1024];
            strcpy(outputDirPath, outputDirectoryPath.c_str());

            if (property("Output Directory", outputDirPath, sizeof(outputDirPath), -32))
                outputDirectoryPath = outputDirPath;

            ImGui::SameLine();

            if (ImGui::Button("..."))
            {
                if (const std::string newOutputDirectoryPath = FileDialog::openFolder(L"Open Output Folder"); !newOutputDirectoryPath.empty())
                    outputDirectoryPath = newOutputDirectoryPath;
            }

            endProperties();
        }

        const float buttonWidth = (ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * 2) / 2;

        if (ImGui::Button("Open in Explorer", { buttonWidth, 0 }))
        {
            std::filesystem::create_directory(outputDirectoryPath);
            std::system(("explorer \"" + outputDirectoryPath + "\"").c_str());
        }

        ImGui::SameLine();

        if (ImGui::Button("Clean Directory", { buttonWidth, 0 }))
            clean();
    }

    if (ImGui::CollapsingHeader("Baker"))
    {
        if (beginProperties("##Mode Settings"))
        {
            property("Mode",
                {
                    { "Global Illumination", BakingFactoryMode::GI },
                    { "Light Field", BakingFactoryMode::LightField }
                },
                mode
                );

            dirty |= property("Engine",
                {
                    { "Hedgehog Engine 1", TargetEngine::HE1},
                    { "Hedgehog Engine 2", TargetEngine::HE2}
                },
                bakeParams.targetEngine
                );

            endProperties();
            ImGui::Separator();
        }

        if (beginProperties("##Mode Specific Settings"))
        {
            if (mode == BakingFactoryMode::LightField && bakeParams.targetEngine == TargetEngine::HE1)
            {
                property("Minimum Cell Radius", ImGuiDataType_Float, &bakeParams.lightFieldMinCellRadius);
                property("AABB Size Multiplier", ImGuiDataType_Float, &bakeParams.lightFieldAabbSizeMultiplier);
            }
            else if (mode == BakingFactoryMode::GI)
            {
                property("Denoise Shadow Map", bakeParams.denoiseShadowMap);
                property("Optimize Seams", bakeParams.optimizeSeams);
                property("Skip Existing Files", skipExistingFiles);

                property("Denoiser Type",
                    {
                        {"None", DenoiserType::None},
                        {"Optix AI", DenoiserType::Optix},
                        {"oidn", DenoiserType::Oidn},
                    },
                    bakeParams.denoiserType);

                property("Resolution Override", ImGuiDataType_S16, &bakeParams.resolutionOverride);
            }

            endProperties();
        }

        const float buttonWidth = (ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * 3) / 3;

        if (ImGui::Button("Bake", { buttonWidth / 2, 0 }))
        {
            futureBake = std::async(std::launch::async, [&]()
            {
                bake();
            });
        }

        ImGui::SameLine();

        if (ImGui::Button("Bake and Pack", { buttonWidth * 2, 0 }))
        {
            futureBake = std::async(std::launch::async, [&]()
            {
                bake();
                pack();
            });
        }

        ImGui::SameLine();

        if (ImGui::Button("Pack", { buttonWidth / 2, 0 }))
            pack();
    }

    ImGui::End();
}

void Application::drawViewportUI()
{
    if (!showViewport)
        return;

    if (!ImGui::Begin("Viewport", &showViewport))
        return ImGui::End();

    const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    const ImVec2 windowPos = ImGui::GetWindowPos();

    const ImVec2 min = { contentMin.x + windowPos.x, contentMin.y + windowPos.y };
    const ImVec2 max = { contentMax.x + windowPos.x, contentMax.y + windowPos.y };

    updateViewport();

    if (const Texture* texture = viewport.getTexture(); texture != nullptr)
    {
        ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)texture->id, min, max,
            { 0, 1 }, { viewport.getNormalizedWidth(), 1.0f - viewport.getNormalizedHeight() });
    }

    viewportWidth = std::max(1, (int)(contentMax.x - contentMin.x));
    viewportHeight = std::max(1, (int)(contentMax.y - contentMin.y));

    viewportResolutionInvRatio = std::max(viewportResolutionInvRatio, 1.0f);
    viewportWidth = (int)((float)viewportWidth / viewportResolutionInvRatio);
    viewportHeight = (int)((float)viewportHeight / viewportResolutionInvRatio);
    viewportFocused = ImGui::IsMouseHoveringRect(min, max) || ImGui::IsWindowFocused();

    ImGui::End();
}

bool Application::drawLogsContainerUI(const ImVec2& size)
{
    std::lock_guard lock(logMutex);
    if (logs.empty() || !ImGui::BeginListBox("##Logs", size))
        return false;

    for (auto& log : logs)
    {
        const ImVec4 colors[] =
        {
            ImVec4(0.13f, 0.7f, 0.3f, 1.0f), // Success
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // Normal
            ImVec4(1.0f, 0.78f, 0.054f, 1.0f), // Warning
            ImVec4(1.0f, 0.02f, 0.02f, 1.0f) // Error
        };

        ImGui::TextColored(colors[(size_t)log.first], log.second.c_str());
    }

    if (previousLogSize != logs.size())
        ImGui::SetScrollHereY();

    previousLogSize = logs.size();

    ImGui::EndListBox();
    return true;
}

void Application::drawLogsUI()
{
    if (!showLogs)
        return;

    if (!ImGui::Begin("Logs", &showLogs))
        return ImGui::End();

    const ImVec2 min = ImGui::GetWindowContentRegionMin();
    const ImVec2 max = ImGui::GetWindowContentRegionMax();

    drawLogsContainerUI({ max.x - min.x, max.y - min.y });
    ImGui::End();
}

void Application::setTitle(const float fps)
{
    if (titleUpdateTime < 0.5f)
    {
        titleUpdateTime += elapsedTime;
        return;
    }
    titleUpdateTime = 0.0f;

    char title[256];

    if (!stageName.empty())
        sprintf(title, "Hedge GI - %s - %s (FPS: %d)", stageName.c_str(), GAME_NAMES[(size_t)game], (int)fps);
    else
        sprintf(title, "Hedge GI (FPS: %d)", (int)fps);

    glfwSetWindowTitle(window, title);
}

void Application::loadProperties()
{
    camera.load(propertyBag);
    bakeParams.load(propertyBag);
    viewportResolutionInvRatio = propertyBag.get(PROP("viewportResolutionInvRatio"), 2.0f);
    outputDirectoryPath = propertyBag.getString(PROP("outputDirectoryPath"), stageDirectoryPath + "-HedgeGI");
    mode = propertyBag.get(PROP("mode"), BakingFactoryMode::GI);
    skipExistingFiles = propertyBag.get(PROP("skipExistingFiles"), false);

    if (game == Game::Forces)
        bakeParams.targetEngine = TargetEngine::HE2;
}

void Application::storeProperties()
{
    camera.store(propertyBag);
    bakeParams.store(propertyBag);
    propertyBag.set(PROP("viewportResolutionInvRatio"), viewportResolutionInvRatio);
    propertyBag.setString(PROP("outputDirectoryPath"), outputDirectoryPath);
    propertyBag.set(PROP("mode"), mode);
    propertyBag.set(PROP("skipExistingFiles"), skipExistingFiles);
}

void Application::destroyScene()
{
    // Wait till viewport is done baking since it holds onto the raytracing context
    viewport.waitForBake();

    scene = nullptr;

    if (!stageDirectoryPath.empty() && !stageName.empty())
    {
        storeProperties();
        propertyBag.save(stageDirectoryPath + "/" + stageName + ".hgi");
    }

    stageDirectoryPath.clear();
    stageName.clear();

    dirty = true;
}

void Application::addRecentStage(const std::string& path)
{
    recentStages.remove(path);

    if (recentStages.size() >= 10)
        recentStages.pop_back();

    recentStages.push_front(path);
}

void Application::loadRecentStages()
{
    std::ifstream file(TEMP_FILE_PATH, std::ios::in);
    if (!file.is_open())
        return;

    std::string path;
    while (std::getline(file, path))
    {
        if (std::filesystem::exists(path))
            recentStages.push_back(path);
    }
}   

void Application::saveRecentStages() const
{
    std::ofstream file(TEMP_FILE_PATH, std::ios::out);
    if (!file.is_open())
        return;

    for (auto& path : recentStages)
        file << path << std::endl; 
}

void Application::drawBakingPopupUI()
{
    if (futureBake.valid())
        ImGui::OpenPopup("Baking");

    if (!ImGui::BeginPopupModal("Baking", nullptr, ImGuiWindowFlags_Static))
        return;

    ImGui::Text(cancelBake ? "Cancelling..." : "Baking...");

    ImGui::Separator();

    const float popupWidth = (float)width / 4;
    const float popupHeight = (float)height / 4;

    if (drawLogsContainerUI({ popupWidth, popupHeight }))
        ImGui::Separator();

    if (mode == BakingFactoryMode::GI || bakeParams.targetEngine == TargetEngine::HE2)
    {
        const Instance* const lastBakedInstance = this->lastBakedInstance;
        const SHLightField* const lastBakedShlf = this->lastBakedShlf;
        const size_t bakeProgress = this->bakeProgress;

        ImGui::SetNextItemWidth(popupWidth);

        if (mode == BakingFactoryMode::GI)
        {
            char overlay[1024];
            if (lastBakedInstance != nullptr)
            {
                const uint16_t resolution = bakeParams.resolutionOverride > 0 ? bakeParams.resolutionOverride : propertyBag.get(lastBakedInstance->name + ".resolution", 256);
                sprintf(overlay, "%s (%dx%d)", lastBakedInstance->name.c_str(), resolution, resolution);
            }

            ImGui::ProgressBar(bakeProgress / ((float)scene->instances.size() + 1), { 0, 0 }, lastBakedInstance ? overlay : nullptr);
        }
        else if (mode == BakingFactoryMode::LightField)
        {
            char overlay[1024];
            if (lastBakedShlf != nullptr)
                sprintf(overlay, "%s (%dx%dx%d)", lastBakedShlf->name.c_str(), lastBakedShlf->resolution.x(), lastBakedShlf->resolution.y(), lastBakedShlf->resolution.z());

            ImGui::ProgressBar(bakeProgress / ((float)scene->shLightFields.size() + 1), { 0, 0 }, lastBakedShlf ? overlay : nullptr);
        }

        ImGui::Separator();
    }

    if (ImGui::Button("Cancel"))
        cancelBake = true;

    ImGui::EndPopup();
}

void Application::bake()
{
    viewport.waitForBake();

    bakeProgress = 0;
    lastBakedInstance = nullptr;
    lastBakedShlf = nullptr;
    cancelBake = false;

    clearLogs();

    std::filesystem::create_directory(outputDirectoryPath);

    const auto begin = std::chrono::high_resolution_clock::now();

    if (mode == BakingFactoryMode::GI)
        bakeGI();

    else if (mode == BakingFactoryMode::LightField)
        bakeLightField();

    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin);

    const int seconds = (int)(duration.count() % 60);
    const int minutes = (int)((duration.count() / 60) % 60);
    const int hours = (int)(duration.count() / (60 * 60));

    Logger::logFormatted(LogType::Success, "Bake completed in %02dh:%02dm:%02ds!", hours, minutes, seconds);

    alert();
}

void Application::bakeGI()
{
    std::for_each(std::execution::par_unseq, scene->instances.begin(), scene->instances.end(), [&](const std::unique_ptr<const Instance>& instance)
    {
        TRY_CANCEL()

        bool skip = instance->name.find("_NoGI") != std::string::npos || instance->name.find("_noGI") != std::string::npos;
    
        const bool isSg = !skip && bakeParams.targetEngine == TargetEngine::HE2 && propertyBag.get(instance->name + ".isSg", false);
    
        std::string lightMapFileName;
        std::string shadowMapFileName;
    
        if (!skip)
        {
            if (bakeParams.targetEngine == TargetEngine::HE2)
            {
                lightMapFileName = isSg ? instance->name + "_sg.dds" : instance->name + ".dds";
                shadowMapFileName = instance->name + "_occlusion.dds";
            }
            else if (game == Game::LostWorld)
            {
                lightMapFileName = instance->name + ".dds";
            }
            else
            {
                lightMapFileName = instance->name + "_lightmap.png";
                shadowMapFileName = instance->name + "_shadowmap.png";
            }
        }
    
        lightMapFileName = outputDirectoryPath + "/" + lightMapFileName;
        skip |= skipExistingFiles && std::filesystem::exists(lightMapFileName);
    
        if (!shadowMapFileName.empty())
        {
            shadowMapFileName = outputDirectoryPath + "/" + shadowMapFileName;
            skip |= skipExistingFiles && std::filesystem::exists(shadowMapFileName);
        }
    
        if (skip)
        {
            Logger::logFormatted(LogType::Normal, "Skipped %s", instance->name.c_str());

            ++bakeProgress;
            lastBakedInstance = instance.get();
            return;
        }

        const uint16_t resolution = bakeParams.resolutionOverride > 0 ? bakeParams.resolutionOverride :
            propertyBag.get(instance->name + ".resolution", 256);
    
        if (isSg)
        {
            GIPair pair;
            {
                const std::lock_guard<std::mutex> lock = BakingFactory::lock();
                {
                    ++bakeProgress;
                    lastBakedInstance = instance.get();
                }

                TRY_CANCEL()

                pair = SGGIBaker::bake(scene->getRaytracingContext(), *instance, resolution, bakeParams);
            }

            TRY_CANCEL()
    
            // Dilate
            pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
            pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

            TRY_CANCEL()

            // Denoise
            if (bakeParams.denoiserType != DenoiserType::None)
            {
                pair.lightMap = BitmapHelper::denoise(*pair.lightMap, bakeParams.denoiserType);
    
                if (bakeParams.denoiseShadowMap)
                    pair.shadowMap = BitmapHelper::denoise(*pair.shadowMap, bakeParams.denoiserType);
            }

            TRY_CANCEL()
    
            if (bakeParams.optimizeSeams)
            {
                const SeamOptimizer seamOptimizer(*instance);
                pair.lightMap = seamOptimizer.optimize(*pair.lightMap);
                pair.shadowMap = seamOptimizer.optimize(*pair.shadowMap);
            }

            TRY_CANCEL()
    
            pair.lightMap->save(lightMapFileName, game == Game::Generations ? DXGI_FORMAT_R16G16B16A16_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT);
            pair.shadowMap->save(shadowMapFileName, game == Game::Generations ? DXGI_FORMAT_R8_UNORM : SGGIBaker::SHADOW_MAP_FORMAT);
        }
        else
        {
            GIPair pair;
            {
                const std::lock_guard<std::mutex> lock = BakingFactory::lock();
                {
                    ++bakeProgress;
                    lastBakedInstance = instance.get();
                }

                TRY_CANCEL()

                pair = GIBaker::bake(scene->getRaytracingContext(), *instance, resolution, bakeParams);
            }

            TRY_CANCEL()
    
            // Dilate
            pair.lightMap = BitmapHelper::dilate(*pair.lightMap);
            pair.shadowMap = BitmapHelper::dilate(*pair.shadowMap);

            TRY_CANCEL()

            // Combine
            auto combined = BitmapHelper::combine(*pair.lightMap, *pair.shadowMap);

            TRY_CANCEL()

            // Denoise
            if (bakeParams.denoiserType != DenoiserType::None)
                combined = BitmapHelper::denoise(*combined, bakeParams.denoiserType, bakeParams.denoiseShadowMap);

            TRY_CANCEL()

            // Optimize seams
            if (bakeParams.optimizeSeams)
                combined = BitmapHelper::optimizeSeams(*combined, *instance);

            TRY_CANCEL()

            // Make ready for encoding
            if (bakeParams.targetEngine == TargetEngine::HE1)
                combined = BitmapHelper::makeEncodeReady(*combined, ENCODE_READY_FLAGS_SQRT);

            TRY_CANCEL()

            if (game == Game::Generations && bakeParams.targetEngine == TargetEngine::HE1)
            {
                combined->save(lightMapFileName, Bitmap::transformToLightMap);
                combined->save(shadowMapFileName, Bitmap::transformToShadowMap);
            }
            else if (game == Game::LostWorld)
            {
                combined->save(lightMapFileName, DXGI_FORMAT_BC3_UNORM);
            }
            else if (bakeParams.targetEngine == TargetEngine::HE2)
            {
                combined->save(lightMapFileName, game == Game::Generations ? DXGI_FORMAT_R16G16B16A16_FLOAT : SGGIBaker::LIGHT_MAP_FORMAT, Bitmap::transformToLightMap);
                combined->save(shadowMapFileName, game == Game::Generations ? DXGI_FORMAT_R8_UNORM : SGGIBaker::SHADOW_MAP_FORMAT, Bitmap::transformToShadowMap);
            }
        }
    });

    ++bakeProgress;
    lastBakedInstance = nullptr;
}

void Application::bakeLightField()
{
    if (bakeParams.targetEngine == TargetEngine::HE2)
    {
        std::for_each(std::execution::par_unseq, scene->shLightFields.begin(), scene->shLightFields.end(), [&](const std::unique_ptr<SHLightField>& shlf)
        {            
            ++bakeProgress;
            lastBakedShlf = shlf.get();

            TRY_CANCEL()

            auto bitmap = SHLightFieldBaker::bake(scene->getRaytracingContext(), *shlf, bakeParams);

            TRY_CANCEL()

            if (bakeParams.denoiserType != DenoiserType::None)
                bitmap = BitmapHelper::denoise(*bitmap, bakeParams.denoiserType);

            TRY_CANCEL()

            bitmap->save(outputDirectoryPath + "/" + shlf->name + ".dds", DXGI_FORMAT_R16G16B16A16_FLOAT);
        });

        ++bakeProgress;
        lastBakedShlf = nullptr;
    }

    else if (bakeParams.targetEngine == TargetEngine::HE1)
    {
        TRY_CANCEL()

        std::unique_ptr<LightField> lightField = LightFieldBaker::bake(scene->getRaytracingContext(), bakeParams);

        Logger::log(LogType::Normal, "Saving...\n");

        TRY_CANCEL()

        lightField->save(outputDirectoryPath + "/light-field.lft");
    }
}

void Application::clean()
{
    process([&]()
    {
        for (auto& file : std::filesystem::directory_iterator(outputDirectoryPath))
        {
            if (!file.is_regular_file())
                continue;

            std::string extension = file.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

            if (extension != ".png" && extension != ".dds" && extension != ".lft")
                continue;

            std::filesystem::remove(file);
            Logger::logFormatted(LogType::Normal, "Deleted %s", toUtf8(file.path().filename().c_str()));
        }
    });
}

void Application::pack()
{
    process([this]()
    {
        if (mode == BakingFactoryMode::LightField)
            packLightField();

        else if (mode == BakingFactoryMode::GI)
        {
            switch (game)
            {
            case Game::Generations:
                packGenerationsGI();
                break;

            case Game::LostWorld:
            case Game::Forces:
                packLostWorldOrForcesGI();
                break;

            default: break;
            }
        }
    });
}

void Application::packLightField()
{
    std::string archiveFileName;

    if (game == Game::Generations)
        archiveFileName = stageName + ".ar.00";

    else if (game == Game::LostWorld || game == Game::Forces)
        archiveFileName = stageName + "_trr_cmn.pac";

    else return;

    auto nArchiveFilePath = toNchar((stageDirectoryPath + "/" + archiveFileName).c_str());

    hl::archive archive = game == Game::Generations ? hl::hh::ar::load(nArchiveFilePath.data()) : hl::pacx::load(nArchiveFilePath.data());

    if (bakeParams.targetEngine == TargetEngine::HE1)
    {
        const std::string lightFieldFilePath = outputDirectoryPath + "/light-field.lft";
        if (!std::filesystem::exists(lightFieldFilePath))
        {
            Logger::log(LogType::Error, "Failed to find light-field.lft");
            return;
        }

        Logger::log(LogType::Normal, "Packed light-field.lft");

        addOrReplace(archive, HL_NTEXT("light-field.lft"), hl::blob(toNchar(lightFieldFilePath.c_str()).data()));
    }

    else if (bakeParams.targetEngine == TargetEngine::HE2)
    {
        bool any = false;

        for (auto& shLightField : scene->shLightFields)
        {
            const std::string textureFileName = shLightField->name + ".dds";
            const std::string textureFilePath = outputDirectoryPath + "/" + textureFileName;
            if (!std::filesystem::exists(textureFilePath))
                continue;

            Logger::logFormatted(LogType::Normal, "Packing %s", textureFileName.c_str());

            addOrReplace(archive, toNchar(textureFileName.c_str()).data(), hl::blob(toNchar(textureFilePath.c_str()).data()));
            any = true;
        }

        if (!any) 
        {
            Logger::log(LogType::Error, "Failed to find light field textures");
            return;
        }
    }

    else return;

    switch (game)
    {
    case Game::Generations:
        hl::hh::ar::save(archive, nArchiveFilePath.data());
        break;

    case Game::LostWorld:
        hl::pacx::v2::save(archive, hl::bina::endian_flag::little, hl::pacx::lw_exts, hl::pacx::lw_ext_count, nArchiveFilePath.data());
        break;

    case Game::Forces:
        hl::pacx::v3::save(archive, hl::bina::endian_flag::little, hl::pacx::forces_exts, hl::pacx::forces_ext_count, nArchiveFilePath.data());
        break;

    default: return;
    }

    Logger::logFormatted(LogType::Normal, "Saved %s", archiveFileName.c_str());
}

void Application::packGenerationsGI()
{
    PostRender::process(stageDirectoryPath, outputDirectoryPath, bakeParams.targetEngine);
}

void Application::packLostWorldOrForcesGI()
{
    static const char* lwSuffixes[] = { ".dds" };
    static const char* forcesSuffixes[] = { ".dds", "_sg.dds", "_occlusion.dds" };

    const char** suffixes;
    size_t suffixCount;

    if (game == Game::LostWorld)
    {
        suffixes = lwSuffixes;
        suffixCount = _countof(lwSuffixes);
    }
    else if (game == Game::Forces)
    {
        suffixes = forcesSuffixes;
        suffixCount = _countof(forcesSuffixes);
    }
    else return;

    auto processArchive = [&](const std::string& archiveFileName)
    {
        const std::string archiveFilePath = stageDirectoryPath + "/" + archiveFileName;
        if (!std::filesystem::exists(archiveFilePath))
            return;

        auto nArchiveFilePath = toNchar(archiveFilePath.c_str());

        hl::archive archive = hl::pacx::load(nArchiveFilePath.data());

        bool any = false;

        for (auto& instance : scene->instances)
        {
            for (size_t i = 0; i < suffixCount; i++)
            {
                // If the texture exists, clean it from the archive, 
                // then add it to the archive if the corresponding terrain model/instance info exists.
                const std::string textureFileName = instance->name + suffixes[i];
                const std::string textureFilePath = outputDirectoryPath + "/" + textureFileName;

                if (!std::filesystem::exists(textureFilePath))
                    continue;

                auto nTextureFileName = toNchar(textureFileName.c_str());
                auto nTextureFilePath = toNchar(textureFilePath.c_str());

                for (size_t j = 0; j < archive.size();)
                {
                    const hl::archive_entry& entry = archive[j];
                    if (!hl::text::iequal(entry.name(), nTextureFileName.data()))
                    {
                        j++;
                        continue;
                    }

                    archive.erase(archive.begin() + j);
                    any = true;
                }

                auto instanceFileName = toNchar((instance->name + ".terrain-instanceinfo").c_str());
                auto modelFileName = toNchar((instance->name + ".terrain-model").c_str());

                for (size_t j = 0; j < archive.size(); j++)
                {
                    const hl::archive_entry& entry = archive[j];
                    if (!hl::text::iequal(entry.name(), instanceFileName.data()) &&
                        !hl::text::iequal(entry.name(), modelFileName.data()))
                        continue;

                    Logger::logFormatted(LogType::Normal, "Packing %s", textureFileName.c_str());

                    hl::blob blob(nTextureFilePath.data());
                    archive.push_back(std::move(hl::archive_entry::make_regular_file(nTextureFileName.data(), blob.size(), blob.data())));

                    any = true;
                    break;
                }
            }
        }

        if (!any) return;

        if (game == Game::LostWorld)
            hl::pacx::v2::save(archive, hl::bina::endian_flag::little, hl::pacx::lw_exts, hl::pacx::lw_ext_count, nArchiveFilePath.data());

        else if (game == Game::Forces)
            hl::pacx::v3::save(archive, hl::bina::endian_flag::little, hl::pacx::forces_exts, hl::pacx::forces_ext_count, nArchiveFilePath.data());

        Logger::logFormatted(LogType::Normal, "Saved %s", archiveFileName.c_str());
    };

    processArchive(stageName + "_trr_cmn.pac");

    for (size_t i = 0; i < 100; i++)
    {
        char sector[4];
        sprintf(sector, "%02d", (int)i);

        processArchive(stageName + "_trr_s" + sector + ".pac");
    }
}

void Application::drawProcessingPopupUI()
{
    if (futureProcess.valid())
        ImGui::OpenPopup("Processing");

    const float popupWidth = (float)width / 2;
    const float popupHeight = (float)height / 2;
    const float popupX = ((float)width - popupWidth) / 2;
    const float popupY = ((float)height - popupHeight) / 2;

    ImGui::SetNextWindowSize({ popupWidth, popupHeight });
    ImGui::SetNextWindowPos({ popupX, popupY });

    if (!ImGui::BeginPopupModal("Processing", nullptr, ImGuiWindowFlags_Static))
        return;

    ImGui::Text("Logs");

    ImGui::Separator();
    drawLogsContainerUI({ popupWidth, popupHeight - 32 });

    if (!futureProcess.valid())
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void Application::process(std::function<void()> function)
{
    futureProcess = std::async([this, function]()
    {
        if (!futureBake.valid())
            clearLogs();

        function();
        alert();
    });
}

void Application::processStage(ModelProcessor::ProcModelFunc function)
{
    const std::string directoryPath = FileDialog::openFolder(L"Enter into a stage directory.");
    
    if (directoryPath.empty())
        return;

    process([=]()
    {
        ModelProcessor::processStage(directoryPath, function);
    
        // Reload stage if we processed the currently open one
        if (scene && std::filesystem::canonical(directoryPath) == std::filesystem::canonical(stageDirectoryPath))
            loadScene(directoryPath);
    });
}

Application::Application()
    : window(createGLFWwindow()), input(window), bakeParams(TargetEngine::HE1)
{
    Logger::addListener(this, logListener);
    initializeImGui();
    loadRecentStages();
}

Application::~Application()
{
    Logger::removeListener(this);
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();
    ImGui_ImplOpenGL3_Shutdown();
    saveRecentStages();
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

bool Application::isViewportFocused() const
{
    return viewportFocused;
}

bool Application::isDirty() const
{
    return dirty;
}

void Application::loadScene(const std::string& directoryPath)
{
    destroyScene();

    futureScene = std::async(std::launch::async, [this, directoryPath]()
    {
        clearLogs();
        stageName = getFileNameWithoutExtension(directoryPath);
        stageDirectoryPath = directoryPath;
        addRecentStage(directoryPath);
        game = detectGameFromStageDirectory(stageDirectoryPath);

        propertyBag.load(directoryPath + "/" + stageName + ".hgi");
        loadProperties();

        return SceneFactory::create(directoryPath);
    });
}

Game Application::getGame() const
{
    return game;
}

const RaytracingContext Application::getRaytracingContext() const
{
    return scene->getRaytracingContext();
}

const SceneEffect& Application::getSceneEffect() const
{
    return scene->effect;
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

    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED) && !futureScene.valid() && !futureBake.valid() && !futureProcess.valid())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }

    const double glfwTime = glfwGetTime();
    elapsedTime = (float)(glfwTime - currentTime);
    currentTime = glfwTime;

    glfwGetWindowSize(window, &width, &height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(font);
    {
        draw();
    }
    ImGui::PopFont();
    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    glClearColor(0.22f, 0.22f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    input.postUpdate();

    glfwSwapBuffers(window);
    setTitle(1.0f / elapsedTime);
}
