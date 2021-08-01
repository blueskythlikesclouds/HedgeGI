#pragma once

#include "BakeParams.h"
#include "Camera.h"
#include "Scene.h"

class Application;
class Bitmap;
class FramebufferTexture;
class ShaderProgram;
class Texture;

class Viewport
{
    std::unique_ptr<Bitmap> bitmap;
    size_t progress {};

    const ShaderProgram& copyTexShader;
    const ShaderProgram& toneMapShader;

    std::unique_ptr<FramebufferTexture> hdrFramebufferTex;
    std::unique_ptr<FramebufferTexture> ldrFramebufferTex;

    std::unique_ptr<FramebufferTexture> avgLuminanceFramebufferTex;

    struct BakeArgs
    {
        std::atomic<bool> baking;
        RaytracingContext raytracingContext;
        size_t viewportWidth{};
        size_t viewportHeight{};
        Camera camera;
        BakeParams bakeParams;
        std::atomic<bool> end;
    } bakeArgs{};

    float normalizedWidth {};
    float normalizedHeight {};

    std::thread bakeThread;

    void bakeThreadFunc();

    void initialize();
    void validate(const Application& application);
    void copy(const Application& application) const;
    void toneMap(const Application& application) const;
    void notifyBakeThread(const Application& application);

public:
    Viewport();
    ~Viewport();

    void update(const Application& application);

    const Texture* getTexture() const;

    float getNormalizedWidth() const;
    float getNormalizedHeight() const;
    bool isBaking() const;
};
