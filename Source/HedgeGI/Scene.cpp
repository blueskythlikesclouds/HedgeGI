#include "Scene.h"

#include "Bitmap.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"
#include "Instance.h"
#include "Light.h"
#include "SHLightField.h"
#include "RaytracingDevice.h"

Scene::~Scene()
{
    if (rtcScene != nullptr)
        rtcReleaseScene(rtcScene);

    if (skyRtcScene != nullptr)
        rtcReleaseScene(skyRtcScene);
}

const LightBVH& Scene::getLightBVH() const
{
    return lightBVH;
}

void Scene::buildAABB()
{
    aabb.setEmpty();

    for (auto& mesh : meshes)
        aabb.extend(mesh->aabb);
}

RTCScene Scene::createRTCScene()
{
    if (rtcScene != nullptr)
        return rtcScene;

    rtcScene = rtcNewScene(RaytracingDevice::get());
    for (size_t i = 0; i < meshes.size(); i++)
    {
        bool found = false;

        for (auto& instance : instances)
        {
            for (auto& mesh : instance->meshes)
            {
                if (mesh != meshes[i].get())
                    continue;

                found = true;
                break;
            }

            if (found) break;
        }

        if (!found || meshes[i]->type == MeshType::Transparent || meshes[i]->type == MeshType::Special)
            continue;

        const RTCGeometry rtcGeometry = meshes[i]->createRTCGeometry();
        rtcAttachGeometryByID(rtcScene, rtcGeometry, (uint32_t)i);
        rtcReleaseGeometry(rtcGeometry);
    }

    rtcSetSceneBuildQuality(rtcScene, RTC_BUILD_QUALITY_HIGH);
    rtcCommitScene(rtcScene);
    return rtcScene;
}

RTCScene Scene::createSkyRTCScene()
{
    if (skyRtcScene != nullptr)
        return skyRtcScene;

    skyRtcScene = rtcNewScene(RaytracingDevice::get());
    for (size_t i = 0; i < meshes.size(); i++)
    {
        if (meshes[i]->material == nullptr || meshes[i]->material->type != MaterialType::Sky || !meshes[i]->material->textures.diffuse)
            continue;

        bool found = false;

        for (auto& model : models)
        {
            for (auto& mesh : model->meshes)
            {
                if (mesh != meshes[i].get())
                    continue;

                found = true;
                break;
            }

            if (found) break;
        }

        if (!found)
            continue;

        const RTCGeometry rtcGeometry = meshes[i]->createRTCGeometry();
        rtcAttachGeometryByID(skyRtcScene, rtcGeometry, (uint32_t)i);
        rtcReleaseGeometry(rtcGeometry);
    }

    rtcSetSceneBuildQuality(skyRtcScene, RTC_BUILD_QUALITY_HIGH);
    rtcCommitScene(skyRtcScene);
    return skyRtcScene;
}

const LightBVH* Scene::createLightBVH(const bool force)
{
    if ((force || !lightBVH.valid()) && !lights.empty())
        lightBVH.build(*this);

    return &lightBVH;
}

RaytracingContext Scene::getRaytracingContext()
{
    return { this, createRTCScene(), createSkyRTCScene(), createLightBVH() };
}

void Scene::removeUnusedBitmaps()
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

    std::vector<std::unique_ptr<const Bitmap>> distinctBitmaps;
    for (auto& bitmap : bitmaps)
    {
        if (bitmapSet.find(bitmap.get()) == bitmapSet.end())
            continue;

        distinctBitmaps.push_back(std::move(bitmap));
    }

    std::swap(bitmaps, distinctBitmaps);
}