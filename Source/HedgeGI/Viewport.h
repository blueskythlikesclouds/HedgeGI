#pragma once

#include "BakeParams.h"
#include "CameraController.h"
#include "Quad.h"
#include "Scene.h"

class Bitmap;
class FramebufferTexture;
class ShaderProgram;
class Texture;

class Viewport final : public Component
{
    const Quad quad;

    std::unique_ptr<Bitmap> bitmap;
    size_t progress{};

    const ShaderProgram& copyTexShader;
    const ShaderProgram& toneMapShader;

    std::unique_ptr<FramebufferTexture> hdrFramebufferTex;
    std::unique_ptr<FramebufferTexture> ldrFramebufferTex;

    std::unique_ptr<FramebufferTexture> avgLuminanceFramebufferTex;

    const Bitmap* rgbTable{};
    std::unique_ptr<Texture> rgbTableTex;

    struct BakeArgs
    {
        std::atomic<bool> baking;
        RaytracingContext raytracingContext;
        size_t bakeWidth{};
        size_t bakeHeight{};
        Camera camera;
        BakeParams bakeParams;
        bool antiAliasing{};
        std::atomic<bool> end;
    } bakeArgs{};

    float normalizedWidth{};
    float normalizedHeight{};

    double currentTime{};
    size_t frameRate{};
    double frameRateUpdateTime{};

    std::thread bakeThread;

    void bakeThreadFunc();

    void drawQuad() const;

    void validate();
    void copy();
    void toneMap();
    void notifyBakeThread();

public:
    Viewport();
    ~Viewport() override;

    void initialize() override;
    void update(float deltaTime) override;

    const Texture* getInitialTexture() const;
    const Texture* getFinalTexture() const;

    const Camera& getCamera() const;

    float getNormalizedWidth() const;
    float getNormalizedHeight() const;

    size_t getFrameRate() const;

    bool isBaking() const;
    void waitForBake() const;
};