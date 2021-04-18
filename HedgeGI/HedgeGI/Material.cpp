#include "Material.h"
#include "Scene.h"

const Bitmap* Material::readBitmapReference(const FileStream& file, const Scene& scene)
{
    const uint32_t index = file.read<uint32_t>();
    return index != (uint32_t)-1 ? scene.bitmaps[index].get() : nullptr;
}

void Material::writeBitmapReference(const FileStream& file, const Scene& scene, const Bitmap* bitmap)
{
    uint32_t index = (uint32_t)-1;

    if (bitmap != nullptr)
    {
        for (size_t i = 0; i < scene.bitmaps.size(); i++)
        {
            if (bitmap != scene.bitmaps[i].get())
                continue;

            index = (uint32_t)i;
            break;
        }
    }

    file.write(index);
}

void Material::read(const FileStream& file, const Scene& scene)
{
    name = file.readString();
    type = (MaterialType)file.read<uint32_t>();
    parameters = file.read<Parameters>();
    textures.diffuse = readBitmapReference(file, scene);
    textures.specular = readBitmapReference(file, scene);
    textures.gloss = readBitmapReference(file, scene);
    textures.alpha = readBitmapReference(file, scene);
    textures.diffuseBlend = readBitmapReference(file, scene);
    textures.specularBlend = readBitmapReference(file, scene);
    textures.glossBlend = readBitmapReference(file, scene);
    textures.emission = readBitmapReference(file, scene);
    textures.environment = readBitmapReference(file, scene);
}

void Material::write(const FileStream& file, const Scene& scene) const
{
    file.write(name);
    file.write((uint32_t)type);
    file.write(parameters);
    writeBitmapReference(file, scene, textures.diffuse);
    writeBitmapReference(file, scene, textures.specular);
    writeBitmapReference(file, scene, textures.gloss);
    writeBitmapReference(file, scene, textures.alpha);
    writeBitmapReference(file, scene, textures.diffuseBlend);
    writeBitmapReference(file, scene, textures.specularBlend);
    writeBitmapReference(file, scene, textures.glossBlend);
    writeBitmapReference(file, scene, textures.emission);
    writeBitmapReference(file, scene, textures.environment);
}
