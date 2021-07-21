#pragma once

class FramebufferTexture;
class Application;
class Bitmap;
class Camera;

class Viewport
{
    std::unique_ptr<Bitmap> bitmap;
    std::unique_ptr<FramebufferTexture> framebufferTex;

public:
    void update(const Application& application);

    const FramebufferTexture& getFramebufferTexture() const;
};
