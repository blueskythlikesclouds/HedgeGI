#pragma once

class Bitmap;
class Instance;
class Light;
class Material;
class Mesh;
class Scene;

class SceneFactory
{
public:
    static std::unique_ptr<Bitmap> createBitmap(const uint8_t* data, size_t length);
    static std::unique_ptr<Material> createMaterial(HlHHMaterialV3* material, const Scene& scene);
    static std::unique_ptr<Mesh> createMesh(HlHHMesh* mesh, const Eigen::Affine3f& transformation, const Scene& scene);
    static std::unique_ptr<Instance> createInstance(HlHHTerrainInstanceInfoV0* instance, HlHHTerrainModelV5* model, Scene& scene);
    static std::unique_ptr<Light> createLight(HlHHLightV1* light);

    static void loadResources(HlArchive* archive, Scene& scene);
    static void loadTerrain(HlArchive* archive, Scene& scene);

    static std::unique_ptr<Scene> createFromGenerations(const std::string& directoryPath);
    static std::unique_ptr<Scene> createFromLostWorldOrForces(const std::string& directoryPath);
};
