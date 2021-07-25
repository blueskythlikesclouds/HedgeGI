#include "Viewport.h"
#include "Application.h"
#include "Camera.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "ShaderProgram.h"

void Viewport::initialize()
{
    avgLuminanceFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA32F, 256, 256, GL_RGBA, GL_FLOAT);
    avgLuminanceFramebufferTex->bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 8);
}

void Viewport::validate(const Application& application)
{
    const size_t viewportWidth = application.getViewportWidth();
    const size_t viewportHeight = application.getViewportHeight();

    if (bitmap == nullptr || bitmap->width < viewportWidth || bitmap->height < viewportHeight)
    {
        const size_t bitmapWidth = bitmap != nullptr ? std::max<size_t>(bitmap->width, viewportWidth) : viewportWidth;
        const size_t bitmapHeight = bitmap != nullptr ? std::max<size_t>(bitmap->height, viewportHeight) : viewportHeight;

        bitmap = std::make_unique<Bitmap>((uint32_t)bitmapWidth, (uint32_t)bitmapHeight);
        hdrFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA32F, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT);
        ldrFramebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA8, bitmap->width, bitmap->height, GL_RGBA, GL_UNSIGNED_BYTE);
        progress = 0;
    }

    else if (previousViewportWidth != viewportWidth || previousViewportHeight != viewportHeight ||
        application.isDirty() || application.getCamera().hasChanged())
    {
        bitmap->clear();
        progress = 0;
    }

    normalizedWidth = viewportWidth / (float)bitmap->width;
    normalizedHeight = viewportHeight / (float)bitmap->height;

    previousViewportWidth = viewportWidth;
    previousViewportHeight = viewportHeight;
}

void Viewport::pathTrace(const Application& application)
{
    const size_t viewportWidth = application.getViewportWidth();
    const size_t viewportHeight = application.getViewportHeight();
    const Camera& camera = application.getCamera();

    BakingFactory::bake(application.getRaytracingContext(), *bitmap, viewportWidth, viewportHeight, camera, application.getBakeParams(), progress++);
    hdrFramebufferTex->texture.subImage(0, (GLint)(bitmap->height - viewportHeight), (GLsizei)viewportWidth, (GLsizei)viewportHeight, GL_RGBA, GL_FLOAT, bitmap->data.get());
}

void Viewport::copy(const Application& application) const
{
    copyTexShader.use();
    copyTexShader.set("uRect", Vector4(0, 1 - normalizedHeight, normalizedWidth, normalizedHeight));
    copyTexShader.set("uTexture", 0);

    avgLuminanceFramebufferTex->bind();
    hdrFramebufferTex->texture.bind(0);

    application.drawQuad();

    avgLuminanceFramebufferTex->texture.bind();
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Viewport::toneMap(const Application& application) const
{
    const size_t viewportWidth = application.getViewportWidth();
    const size_t viewportHeight = application.getViewportHeight();

    toneMapShader.use();
    toneMapShader.set("uRect", Vector4(0, 1 - normalizedHeight, normalizedWidth, normalizedHeight));
    toneMapShader.set("uTexture", 0);
    toneMapShader.set("uAvgLuminanceTex", 1);
    toneMapShader.set("uApplyGamma", application.getBakeParams().targetEngine == TargetEngine::HE2);

    ldrFramebufferTex->bind();
    hdrFramebufferTex->texture.bind(0);
    avgLuminanceFramebufferTex->texture.bind(1);

    glViewport(0, ldrFramebufferTex->height - viewportHeight, viewportWidth, viewportHeight);
    application.drawQuad();
}

Viewport::Viewport() :
    copyTexShader(ShaderProgram::get("CopyTexture")),
    toneMapShader(ShaderProgram::get("ToneMap"))
{
    initialize();
}

void Viewport::update(const Application& application)
{
    validate(application);
    pathTrace(application);
    copy(application);
    toneMap(application);
}

const Texture* Viewport::getTexture() const
{
    return ldrFramebufferTex ? &ldrFramebufferTex->texture : nullptr;
}
