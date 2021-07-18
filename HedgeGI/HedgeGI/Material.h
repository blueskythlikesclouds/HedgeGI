#pragma once

class Bitmap;
class Scene;

enum MaterialType : uint32_t
{
    MATERIAL_TYPE_COMMON,
    MATERIAL_TYPE_IGNORE_LIGHT
};

class Material
{
private:
    static const Bitmap* readBitmapReference(const FileStream& file, const Scene& scene);
    static void writeBitmapReference(const FileStream& file, const Scene& scene, const Bitmap* bitmap);

public:
    std::string name;
    MaterialType type{};

    struct Parameters
    {
        Color4 diffuse { 1, 1, 1, 1 };
        Color4 specular { 1, 1, 1, 1 };
        Color4 ambient { 1, 1, 1, 1 };
        Color4 powerGlossLevel { 0, 0, 0, 0 };
        Color4 opacityReflectionRefractionSpecType { 1, 0, 0, 0 };
        Color4 luminanceRange { 0, 0, 0, 0 };
        Color4 luminance { 1, 1, 1, 1 };
        Color4 pbrFactor { 0.04, 0.5, 0, 0 };
        Color4 pbrFactor2 { 0.04, 0.5, 0, 0 };
        Color4 emissionParam { 0, 0, 0, 1 };
        Color4 emissive { 0, 0, 0, 0 };
    } parameters{};

    struct Textures
    {
        const Bitmap* diffuse {};
        const Bitmap* specular {};
        const Bitmap* gloss {};
        const Bitmap* alpha {};
        const Bitmap* diffuseBlend {};
        const Bitmap* specularBlend {};
        const Bitmap* glossBlend {};
        const Bitmap* emission {};
        const Bitmap* environment {};
    } textures{};

    void read(const FileStream& file, const Scene& scene);
    void write(const FileStream& file, const Scene& scene) const;
};
