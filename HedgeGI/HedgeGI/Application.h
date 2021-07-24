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
    static std::string TEMP_FILE_PATH;

    GLFWwindow* window{};
    Input input;
    Camera camera;
    Viewport viewport;
    const Quad quad;

    std::vector<std::string> logs;
    std::mutex logMutex;

    struct ImFont* font {};
    std::string imGuiIniPath;

    bool docksInitialized {};

    float elapsedTime{};
    double currentTime{};
    float titleUpdateTime {};

    int width {};
    int height {};

    int viewportWidth {};
    int viewportHeight {};
    bool viewportFocused {};
    bool viewportVisible {};

    bool showScene { true };
    bool showViewport { true };
    bool showSettings { true };
    bool showBakingFactory { true };
    bool dirty {};

    bool openLoadingPopup {};
    bool openBakingPopup {};

    std::string stageName;
    std::string stageDirectoryPath;
    std::list<std::string> recentStages;
    Game game {};
    PropertyBag propertyBag;
    std::unique_ptr<Scene> scene;

    std::atomic<bool> loadingScene {};
    std::future<std::unique_ptr<Scene>> futureScene;

    const Instance* selectedInstance {};
    Light* selectedLight {};
    
    float viewportResolutionInvRatio {};
    BakeParams bakeParams;

    BakingFactoryMode mode {};
    std::string outputDirectoryPath;

    std::future<void> futureBake;
    std::atomic<size_t> bakeProgress {};
    std::atomic<const Instance*> lastBakedInstance {};
    std::atomic<const SHLightField*> lastBakedShlf {};
    std::atomic<bool> cancelBake {};

    static GLFWwindow* createGLFWwindow();
    static void logListener(void* owner, const char* text);

    void initializeImGui();
    void initializeStyle();
    void initializeFonts();
    void initializeDocks();

    void draw();
    void drawLoadingPopupUI();
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

    void addRecentStage(const std::string& path);
    void loadRecentStages();
    void saveRecentStages() const;

    void drawBakingPopupUI();
    void bake();
    void bakeGI();
    void bakeLightField();

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

    void loadScene(const std::string& directoryPath);

    PropertyBag& getPropertyBag();
    const RaytracingContext getRaytracingContext() const;
    const BakeParams getBakeParams() const;

    void drawQuad() const;
    
    void run();
    void update();
};
