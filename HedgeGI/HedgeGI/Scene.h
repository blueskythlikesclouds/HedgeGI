#pragma once

#include "Bitmap.h"
#include "Instance.h"
#include "Light.h"
#include "Material.h"
#include "Mesh.h"
#include "PropertyBag.h"
#include "SHLightField.h"

struct RaytracingContext
{
    const class Scene* scene{};
    RTCScene rtcScene{};
};

class Scene
{
    RTCScene rtcScene{};

public:
    ~Scene();

    std::vector<std::unique_ptr<const Bitmap>> bitmaps;
    std::vector<std::unique_ptr<const Material>> materials;
    std::vector<std::unique_ptr<const Mesh>> meshes;
    std::vector<std::unique_ptr<const Instance>> instances;
    std::vector<std::unique_ptr<const Light>> lights;
    std::vector<std::unique_ptr<const SHLightField>> shLightFields;
    AABB aabb;

    void buildAABB();

    RTCScene createRTCScene();
    RaytracingContext getRaytracingContext();

    void removeUnusedBitmaps();

    void read(const FileStream& file);
    void write(const FileStream& file) const;

    bool load(const std::string& filePath);
    void save(const std::string& filePath) const;
};
