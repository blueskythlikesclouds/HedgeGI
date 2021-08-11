#pragma once

#include "LightBVH.h"
#include "SceneEffect.h"

class Bitmap;
class Material;
class Mesh;
class Model;
class Instance;
class Light;
class SHLightField;

struct RaytracingContext
{
    const class Scene* scene {};
    RTCScene rtcScene {};
    RTCScene skyRtcScene {};
    const LightBVH* lightBVH;
};

class Scene
{
    RTCScene rtcScene {};
    RTCScene skyRtcScene {};
    LightBVH lightBVH {};

public:
    ~Scene();

    std::vector<std::unique_ptr<const Bitmap>> bitmaps;
    std::vector<std::unique_ptr<const Material>> materials;
    std::vector<std::unique_ptr<const Mesh>> meshes;
    std::vector<std::unique_ptr<const Model>> models;
    std::vector<std::unique_ptr<const Instance>> instances;

    std::vector<std::unique_ptr<Light>> lights;
    std::vector<std::unique_ptr<SHLightField>> shLightFields;

    AABB aabb;

    SceneEffect effect {};

    void buildAABB();

    RTCScene createRTCScene();
    RTCScene createSkyRTCScene();
    const LightBVH* createLightBVH(bool force = false);
    RaytracingContext getRaytracingContext();

    void removeUnusedBitmaps();
};
