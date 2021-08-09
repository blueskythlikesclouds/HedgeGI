#pragma once

#include "Input.h"
#include "Camera.h"
#include "Viewport.h"
#include "Quad.h"
#include "Game.h"
#include "PropertyBag.h"
#include "Scene.h"
#include "BakeParams.h"
#include "ModelProcessor.h"

struct ImVec2;
enum class LogType;

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

    std::list<std::pair<LogType, std::string>> logs;
    size_t previousLogSize;
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

    bool showScene { true };
    bool showViewport { true };
    bool showSettings { true };
    bool showBakingFactory { true };
    bool showLogs { true };
    bool dirty {};
    bool dirtyBVH {};

    std::string stageName;
    std::string stageDirectoryPath;
    std::list<std::string> recentStages;
    Game game {};
    PropertyBag propertyBag;
    std::unique_ptr<Scene> scene;

    std::future<std::unique_ptr<Scene>> futureScene;

    const Instance* selectedInstance {};
    Light* selectedLight {};

    float viewportResolutionInvRatio {};
    bool gammaCorrectionFlag {};
    BakeParams bakeParams;

    BakingFactoryMode mode {};
    std::string outputDirectoryPath;

    bool skipExistingFiles { true };
    std::future<void> futureBake;
    std::atomic<size_t> bakeProgress {};
    std::atomic<const Instance*> lastBakedInstance {};
    std::atomic<const SHLightField*> lastBakedShlf {};
    std::atomic<bool> cancelBake {};

    std::future<void> futureProcess;

    static GLFWwindow* createGLFWwindow();
    static void logListener(void* owner, LogType logType, const char* text);

    void clearLogs();

    void initializeImGui();
    void initializeStyle();
    void initializeFonts();
    void initializeDocks();

    static bool beginProperties(const char* name);
    static void beginProperty(const char* label, float width = -1);
    static bool property(const char* label, enum ImGuiDataType_ dataType, void* data);
    static bool property(const char* label, bool& data);
    static bool property(const char* label, Color3& data);
    static bool property(const char* label, char* data, size_t dataSize, float width = -1);
    template<typename T> static bool property(const char* label, const std::initializer_list<std::pair<const char*, T>>& values, T& data);

    static bool dragProperty(const char* label, float& data, float speed = 0.1f, float min = 0, float max = 0);
    static bool dragProperty(const char* label, Vector3& data, float speed = 0.1f, float min = 0, float max = 0);

    static void endProperties();

    void updateViewport();

    void draw();
    void drawLoadingPopupUI();
    void drawSceneUI();
    void drawInstancesUI();
    void drawLightsUI();
    void drawSettingsUI();
    void drawBakingFactoryUI();
    void drawViewportUI();
    bool drawLogsContainerUI(const ImVec2& size);
    void drawLogsUI();
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

    void clean();
    void pack();
    void packLightField();
    void packGenerationsGI();
    void packLostWorldOrForcesGI();

    void drawProcessingPopupUI();
    void process(std::function<void()> function);
    void processStage(ModelProcessor::ProcModelFunc function);

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
    bool isDirtyBVH() const;

    Scene& getScene() const;
    void loadScene(const std::string& directoryPath);

    Game getGame() const;
    PropertyBag& getPropertyBag();
    const RaytracingContext getRaytracingContext() const;
    const SceneEffect& getSceneEffect() const;
    bool getGammaCorrectionFlag() const;
    const BakeParams getBakeParams() const;

    void drawQuad() const;
    
    void run();
    void update();
};