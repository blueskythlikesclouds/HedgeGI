#pragma once

class Bitmap;
class Scene;

class Material
{
private:
    static const Bitmap* readBitmapReference(const FileStream& file, const Scene& scene);
    static void writeBitmapReference(const FileStream& file, const Scene& scene, const Bitmap* bitmap);

public:
    std::string name;

    struct Parameters
    {
        Eigen::Array4f diffuse { Eigen::Array4f::Ones() };
        Eigen::Array4f specular { Eigen::Array4f::Ones() };
        Eigen::Array4f ambient { Eigen::Array4f::Ones() };
        Eigen::Array4f powerGlossLevel {};
        Eigen::Array4f luminanceRange {};
        Eigen::Array4f luminance { Eigen::Array4f::Ones() };
        Eigen::Array4f pbrFactor { };
        Eigen::Array4f pbrFactor2 { };
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
