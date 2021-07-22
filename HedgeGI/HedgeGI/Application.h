#pragma once

#include "BakingFactory.h"
#include "Input.h"
#include "Quad.h"
#include "Viewport.h"
#include "Camera.h"

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

    int width {};
    int height {};

    int viewportWidth {};
    int viewportHeight {};

    bool showScene { true };
    bool showViewport { true };
    bool showSettings { true };

    bool dirty {};

    std::unique_ptr<Scene> scene;
    std::future<std::unique_ptr<Scene>> futureScene;

    PropertyBag propertyBag;
    std::string propertyBagFilePath;

    float viewportResolutionInvRatio {};
    BakeParams bakeParams;

    static GLFWwindow* createGLFWwindow();

    void initializeUI();

    void draw();
    void drawFPS(float y) const;

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

    const RaytracingContext getRaytracingContext() const;
    const BakeParams getBakeParams() const;

    PropertyBag& getPropertyBag();

    void drawQuad() const;
    
    void run();
    void update();
};
