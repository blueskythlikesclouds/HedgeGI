#pragma once

#include "Bitmap.h"
#include "Instance.h"
#include "Light.h"
#include "Material.h"
#include "Mesh.h"

struct RaytracingContext
{
    const class Scene* scene;
    RTCScene rtcScene;

    RaytracingContext(RaytracingContext const&) = delete;
    RaytracingContext(RaytracingContext&&) = delete;
    ~RaytracingContext();
};

class Scene
{
public:
    std::vector<std::unique_ptr<const Bitmap>> bitmaps;
    std::vector<std::unique_ptr<const Material>> materials;
    std::vector<std::unique_ptr<const Mesh>> meshes;
    std::vector<std::unique_ptr<const Instance>> instances;
    std::vector<std::unique_ptr<const Light>> lights;

    RTCScene createRTCScene() const;
    RaytracingContext createRaytracingContext() const;

    void read(const FileStream& file);
    void write(const FileStream& file) const;

    bool load(const std::string& filePath);
    void save(const std::string& filePath) const;
};
