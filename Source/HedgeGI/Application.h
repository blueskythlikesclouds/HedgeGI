#pragma once

#include "Input.h"
#include "Camera.h"
#include "Viewport.h"
#include "Quad.h"
#include "Game.h"
#include "PropertyBag.h"
#include "Scene.h"
#include "BakeParams.h"

enum class BakingFactoryMode
{
    GI,
    LightField
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

    float bakeElapsedTime {};
    double bakeCurrentTime {};

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

    static bool beginProperties(const char* name);
    static void beginProperty(const char* label);
    static bool property(const char* label, enum ImGuiDataType_ dataType, void* data);
    static bool property(const char* label, bool& data);
    static bool property(const char* label, Color3& data);
    static bool property(const char* label, char* data, size_t dataSize);
    template<typename T> static bool property(const char* label, const std::initializer_list<std::pair<const char*, T>>& values, T& data);
    static void endProperties();

    void draw();
    void drawLoadingPopupUI();
    void drawSceneUI();
    void drawInstancesUI();
    void drawLightsUI();
    void drawViewportUI();
    void drawSettingsUI();
    void drawBakingFactoryUI();
    void setTitle(float fps);

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

    bool isViewportFocused() const;

    bool isDirty() const;

    void loadScene(const std::string& directoryPath);

    Game getGame() const;
    PropertyBag& getPropertyBag();
    const RaytracingContext getRaytracingContext() const;
    const SceneEffect& getSceneEffect() const;
    const BakeParams getBakeParams() const;

    void drawQuad() const;
    
    void run();
    void update();
};