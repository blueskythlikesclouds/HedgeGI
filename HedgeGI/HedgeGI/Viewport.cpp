#include "Viewport.h"
#include "Application.h"
#include "Camera.h"
#include "Texture.h"

void Viewport::update(const Application& application)
{
    if (bitmap == nullptr || bitmap->width != application.getViewportWidth() || bitmap->height != application.getViewportHeight())
    {
        bitmap = std::make_unique<Bitmap>(application.getViewportWidth(), application.getViewportHeight());
        texture = std::make_unique<Texture>(GL_TEXTURE_2D, GL_RGBA, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT);
        progress = 0;
    }

    else if (application.isDirty() || application.getCamera().hasChanged())
    {
        bitmap->clear();
        progress = 0;
    }

    BakingFactory::bake(application.getRaytracingContext(), *bitmap, application.getCamera(), application.getBakeParams(), progress++);
    texture->subImage(0, 0, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT, bitmap->data.get());
}

const Texture* Viewport::getTexture() const
{
    return texture.get();
}
