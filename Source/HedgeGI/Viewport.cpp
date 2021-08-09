#include "Viewport.h"

#include "Application.h"
#include "BakingFactory.h"
#include "Bitmap.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "ShaderProgram.h"
#include "Texture.h"

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

void Viewport::initialize()
{
    avgLuminanceFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_R32F, 256, 256, GL_RED, GL_FLOAT);
    avgLuminanceFramebufferTex->bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 8);
}

void Viewport::validate(const Application& application)
{
    const size_t bakeWidth = application.getBakeWidth();
    const size_t bakeHeight = application.getBakeHeight();

    if (bitmap == nullptr || bitmap->width < bakeWidth || bitmap->height < bakeHeight)
    {
        const size_t bitmapWidth = bitmap != nullptr ? std::max<size_t>(bitmap->width, bakeWidth) : bakeWidth;
        const size_t bitmapHeight = bitmap != nullptr ? std::max<size_t>(bitmap->height, bakeHeight) : bakeHeight;

        bitmap = std::make_unique<Bitmap>((uint32_t)bitmapWidth, (uint32_t)bitmapHeight);
        hdrFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA32F, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT);
        ldrFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA8, bitmap->width, bitmap->height, GL_RGBA, GL_UNSIGNED_BYTE);
        progress = 0;
    }

    else if (bakeArgs.bakeWidth != bakeWidth || bakeArgs.bakeHeight != bakeHeight || application.isDirty())
        progress = 0;

    if (application.isDirtyBVH())
        application.getScene().createLightBVH(true);

    bakeArgs.antiAliasing = !application.isDirty(); // Random jitter is very eye-killing when moving/modifying values
}

void Viewport::copy(const Application& application) const
{
    hdrFramebufferTex->texture.subImage(0, (GLint)(bitmap->height - bakeArgs.bakeHeight), (GLsizei)bakeArgs.bakeWidth, (GLsizei)bakeArgs.bakeHeight, GL_RGBA, GL_FLOAT, bitmap->data.get());

    const SceneEffect& sceneEffect = application.getSceneEffect();

    copyTexShader.use();
    copyTexShader.set("uRect", Vector4(0, 1 - normalizedHeight, normalizedWidth, normalizedHeight));
    copyTexShader.set("uLumMinMax", Vector2(sceneEffect.hdr.lumMin, sceneEffect.hdr.lumMax));
    copyTexShader.set("uTexture", 0);

    avgLuminanceFramebufferTex->bind();
    hdrFramebufferTex->texture.bind(0);

    application.drawQuad();

    avgLuminanceFramebufferTex->texture.bind();
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Viewport::toneMap(const Application& application) const
{
    const float middleGray = application.getSceneEffect().hdr.middleGray;

    toneMapShader.use();
    toneMapShader.set("uRect", Vector4(0, 1 - normalizedHeight, normalizedWidth, normalizedHeight));
    toneMapShader.set("uMiddleGray", application.getGame() == Game::Forces ? middleGray * 0.5f : middleGray);
    toneMapShader.set("uTexture", 0);
    toneMapShader.set("uAvgLuminanceTex", 1);
    toneMapShader.set("uGamma",
        application.getBakeParams().targetEngine == TargetEngine::HE2 ? 1.0f / 2.2f :
        application.getGame() == Game::Generations && application.getGammaCorrectionFlag() ? 1.5f :
        1.0f);

    ldrFramebufferTex->bind();
    hdrFramebufferTex->texture.bind(0);
    avgLuminanceFramebufferTex->texture.bind(1);

    glViewport(0, (GLint)(ldrFramebufferTex->height - bakeArgs.bakeHeight), (GLsizei)bakeArgs.bakeWidth, (GLsizei)bakeArgs.bakeHeight);
    application.drawQuad();
}

void Viewport::notifyBakeThread(const Application& application)
{
    bakeArgs.raytracingContext = application.getRaytracingContext();
    bakeArgs.bakeWidth = application.getBakeWidth();
    bakeArgs.bakeHeight = application.getBakeHeight();
    bakeArgs.camera = application.getCamera();
    bakeArgs.bakeParams = application.getBakeParams();
    bakeArgs.baking = true;
}

Viewport::Viewport() :
    copyTexShader(ShaderProgram::get("CopyTexture")),
    toneMapShader(ShaderProgram::get("ToneMap")),
    bakeThread(&Viewport::bakeThreadFunc, this)
{
    initialize();
}

Viewport::~Viewport()
{
    bakeArgs.end = true;
    bakeThread.join();
}

void Viewport::update(const Application& application)
{
    if (bakeArgs.baking)
        return;

    if (bitmap != nullptr)
    {
        normalizedWidth = bakeArgs.bakeWidth / (float)bitmap->width;
        normalizedHeight = bakeArgs.bakeHeight / (float)bitmap->height;

        copy(application);
        toneMap(application);
    }

    validate(application);
    notifyBakeThread(application);
}

const Texture* Viewport::getTexture() const
{
    return ldrFramebufferTex ? &ldrFramebufferTex->texture : nullptr;
}

float Viewport::getNormalizedWidth() const
{
    return normalizedWidth;
}

float Viewport::getNormalizedHeight() const
{
    return normalizedHeight;
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
