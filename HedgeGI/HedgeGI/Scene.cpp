#include "Scene.h"

RaytracingContext::~RaytracingContext()
{
    rtcReleaseScene(rtcScene);
}

RTCScene Scene::createRTCScene() const
{
    const RTCScene rtcScene = rtcNewScene(RaytracingDevice::get());
    for (size_t i = 0; i < meshes.size(); i++)
    {
        const RTCGeometry rtcGeometry = meshes[i]->createRTCGeometry();
        rtcAttachGeometryByID(rtcScene, rtcGeometry, (uint32_t)i);
        rtcReleaseGeometry(rtcGeometry);
    }

    rtcCommitScene(rtcScene);
    return rtcScene;
}

RaytracingContext Scene::createRaytracingContext() const
{
    return { this, createRTCScene() };
}

void Scene::read(const FileStream& file)
{
    const uint32_t bitmapCount = file.read<uint32_t>();
    const uint32_t materialCount = file.read<uint32_t>();
    const uint32_t meshCount = file.read<uint32_t>();
    const uint32_t instanceCount = file.read<uint32_t>();
    const uint32_t lightCount = file.read<uint32_t>();

    bitmaps.reserve(bitmapCount);
    materials.reserve(materialCount);
    meshes.reserve(meshCount);
    instances.reserve(instanceCount);
    lights.reserve(lightCount);

    for (uint32_t i = 0; i < bitmapCount; i++)
    {
        std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>();
        bitmap->read(file);
        bitmaps.push_back(std::move(bitmap));
    }

    for (uint32_t i = 0; i < materialCount; i++)
    {
        std::unique_ptr<Material> material = std::make_unique<Material>();
        material->read(file, *this);
        materials.push_back(std::move(material));
    }

    for (uint32_t i = 0; i < meshCount; i++)
    {
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
        mesh->read(file, *this);
        meshes.push_back(std::move(mesh));
    }

    for (uint32_t i = 0; i < instanceCount; i++)
    {
        std::unique_ptr<Instance> instance = std::make_unique<Instance>();
        instance->read(file, *this);
        instances.push_back(std::move(instance));
    }

    for (uint32_t i = 0; i < lightCount; i++)
    {
        std::unique_ptr<Light> light = std::make_unique<Light>();
        light->read(file);
        lights.push_back(std::move(light));
    }
}

void Scene::write(const FileStream& file) const
{
    file.write((uint32_t)bitmaps.size());
    file.write((uint32_t)materials.size());
    file.write((uint32_t)meshes.size());
    file.write((uint32_t)instances.size());
    file.write((uint32_t)lights.size());

    for (auto& bitmap : bitmaps)
        bitmap->write(file);

    for (auto& material : materials)
        material->write(file, *this);

    for (auto& mesh : meshes)
        mesh->write(file, *this);

    for (auto& instance : instances)
        instance->write(file, *this);

    for (auto& light : lights)
        light->write(file);
}

bool Scene::load(const std::string& filePath)
{
    const FileStream file(filePath.c_str(), "rb");
    if (file.read<uint32_t>() != '\0NCS')
        return false;

    if (file.read<uint32_t>() != 0)
        return false;

    read(file);
    return true;
}

void Scene::save(const std::string& filePath) const
{
    const FileStream file(filePath.c_str(), "wb");

    file.write('\0NCS');
    file.write(0);
    write(file);
}
