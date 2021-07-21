#include "Viewport.h"
#include "Application.h"
#include "Camera.h"
#include "Framebuffer.h"

void Viewport::update(const Application& application)
{
    if (bitmap == nullptr || bitmap->width != application.getViewportWidth() || bitmap->height != application.getViewportHeight())
    {
        bitmap = std::make_unique<Bitmap>(application.getViewportWidth(), application.getViewportHeight());
        framebufferTex = std::make_unique<FramebufferTexture>(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT);
    }

    else if (application.isDirty() || application.getCamera().hasChanged())
    {
        bitmap->clear();
    }

    BakingFactory::bake(application.getRaytracingContext(), *bitmap, application.getCamera(), application.getBakeParams());
    framebufferTex->texture.subImage(0, 0, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT, bitmap->data.get());
}

const FramebufferTexture& Viewport::getFramebufferTexture() const
{
    return *framebufferTex;
}
