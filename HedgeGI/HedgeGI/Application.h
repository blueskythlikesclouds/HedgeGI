#pragma once

#include "BakingFactory.h"
#include "Input.h"
#include "Quad.h"
#include "Viewport.h"
#include "Camera.h"

enum BakingFactoryMode
{
    BAKING_FACTORY_MODE_GI,
    BAKING_FACTORY_MODE_LIGHT_FIELD
};

class Application
{
    GLFWwindow* window{};
    Input input;
    Camera camera;
    Viewport viewport;
    const Quad quad;

    ImFont* font {};
    std::string imGuiIniPath;

    float elapsedTime{};
    double currentTime{};
    float titleUpdateTime {};

    int width {};
    int height {};

    int viewportWidth {};
    int viewportHeight {};
    bool viewportFocused {};

    bool showScene { true };
    bool showViewport { true };
    bool showSettings { true };
    bool showBakingFactory { true };
    bool dirty {};

    std::string stageName;
    std::string stageDirectoryPath;
    PropertyBag propertyBag;
    std::unique_ptr<Scene> scene;
    std::future<std::unique_ptr<Scene>> futureScene;

    const Instance* selectedInstance {};
    Light* selectedLight {};
    
    float viewportResolutionInvRatio {};
    BakeParams bakeParams;

    BakingFactoryMode mode {};
    std::string outputDirectoryPath;

    static GLFWwindow* createGLFWwindow();

    void initializeImGui();
    void initializeStyle();
    void initializeFonts();

    void draw();
    void drawSceneUI();
    void drawInstancesUI();
    void drawLightsUI();
    void drawViewportUI();
    void drawSettingsUI();
    void drawBakingFactoryUI();
    void setTitle();

    void loadProperties();
    void storeProperties();
    void destroyScene();

public:
    Application();
    ~Application();

    const Input& getInput() const;
    const Camera& getCamera() const;
    float getElapsedTime() const;

    int getWidth() const;
    int getHeight() const;

    int getViewportWidth() const;
    int getViewportHeight() const;

    bool isDirty() const;

    PropertyBag& getPropertyBag();
    const RaytracingContext getRaytracingContext() const;
    const BakeParams getBakeParams() const;

    void drawQuad() const;
    
    void run();
    void update();
};
