#pragma once

struct BakeParams;
struct RaytracingContext;
class Bitmap;

class Viewport
{
    GLFWwindow* window;

    Eigen::Vector3f cameraPosition;
    Quaternion cameraRotation;

    Eigen::Vector3f previousCameraPosition;
    Quaternion previousCameraRotation;

    std::unique_ptr<Bitmap> bitmap;
    std::unique_ptr<Color4i[]> pixels;

    std::array<bool, GLFW_KEY_LAST + 1> key;
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> mouse;

    double previousCursorX;
    double previousCursorY;

    float previousAverageLuminance;

    double time;
    float elapsedTime;

    bool dirty;
    bool focused;

    bool enableBakeParamsWindow;

    static void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double cursorX, double cursorY);

public:
    Viewport();
    ~Viewport();

    void update(const RaytracingContext& raytracingContext, BakeParams& bakeParams);
    bool isOpen() const;
};
