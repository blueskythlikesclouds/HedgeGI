#pragma once

class Bitmap;
class Scene;

class Material
{
public:
    std::string name;
    const Bitmap* diffuse{};
    const Bitmap* emission{};

    void read(const FileStream& file, const Scene& scene);
    void write(const FileStream& file, const Scene& scene) const;
};
