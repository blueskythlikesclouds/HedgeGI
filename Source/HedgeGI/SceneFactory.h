#pragma once

class Bitmap;
class Material;
class Mesh;
class Instance;
class Light;
class SHLightField;

class Scene;

class SceneFactory
{
public:
    static std::unique_ptr<Bitmap> createBitmap(const uint8_t* data, size_t length);
    static std::unique_ptr<Material> createMaterial(hl::hh::mirage::raw_material_v3* material, const Scene& scene);
    static std::unique_ptr<Mesh> createMesh(hl::hh::mirage::raw_mesh* mesh, const Affine3& transformation, const Scene& scene);
    static std::unique_ptr<Instance> createInstance(hl::hh::mirage::raw_terrain_instance_info_v0* instance, hl::hh::mirage::raw_terrain_model_v5* model, Scene& scene);
    static std::unique_ptr<Light> createLight(hl::hh::mirage::raw_light* light);
    static std::unique_ptr<SHLightField> createSHLightField(hl::hh::needle::raw_sh_light_field_node* shlf);

    static void loadResources(const hl::archive& archive, Scene& scene);
    static void loadTerrain(const hl::archive& archive, Scene& scene);

    static void loadSceneEffect(const hl::archive& archive, Scene& scene, const std::string& stageName);

    static std::unique_ptr<Scene> createFromGenerations(const std::string& directoryPath);
    static std::unique_ptr<Scene> createFromLostWorldOrForces(const std::string& directoryPath);
    static std::unique_ptr<Scene> create(const std::string& directoryPath);
};
