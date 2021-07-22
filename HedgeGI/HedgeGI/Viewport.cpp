#include "Viewport.h"
#include "Application.h"
#include "Camera.h"
#include "Texture.h"

void Viewport::update(const Application& application)
{
    const size_t viewportWidth = application.getViewportWidth();
    const size_t viewportHeight = application.getViewportHeight();
    const Camera& camera = application.getCamera();

    if (bitmap == nullptr || bitmap->width < viewportWidth || bitmap->height < viewportHeight)
    {
        const size_t bitmapWidth = bitmap != nullptr ? std::max<size_t>(bitmap->width, viewportWidth) : viewportWidth;
        const size_t bitmapHeight = bitmap != nullptr ? std::max<size_t>(bitmap->height, viewportHeight) : viewportHeight;

        bitmap = std::make_unique<Bitmap>(bitmapWidth, bitmapHeight);
        texture = std::make_unique<Texture>(GL_TEXTURE_2D, GL_RGBA, bitmap->width, bitmap->height, GL_RGBA, GL_FLOAT);
        progress = 0;
    }

    else if (previousViewportWidth != viewportWidth || previousViewportHeight != viewportHeight || 
        application.isDirty() || application.getCamera().hasChanged())
    {
        bitmap->clear();
        progress = 0;
    }

    BakingFactory::bake(application.getRaytracingContext(), *bitmap, viewportWidth, viewportHeight, camera, application.getBakeParams(), progress++);
    texture->subImage(0, bitmap->height - viewportHeight, viewportWidth, viewportHeight, GL_RGBA, GL_FLOAT, bitmap->data.get());

    previousViewportWidth = viewportWidth;
    previousViewportHeight = viewportHeight;
}

const Texture* Viewport::getTexture() const
{
    return texture.get();
}
