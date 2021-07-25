#pragma once

class Bitmap;
class Instance;
class Light;
class Material;
class Mesh;
class Scene;
class SHLightField;

class SceneFactory
{
public:
    static std::unique_ptr<Bitmap> createBitmap(const uint8_t* data, size_t length);
    static std::unique_ptr<Material> createMaterial(HlHHMaterialV3* material, const Scene& scene);
    static std::unique_ptr<Mesh> createMesh(HlHHMesh* mesh, const Affine3& transformation, const Scene& scene);
    static std::unique_ptr<Instance> createInstance(HlHHTerrainInstanceInfoV0* instance, HlHHTerrainModelV5* model, Scene& scene);
    static std::unique_ptr<Light> createLight(HlHHLightV1* light);
    static std::unique_ptr<SHLightField> createSHLightField(HlHHSHLightField* shlf);

    static void loadResources(HlArchive* archive, Scene& scene);
    static void loadTerrain(HlArchive* archive, Scene& scene);

    static void loadSceneEffect(HlArchive* archive, Scene& scene, const std::string& stageName);

    static std::unique_ptr<Scene> createFromGenerations(const std::string& directoryPath);
    static std::unique_ptr<Scene> createFromLostWorldOrForces(const std::string& directoryPath);
    static std::unique_ptr<Scene> create(const std::string& directoryPath);
};
