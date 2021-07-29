#pragma once

class Application;
class Bitmap;
class Camera;
class FramebufferTexture;
class ShaderProgram;
class Texture;

class Viewport
{
    std::unique_ptr<Bitmap> bitmap;
    size_t progress {};
    size_t previousViewportWidth {};
    size_t previousViewportHeight {};
    float normalizedWidth {};
    float normalizedHeight {};

    const ShaderProgram& copyTexShader;
    const ShaderProgram& toneMapShader;

    std::unique_ptr<FramebufferTexture> hdrFramebufferTex;
    std::unique_ptr<FramebufferTexture> ldrFramebufferTex;

    std::unique_ptr<FramebufferTexture> avgLuminanceFramebufferTex;

    void initialize();
    void validate(const Application& application);
    void pathTrace(const Application& application);
    void copy(const Application& application) const;
    void toneMap(const Application& application) const;

public:
    Viewport();

    void update(const Application& application);

    const Texture* getTexture() const;
};
