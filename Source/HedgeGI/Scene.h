#pragma once

#include "LightBVH.h"
#include "LightField.h"
#include "SceneEffect.h"

class MetaInstancer;
class Bitmap;
class Material;
class Mesh;
class Model;
class Instance;
class Light;
class SHLightField;

#define RAY_MASK_OPAQUE        (1 << 0)
#define RAY_MASK_TRANS         (1 << 1)
#define RAY_MASK_PUNCH_THROUGH (1 << 2)
#define RAY_MASK_SKY           (1 << 3)

struct RaytracingContext
{
    const class Scene* scene {};
    RTCScene rtcScene {};
    const LightBVH* lightBVH;
};

class Scene
{
    RTCScene rtcScene {};
    LightBVH lightBVH {};

public:
    ~Scene();

    std::vector<std::unique_ptr<Bitmap>> bitmaps;
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<Instance>> instances;
    std::vector<std::unique_ptr<Light>> lights;
    std::vector<std::unique_ptr<MetaInstancer>> metaInstancers;
    std::vector<std::unique_ptr<SHLightField>> shLightFields;

    LightField lightField;
    std::unique_ptr<Bitmap> rgbTable;
    SceneEffect effect{};
    AABB aabb;

    const LightBVH& getLightBVH() const;

    void buildAABB();

    RTCScene createRTCScene();
    const LightBVH* createLightBVH(bool force = false);
    RaytracingContext getRaytracingContext();

    void sortAndUnify();
};
