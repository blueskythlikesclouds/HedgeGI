#include "Scene.h"

#include "Bitmap.h"
#include "Instance.h"
#include "Light.h"
#include "Material.h"
#include "Mesh.h"
#include "MetaInstancer.h"
#include "Model.h"
#include "RaytracingDevice.h"
#include "SHLightField.h"

Scene::~Scene()
{
    if (rtcScene != nullptr)
        rtcReleaseScene(rtcScene);
}

const LightBVH& Scene::getLightBVH() const
{
    return lightBVH;
}

void Scene::buildAABB()
{
    aabb.setEmpty();

    for (auto& instance : instances)
        aabb.extend(instance->aabb);
}

RTCScene Scene::createRTCScene()
{
    if (rtcScene != nullptr)
        return rtcScene;

    rtcScene = rtcNewScene(RaytracingDevice::get());
    for (size_t i = 0; i < meshes.size(); i++)
    {
        const auto& mesh = meshes[i];
        
        if ((mesh->material && mesh->material->parameters.additive) || mesh->type == MeshType::Special)
            continue;

        const RTCGeometry rtcGeometry = mesh->createRTCGeometry();

        rtcAttachGeometryByID(rtcScene, rtcGeometry, (uint32_t)i);
        rtcReleaseGeometry(rtcGeometry);
    }

    rtcSetSceneBuildQuality(rtcScene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(rtcScene, RTC_SCENE_FLAG_COMPACT | RTC_SCENE_FLAG_FILTER_FUNCTION_IN_ARGUMENTS);

    rtcCommitScene(rtcScene);

    return rtcScene;
}

const LightBVH* Scene::createLightBVH(const bool force)
{
    if ((force || !lightBVH.valid()) && !lights.empty())
        lightBVH.build(*this);

    return &lightBVH;
}

RaytracingContext Scene::getRaytracingContext()
{
    return { this, createRTCScene(), createLightBVH() };
}

void Scene::sortAndUnify()
{
    std::unordered_set<const Bitmap*> bitmapSet;
    for (auto& material : materials)
    {
        bitmapSet.insert(material->textures.diffuse);
        bitmapSet.insert(material->textures.specular);
        bitmapSet.insert(material->textures.gloss);
        bitmapSet.insert(material->textures.normal);
        bitmapSet.insert(material->textures.alpha);
        bitmapSet.insert(material->textures.diffuseBlend);
        bitmapSet.insert(material->textures.specularBlend);
        bitmapSet.insert(material->textures.glossBlend);
        bitmapSet.insert(material->textures.normalBlend);
        bitmapSet.insert(material->textures.emission);
        bitmapSet.insert(material->textures.environment);
    }

    std::vector<std::unique_ptr<Bitmap>> distinctBitmaps;
    for (auto& bitmap : bitmaps)
    {
        if (bitmapSet.find(bitmap.get()) == bitmapSet.end())
            continue;

        distinctBitmaps.push_back(std::move(bitmap));
    }

    std::swap(bitmaps, distinctBitmaps);

    std::stable_sort(instances.begin(), instances.end(), [](const auto& left, const auto& right) { return left->name < right->name; });
}