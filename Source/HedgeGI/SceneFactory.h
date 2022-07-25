#pragma once

class Bitmap;
class Material;
class Mesh;
class Model;
class Instance;
class Light;
class SHLightField;

class Scene;

class SceneFactory
{
private:
    std::unique_ptr<Scene> scene;
    std::string stageName;
    CriticalSection criticalSection;

    std::unique_ptr<Bitmap> createBitmap(const uint8_t* data, size_t length) const;
    template<typename T> std::unique_ptr<Material> createMaterial(T* material, const hl::archive& archive) const;
    std::unique_ptr<Mesh> createMesh(hl::hh::mirage::raw_mesh* mesh, const Affine3& transformation) const;
    std::unique_ptr<Model> createModel(hl::hh::mirage::raw_skeletal_model_v5* model);
    std::unique_ptr<Instance> createInstance(hl::hh::mirage::raw_terrain_instance_info_v0* instance, hl::hh::mirage::raw_terrain_model_v5* model);
    std::unique_ptr<Light> createLight(hl::hh::mirage::raw_light* light) const;
    std::unique_ptr<SHLightField> createSHLightField(hl::hh::needle::raw_sh_light_field_node* shlf) const;

    void loadLights(const hl::archive& archive);
    void loadResources(const hl::archive& archive);
    void loadTerrain(const std::vector<hl::archive>& archives);
    void loadResolutions(const hl::archive& archive) const;
    void loadSceneEffect(const hl::archive& archive) const;

    void createFromUnleashedOrGenerations(const std::string& directoryPath);
    void createFromLostWorldOrForces(const std::string& directoryPath);

public:
    static std::unique_ptr<Scene> create(const std::string& directoryPath);
};
