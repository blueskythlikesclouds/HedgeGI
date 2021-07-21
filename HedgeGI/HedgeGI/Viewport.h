#pragma once

class Input;
class Camera;
struct BakeParams;
struct RaytracingContext;
class Bitmap;

class Viewport
{
    GLFWwindow* window;

    std::unique_ptr<Input> input;
    std::unique_ptr<Camera> camera;

    std::unique_ptr<Bitmap> bitmap;
    std::unique_ptr<Color4i[]> pixels;

    float previousAverageLuminance;

    double time;
    float elapsedTime;
    bool focused;

    bool enableBakeParamsWindow;

    GLuint texture;
    GLuint framebuffer;

public:
    Viewport();
    ~Viewport();

    void update(const RaytracingContext& raytracingContext, BakeParams& bakeParams);
    bool isOpen() const;
};
