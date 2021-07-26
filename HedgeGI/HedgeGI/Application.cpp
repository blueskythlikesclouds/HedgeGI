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

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

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

    return window;
}

void Application::logListener(void* owner, const char* text)
{
    Application* application = (Application*)owner;
    std::lock_guard lock(application->logMutex);
    application->logs.emplace_back(text);
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
    ImGuiID rightId;

    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, 0.21f, &topLeftId, &centerId);
    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.3f, &rightId, &centerId);
    ImGui::DockBuilderSplitNode(topLeftId, ImGuiDir_Up, 0.5f, &topLeftId, &bottomLeftId);

    ImGui::DockBuilderDockWindow("Scene", topLeftId);
    ImGui::DockBuilderDockWindow("Baking Factory", bottomLeftId);
    ImGui::DockBuilderDockWindow("Viewport", centerId);
    ImGui::DockBuilderDockWindow("Settings", rightId);

    ImGui::DockBuilderFinish(dockSpaceId);
}

bool Application::beginProperties(const char* name)
{
    return ImGui::BeginTable(name, 2);
}

void Application::beginProperty(const char* label)
{
    ImGui::TableNextColumn();
    const ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ cursorPos.x + 4, cursorPos.y + 4 });
    ImGui::TextUnformatted(label);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
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

bool Application::property(const char* label, char* data, size_t dataSize)
{
    beginProperty(label);
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
            showViewport ^= ImGui::MenuItem("Viewport");
            showSettings ^= ImGui::MenuItem("Settings");
            showBakingFactory ^= ImGui::MenuItem("Baking Factory");

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

    drawLoadingPopupUI();
    drawBakingPopupUI();

    if (!scene || futureScene.valid() || futureBake.valid())
    {
        viewportVisible = false;
        return;
    }

    ImGui::SetNextWindowPos({ 0, dockSpaceY });
    ImGui::SetNextWindowSize({ (float)width, (float)height - dockSpaceY });
    ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_Static);

    if (!docksInitialized)
    {
        initializeDocks();
        docksInitialized = true;
    }

    ImGui::DockSpace(ImGui::GetID("MyDockSpace"));
    {
        drawSceneUI();
        drawViewportUI();
        drawSettingsUI();
        drawBakingFactoryUI();
    }
    ImGui::End();
}

void Application::drawLoadingPopupUI()
{
    if (openLoadingPopup)
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
    ImGui::BeginListBox("##Instances");

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
    ImGui::BeginListBox("##Lights");

    for (auto& light : scene->lights)
    {
        if (doSearch && light->name.find(search) == std::string::npos)
            continue;

        if (ImGui::Selectable(light->name.c_str(), selectedLight == light.get()))
            selectedLight = light.get();
    }

    ImGui::EndListBox();

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

void Application::drawViewportUI()
{
    if (!showViewport)
    {
        viewportVisible = false;
        return;
    }

    if (!ImGui::Begin("Viewport", &showViewport))
        return ImGui::End();

    viewportVisible = true;

    const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    const ImVec2 windowPos = ImGui::GetWindowPos();

    const ImVec2 min = { contentMin.x + windowPos.x, contentMin.y + windowPos.y };
    const ImVec2 max = { contentMax.x + windowPos.x, contentMax.y + windowPos.y };

    if (const Texture* texture = viewport.getTexture(); texture != nullptr)
    {
        ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)texture->id, min, max,
            { 0, 1 }, { (float)viewportWidth / (float)texture->width, 1.0f - (float)viewportHeight / (float)texture->height });
    }

    viewportWidth = std::max(1, (int)(contentMax.x - contentMin.x));
    viewportHeight = std::max(1, (int)(contentMax.y - contentMin.y));

    viewportResolutionInvRatio = std::max(viewportResolutionInvRatio, 1.0f);
    viewportWidth = (int)((float)viewportWidth / viewportResolutionInvRatio);
    viewportHeight = (int)((float)viewportHeight / viewportResolutionInvRatio);
    viewportFocused = ImGui::IsMouseHoveringRect(min, max) || ImGui::IsWindowFocused();

    ImGui::End();
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
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Environment Color") && beginProperties("##Environment Color"))
    {
        if (bakeParams.targetEngine == TargetEngine::HE2)
        {
            Color3 environmentColor = bakeParams.environmentColor.pow(1.0f / 2.2f);

            if (property("Color", environmentColor))
                bakeParams.environmentColor = environmentColor.pow(2.2f);

            property("Exposure", ImGuiDataType_Float, &bakeParams.exposure);
        }
        else
        {
            property("Color", bakeParams.environmentColor);
        }

        endProperties();
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Light Sampling") && beginProperties("##Light Sampling"))
    {
        property("Light Bounce Count", ImGuiDataType_U32, &bakeParams.lightBounceCount);
        property("Light Sample Count", ImGuiDataType_U32, &bakeParams.lightSampleCount);
        property("Russian Roulette Max Depth", ImGuiDataType_U32, &bakeParams.russianRouletteMaxDepth);
        endProperties();
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Shadow Sampling") && beginProperties("##Shadow Sampling"))
    {
        property("Shadow Sample Count", ImGuiDataType_U32, &bakeParams.shadowSampleCount);
        property("Shadow Search Radius", ImGuiDataType_Float, &bakeParams.shadowSearchRadius);
        property("Shadow Bias", ImGuiDataType_Float, &bakeParams.shadowBias);
        endProperties();
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Ambient Occlusion") && beginProperties("##Ambient Occlusion"))
    {
        property("AO Sample Count", ImGuiDataType_U32, &bakeParams.aoSampleCount);
        property("AO Fade Constant", ImGuiDataType_Float, &bakeParams.aoFadeConstant);
        property("AO Fade Linear", ImGuiDataType_Float, &bakeParams.aoFadeLinear);
        property("AO Fade Quadratic", ImGuiDataType_Float, &bakeParams.aoFadeQuadratic);
        property("AO Fade Strength", ImGuiDataType_Float, &bakeParams.aoStrength);
        endProperties();
    }
    ImGui::Separator();

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
        ImGui::Separator();
    }

    if (beginProperties("##Baking Factory Settings"))
    {
        char outputDirPath[1024];
        strcpy(outputDirPath, outputDirectoryPath.c_str());

        if (property("Output Directory", outputDirPath, sizeof(outputDirPath)))
            outputDirectoryPath = outputDirPath;

        endProperties();
        ImGui::Separator();
    }

    if (ImGui::Button("Browse"))
    {
        if (const std::string newOutputDirectoryPath = FileDialog::openFolder(L"Open Output Folder"); !newOutputDirectoryPath.empty())
            outputDirectoryPath = newOutputDirectoryPath;
    }

    ImGui::SameLine();
    if (ImGui::Button("Open in Explorer"))
    {
        std::filesystem::create_directory(outputDirectoryPath);
        std::system(("explorer \"" + outputDirectoryPath + "\"").c_str());
    }

    ImGui::Separator();
    if (ImGui::Button("Bake"))
    {
        futureBake = std::async(std::launch::async, [&]()
        {
            bake();
        });
    }

    ImGui::End();
}

void Application::setTitle()
{
    if (titleUpdateTime < 0.5f)
    {
        titleUpdateTime += elapsedTime;
        return;
    }
    titleUpdateTime = 0.0f;

    char title[256];

    const int fps = (int)round(1.0f / elapsedTime);

    if (!stageName.empty())
        sprintf(title, "Hedge GI - %s - %s (FPS: %d)", stageName.c_str(), GAME_NAMES[(size_t)game], fps);
    else
        sprintf(title, "Hedge GI (FPS: %d)", fps);

    glfwSetWindowTitle(window, title);
}

void Application::loadProperties()
{
    camera.load(propertyBag);
    bakeParams.load(propertyBag);
    viewportResolutionInvRatio = propertyBag.get("viewportResolutionInvRatio", 2.0f);
    outputDirectoryPath = propertyBag.getString("outputDirectoryPath", stageDirectoryPath + "-HedgeGI");
    mode = propertyBag.get("mode", BakingFactoryMode::GI);

    if (game == Game::Forces)
        bakeParams.targetEngine = TargetEngine::HE2;
}

void Application::storeProperties()
{
    camera.store(propertyBag);
    bakeParams.store(propertyBag);
    propertyBag.set("viewportResolutionInvRatio", viewportResolutionInvRatio);
    propertyBag.setString("outputDirectoryPath", outputDirectoryPath);
    propertyBag.set("mode", mode);
}

void Application::destroyScene()
{
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
    if (openBakingPopup)
        ImGui::OpenPopup("Baking");

    if (!ImGui::BeginPopupModal("Baking", nullptr, ImGuiWindowFlags_Static))
        return;

    ImGui::Text(cancelBake ? "Cancelling..." : "Baking...");

    if (mode == BakingFactoryMode::GI || bakeParams.targetEngine == TargetEngine::HE2)
    {
        const Instance* const lastBakedInstance = this->lastBakedInstance;
        const SHLightField* const lastBakedShlf = this->lastBakedShlf;
        const size_t bakeProgress = this->bakeProgress;

        if (mode == BakingFactoryMode::GI)
        {
            char overlay[1024];
            if (lastBakedInstance != nullptr)
            {
                const uint16_t resolution = bakeParams.resolutionOverride > 0 ? bakeParams.resolutionOverride : propertyBag.get(lastBakedInstance->name + ".resolution", 256);
                sprintf(overlay, "%s (%dx%d)", lastBakedInstance->name.c_str(), resolution, resolution);
            }

            ImGui::Separator();
            ImGui::ProgressBar(bakeProgress / ((float)scene->instances.size() + 1), { 0, 0 }, lastBakedInstance ? overlay : nullptr);
        }
        else if (mode == BakingFactoryMode::LightField)
        {
            char overlay[1024];
            if (lastBakedShlf != nullptr)
                sprintf(overlay, "%s (%dx%dx%d)", lastBakedShlf->name.c_str(), lastBakedShlf->resolution.x(), lastBakedShlf->resolution.y(), lastBakedShlf->resolution.z());

            ImGui::Separator();
            ImGui::ProgressBar(bakeProgress / ((float)scene->shLightFields.size() + 1), { 0, 0 }, lastBakedShlf ? overlay : nullptr);
        }
    }
    else if (mode == BakingFactoryMode::LightField)
    {
        std::lock_guard lock(logMutex);

        ImGui::Separator();
        ImGui::BeginListBox("##Logs");

        for (auto& log : logs)
            ImGui::TextUnformatted(log.c_str());

        ImGui::EndListBox();
    }

    ImGui::Separator();
    if (ImGui::Button("Cancel"))
        cancelBake = true;

    ImGui::EndPopup();
}

void Application::bake()
{
    openBakingPopup = true;
    bakeProgress = 0;
    lastBakedInstance = nullptr;
    lastBakedShlf = nullptr;
    cancelBake = false;

    {
        std::lock_guard lock(logMutex);
        logs.clear();
    }

    std::filesystem::create_directory(outputDirectoryPath);

    if (mode == BakingFactoryMode::GI)
        bakeGI();

    else if (mode == BakingFactoryMode::LightField)
        bakeLightField();

    alert();
    openBakingPopup = false;
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
        skip |= std::filesystem::exists(lightMapFileName);
    
        if (!shadowMapFileName.empty())
        {
            shadowMapFileName = outputDirectoryPath + "/" + shadowMapFileName;
            skip |= std::filesystem::exists(shadowMapFileName);
        }
    
        if (skip)
        {
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

        Logger::log("Saving...\n");

        TRY_CANCEL()

        lightField->save(outputDirectoryPath + "/light-field.lft");
    }
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

    openLoadingPopup = true;

    futureScene = std::async(std::launch::async, [this, directoryPath]()
    {
        stageName = getFileNameWithoutExtension(directoryPath);
        stageDirectoryPath = directoryPath;
        addRecentStage(directoryPath);
        game = detectGameFromStageDirectory(stageDirectoryPath);

        propertyBag.load(directoryPath + "/" + stageName + ".hgi");
        loadProperties();

        std::unique_ptr<Scene> scene = SceneFactory::create(directoryPath);

        openLoadingPopup = false;
        return scene;
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
    return scene->sceneEffect;
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

    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED) && !openLoadingPopup && !openBakingPopup)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }

    const double glfwTime = glfwGetTime();
    elapsedTime = (float)(glfwTime - currentTime);
    currentTime = glfwTime;

    glfwGetWindowSize(window, &width, &height);

    // Viewport
    if (viewportVisible && scene && showViewport)
    {
        camera.update(*this);

        // Halven viewport resolution if we are moving
        if (camera.hasChanged())
        {
            viewportWidth /= 2;
            viewportHeight /= 2;
        }

        viewport.update(*this);

        dirty = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    glClearColor(0.22f, 0.22f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // GUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(font);
    {
        draw();
    }
    ImGui::PopFont();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    input.postUpdate();

    glfwSwapBuffers(window);
    setTitle();
}
