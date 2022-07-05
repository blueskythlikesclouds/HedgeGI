#include "Viewport.h"
#include "BakingFactory.h"
#include "Bitmap.h"
#include "Framebuffer.h"
#include "ShaderProgram.h"
#include "Stage.h"
#include "StageParams.h"
#include "Texture.h"
#include "ViewportWindow.h"

void Viewport::bakeThreadFunc()
{
    while (!bakeArgs.end)
    {
        if (!bakeArgs.baking)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        BakingFactory::bake(bakeArgs.raytracingContext, *bitmap, bakeArgs.bakeWidth, bakeArgs.bakeHeight, bakeArgs.camera, bakeArgs.bakeParams, progress++, bakeArgs.antiAliasing);
        bakeArgs.baking = false;
    }
}

void Viewport::drawQuad() const
{
    quad.vertexArray.bind();
    quad.elementArray.bind();
    quad.elementArray.draw();
}

void Viewport::initialize()
{
    avgLuminanceFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_R32F, 256, 256, GL_RED, GL_FLOAT);
    avgLuminanceFramebufferTex->bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 8);

    bakeThread = std::thread(&Viewport::bakeThreadFunc, this);
}

void Viewport::validate()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();
    const auto viewportWindow = get<ViewportWindow>();

    const size_t bakeWidth = viewportWindow->getBakeWidth();
    const size_t bakeHeight = viewportWindow->getBakeHeight();

    if (bitmap == nullptr || bitmap->width < bakeWidth || bitmap->height < bakeHeight)
    {
        const size_t bitmapWidth = bitmap != nullptr ? std::max<size_t>(bitmap->width, bakeWidth) : bakeWidth;
        const size_t bitmapHeight = bitmap != nullptr ? std::max<size_t>(bitmap->height, bakeHeight) : bakeHeight;

        bitmap = std::make_unique<Bitmap>(bitmapWidth, bitmapHeight);
        hdrFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA32F, (GLsizei)bitmap->width, (GLsizei)bitmap->height, GL_RGBA, GL_FLOAT);
        ldrFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA8, (GLsizei)bitmap->width, (GLsizei)bitmap->height, GL_RGBA, GL_UNSIGNED_BYTE);
        progress = 0;
    }

    else if (bakeArgs.bakeWidth != bakeWidth || bakeArgs.bakeHeight != bakeHeight || params->dirty)
        progress = 0;

    if (params->dirtyBVH)
        stage->getScene()->createLightBVH(true);

    bakeArgs.antiAliasing = !params->dirty; // Random jitter is very eye-killing when moving/modifying values

    const auto& sceneRgbTable = stage->getScene()->rgbTable;

    if (rgbTable != sceneRgbTable.get())
    {
        rgbTable = sceneRgbTable.get();
        rgbTableTex = rgbTable ? std::make_unique<Texture>(
            GL_TEXTURE_2D,
            rgbTable->format == BitmapFormat::U8 ? GL_RGBA8 : GL_RGBA32F, 
            (GLsizei)rgbTable->width, 
            (GLsizei)rgbTable->height,
            GL_RGBA, 
            rgbTable->format == BitmapFormat::U8 ? GL_UNSIGNED_BYTE : GL_FLOAT, 
            rgbTable->data) : nullptr;
    }

    params->dirty = false;
    params->dirtyBVH = false;
}

void Viewport::copy()
{
    hdrFramebufferTex->texture.subImage(0, (GLint)(bitmap->height - bakeArgs.bakeHeight), (GLsizei)bakeArgs.bakeWidth, (GLsizei)bakeArgs.bakeHeight, GL_RGBA, GL_FLOAT, bitmap->data);

    const SceneEffect& sceneEffect = get<Stage>()->getScene()->effect;

    copyTexShader.use();
    copyTexShader.set("uRect", Vector4(0, 1 - normalizedHeight, normalizedWidth, normalizedHeight));
    copyTexShader.set("uLumMinMax", Vector2(sceneEffect.hdr.lumMin, sceneEffect.hdr.lumMax));
    copyTexShader.set("uTexture", 0);

    avgLuminanceFramebufferTex->bind();
    hdrFramebufferTex->texture.bind(0);

    drawQuad();

    avgLuminanceFramebufferTex->texture.bind();
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Viewport::toneMap()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    const float middleGray = stage->getScene()->effect.hdr.middleGray;

    toneMapShader.use();
    toneMapShader.set("uRect", Vector4(0, 1 - normalizedHeight, normalizedWidth, normalizedHeight));
    toneMapShader.set("uMiddleGray", stage->getGameType() == GameType::Forces ? middleGray * 0.5f : middleGray);
    toneMapShader.set("uTexture", 0);
    toneMapShader.set("uAvgLuminanceTex", 1);
    toneMapShader.set("uGamma",
        params->bakeParams.targetEngine == TargetEngine::HE2 ? 1.0f / 2.2f :
        stage->getGameType() == GameType::Generations && params->gammaCorrectionFlag ? 1.25f :
        1.0f);

    const bool enableRgbTable = params->bakeParams.targetEngine == TargetEngine::HE2 && params->colorCorrectionFlag && rgbTable != nullptr;

    toneMapShader.set("uEnableRgbTable", enableRgbTable);

    if (enableRgbTable)
    {
        toneMapShader.set("uRgbTableTex", 2);
        rgbTableTex->bind(2);
    }

    ldrFramebufferTex->bind();
    hdrFramebufferTex->texture.bind(0);
    avgLuminanceFramebufferTex->texture.bind(1);

    glViewport(0, (GLint)(ldrFramebufferTex->height - bakeArgs.bakeHeight), (GLsizei)bakeArgs.bakeWidth, (GLsizei)bakeArgs.bakeHeight);
    drawQuad();
}

void Viewport::notifyBakeThread()
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();
    const auto camera = get<CameraController>();
    const auto viewportWindow = get<ViewportWindow>();

    bakeArgs.raytracingContext = stage->getScene()->getRaytracingContext();
    bakeArgs.bakeWidth = viewportWindow->getBakeWidth();
    bakeArgs.bakeHeight = viewportWindow->getBakeHeight();
    bakeArgs.camera = *static_cast<Camera*>(camera);
    bakeArgs.bakeParams = params->bakeParams;
    bakeArgs.baking = true;
}

Viewport::Viewport() :
    copyTexShader(ShaderProgram::get("CopyTexture")),
    toneMapShader(ShaderProgram::get("ToneMap"))
{
}

Viewport::~Viewport()
{
    bakeArgs.end = true;
    bakeThread.join();
}

void Viewport::update(float deltaTime)
{
    const auto viewportWindow = get<ViewportWindow>();

    if (!viewportWindow->show || bakeArgs.baking)
        return;

    const double time = glfwGetTime();
    const double viewportDeltaTime = time - currentTime;
    
    if ((frameRateUpdateTime += viewportDeltaTime) > 0.5)
        frameRate = (size_t)lround(1.0 / std::max((double)deltaTime, viewportDeltaTime)), frameRateUpdateTime = 0.0;

    currentTime = time;

    if (bitmap != nullptr)
    {
        normalizedWidth = bakeArgs.bakeWidth / (float)bitmap->width;
        normalizedHeight = bakeArgs.bakeHeight / (float)bitmap->height;

        copy();
        toneMap();
    }

    validate();
    notifyBakeThread();
}

const Texture* Viewport::getInitialTexture() const
{
    return hdrFramebufferTex ? &hdrFramebufferTex->texture : nullptr;
}

const Texture* Viewport::getFinalTexture() const
{
    return ldrFramebufferTex ? &ldrFramebufferTex->texture : nullptr;
}

const Camera& Viewport::getCamera() const
{
    return bakeArgs.camera;
}

float Viewport::getNormalizedWidth() const
{
    return normalizedWidth;
}

float Viewport::getNormalizedHeight() const
{
    return normalizedHeight;
}

size_t Viewport::getFrameRate() const
{
    return frameRate;
}

bool Viewport::isBaking() const
{
    return bakeArgs.baking;
}

void Viewport::waitForBake() const
{
    while (bakeArgs.baking)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
