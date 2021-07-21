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

    float elapsedTime{};
    double currentTime{};

    int width {};
    int height {};

    const RaytracingContext& raytracingContext;
    const BakeParams& bakeParams;

    static GLFWwindow* createGLFWwindow();

    void drawFrameRateWindow() const;

public:
    Application(const RaytracingContext& raytracingContext, const BakeParams& bakeParams);
    ~Application();

    const Input& getInput() const;
    const Camera& getCamera() const;
    float getElapsedTime() const;

    int getWidth() const;
    int getHeight() const;

    const RaytracingContext& getRaytracingContext() const;
    const BakeParams& getBakeParams() const;

    void drawQuad() const;
    
    void run();
    void update();
};
