#pragma once

#include "LightBVH.h"
#include "LightField.h"
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

    std::vector<std::unique_ptr<Bitmap>> bitmaps;
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<Instance>> instances;

    std::vector<std::unique_ptr<Light>> lights;
    std::vector<std::unique_ptr<SHLightField>> shLightFields;
    LightField lightField;

    std::unique_ptr<const Bitmap> rgbTable;

    AABB aabb;

    SceneEffect effect {};

    const LightBVH& getLightBVH() const;

    void buildAABB();

    RTCScene createRTCScene();
    RTCScene createSkyRTCScene();
    const LightBVH* createLightBVH(bool force = false);
    RaytracingContext getRaytracingContext();

    void sortAndUnify();
};
