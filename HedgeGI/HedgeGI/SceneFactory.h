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
    static std::unique_ptr<Material> createMaterial(hl::HHMaterialV3* material, const Scene& scene);
    static std::unique_ptr<Mesh> createMesh(hl::HHMesh* mesh, const Eigen::Affine3f& transformation, const Scene& scene);
    static std::unique_ptr<Instance> createInstance(hl::HHTerrainInstanceInfoV0* instance, hl::HHTerrainModel* model, Scene& scene);
    static std::unique_ptr<Light> createLight(hl::HHLight* light);

    static void loadResources(const hl::Archive& archive, Scene& scene);
    static void loadTerrain(const hl::Archive& archive, Scene& scene);

    static std::unique_ptr<Scene> createFromGenerations(const std::string& directoryPath);
};
