#pragma once

class Texture;
class Application;
class Bitmap;
class Camera;

class Viewport
{
    std::unique_ptr<Bitmap> bitmap;
    std::unique_ptr<Texture> texture;
    size_t progress {};
    size_t previousViewportWidth {};
    size_t previousViewportHeight {};

public:
    void update(const Application& application);

    const Texture* getTexture() const;
};
