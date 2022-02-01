#include "SceneFactory.h"

#include "Scene.h"

#include "Bitmap.h"
#include "CabinetCompression.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"
#include "Instance.h"
#include "Light.h"
#include "SHLightField.h"

#include "FileStream.h"
#include "Logger.h"
#include "Utilities.h"

std::unique_ptr<Bitmap> SceneFactory::createBitmap(const uint8_t* data, const size_t length)
{
    std::unique_ptr<DirectX::ScratchImage> scratchImage = std::make_unique<DirectX::ScratchImage>();

    DirectX::TexMetadata metadata;
    LoadFromDDSMemory(data, length, DirectX::DDS_FLAGS_NONE, &metadata, *scratchImage);

    if (!scratchImage->GetImages())
        return nullptr;

    // Try getting the second mip (we don't need much quality from textures)
    const size_t mipLevel = scratchImage->IsAlphaAllOpaque() ? std::min<size_t>(2, metadata.mipLevels - 1) : 0;

    if (DirectX::IsCompressed(metadata.format))
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Decompress(*scratchImage->GetImage(mipLevel, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }
    else if (metadata.format != DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Convert(*scratchImage->GetImage(mipLevel, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }

    if (!scratchImage->GetImages())
        return nullptr;

    metadata = scratchImage->GetMetadata();

    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>();

    bitmap->type =
        ((metadata.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0) ? BitmapType::Cube :
        metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D ? BitmapType::_3D :
        BitmapType::_2D;

    bitmap->width = (uint32_t)metadata.width;
    bitmap->height = (uint32_t)metadata.height;
    bitmap->arraySize = bitmap->type == BitmapType::_3D ? (uint32_t)metadata.depth : (uint32_t)metadata.arraySize;
    bitmap->data = std::make_unique<Color4[]>(bitmap->width * bitmap->height * bitmap->arraySize);

    for (size_t i = 0; i < bitmap->arraySize; i++)
        memcpy(&bitmap->data[bitmap->width * bitmap->height * i], scratchImage->GetImage(0, i, 0)->pixels, bitmap->width * bitmap->height * sizeof(Vector4));

    return bitmap;
}

std::unique_ptr<Material> SceneFactory::createMaterial(hl::hh::mirage::raw_material_v3* material, const Scene& scene)
{
    std::unique_ptr<Material> newMaterial = std::make_unique<Material>();

    newMaterial->type =
        strstr(material->shaderName.get(), "Sky") != nullptr ? MaterialType::Sky :
        strstr(material->shaderName.get(), "IgnoreLight") != nullptr ? MaterialType::IgnoreLight :
        strstr(material->shaderName.get(), "Blend") != nullptr ? MaterialType::Blend
        : MaterialType::Common;

    if (newMaterial->type == MaterialType::Sky)
    {
        newMaterial->skyType =
            strstr(material->shaderName.get(), "Sky3") != nullptr ? 3 :
            strstr(material->shaderName.get(), "Sky2") != nullptr ? 2
            : 0;

        newMaterial->skySqrt = strstr(material->shaderName.get(), "Sqrt") != nullptr;
    }

    newMaterial->ignoreVertexColor = newMaterial->type == MaterialType::Blend || strstr(material->shaderName.get(), "FadeOutNormal");

    newMaterial->hasMetalness =
        strstr(material->shaderName.get(), "MCommon") != nullptr ||
        strstr(material->shaderName.get(), "MBlend") != nullptr ||
        strstr(material->shaderName.get(), "MEmission") != nullptr;

    for (size_t i = 0; i < material->float4ParamCount; i++)
    {
        const auto& param = material->float4Params[i];
        const Color4 value(param->values[0].x, param->values[0].y, param->values[0].z, param->values[0].w);

        if (strcmp(param->name.get(), "diffuse") == 0) newMaterial->parameters.diffuse = value;
        else if (strcmp(param->name.get(), "specular") == 0) newMaterial->parameters.specular = value;
        else if (strcmp(param->name.get(), "ambient") == 0) newMaterial->parameters.ambient = value;
        else if (strcmp(param->name.get(), "power_gloss_level") == 0) newMaterial->parameters.powerGlossLevel = value;
        else if (strcmp(param->name.get(), "opacity_reflection_refraction_spectype") == 0) newMaterial->parameters.opacityReflectionRefractionSpecType = value;
        else if (strcmp(param->name.get(), "mrgLuminanceRange") == 0) newMaterial->parameters.luminanceRange = value;
        else if (strcmp(param->name.get(), "Luminance") == 0) newMaterial->parameters.luminance = value;
        else if (strcmp(param->name.get(), "PBRFactor") == 0) newMaterial->parameters.pbrFactor = value;
        else if (strcmp(param->name.get(), "PBRFactor2") == 0) newMaterial->parameters.pbrFactor2 = value;
        else if (strcmp(param->name.get(), "g_EmissionParam") == 0) newMaterial->parameters.emissionParam = value;
        else if (strcmp(param->name.get(), "emissive") == 0) newMaterial->parameters.emissive = value;
    }

    newMaterial->parameters.doubleSided = material->noBackfaceCulling;
    newMaterial->parameters.additive = material->useAdditiveBlending;

    for (size_t i = 0; i < material->textureEntryCount; i++)
    {
        const auto& texture = material->textureEntries[i];

        const Bitmap* bitmap = nullptr;

        for (auto& item : scene.bitmaps)
        {
            if (strcmp(texture->texName.get(), item->name.c_str()) != 0)
                continue;

            bitmap = item.get();
            break;
        }

        if (bitmap == nullptr)
        {
            Logger::logFormatted(LogType::Error, "Failed to find %s.dds", texture->texName.get());
            continue;
        }

        if (strcmp(texture->type.get(), "diffuse") == 0)
        {
            if (newMaterial->textures.diffuse != nullptr)
                newMaterial->textures.diffuseBlend = bitmap;
            else
                newMaterial->textures.diffuse = bitmap;
        }
        else if (strcmp(texture->type.get(), "specular") == 0)
        {
            if (newMaterial->textures.specular != nullptr)
                newMaterial->textures.specularBlend = bitmap;
            else
                newMaterial->textures.specular = bitmap;
        }
        else if (strcmp(texture->type.get(), "gloss") == 0)
        {
            if (newMaterial->textures.gloss != nullptr)
                newMaterial->textures.glossBlend = bitmap;
            else
                newMaterial->textures.gloss = bitmap;
        }
        else if (strcmp(texture->type.get(), "normal") == 0)
        {
            if (newMaterial->textures.normal != nullptr)
                newMaterial->textures.normalBlend = bitmap;
            else
                newMaterial->textures.normal = bitmap;
        }
        else if (strcmp(texture->type.get(), "opacity") == 0 ||
            strcmp(texture->type.get(), "transparency") == 0)
        {
            newMaterial->textures.alpha = bitmap;
        }
        else if (strcmp(texture->type.get(), "displacement") == 0 ||
            strcmp(texture->type.get(), "emission") == 0)
        {
            newMaterial->textures.emission = bitmap;
        }
        else if (strcmp(texture->type.get(), "reflection") == 0)
        {
            newMaterial->textures.environment = bitmap;
        }
    }

    return newMaterial;
}

std::unique_ptr<Mesh> SceneFactory::createMesh(hl::hh::mirage::raw_mesh* mesh, const Affine3& transformation, const Scene& scene)
{
    std::unique_ptr<Mesh> newMesh = std::make_unique<Mesh>();

    newMesh->vertexCount = mesh->vertexCount;
    newMesh->vertices = std::make_unique<Vertex[]>(mesh->vertexCount);

    for (size_t i = 0; i < mesh->vertexCount; i++)
    {
        auto element = &mesh->vertexElements[0];

        while (element->format != (hl::u32)hl::hh::mirage::vertex_format::last_entry)
        {
            Vertex& vertex = newMesh->vertices[i];

            float* destination = nullptr;
            size_t size = 0;

            switch (element->type)
            {
            case hl::hh::mirage::vertex_type::position:
                destination = vertex.position.data();
                size = vertex.position.size();
                break;

            case hl::hh::mirage::vertex_type::normal:
                destination = vertex.normal.data();
                size = vertex.normal.size();
                break;

            case hl::hh::mirage::vertex_type::tangent:
                destination = vertex.tangent.data();
                size = vertex.tangent.size();
                break;

            case hl::hh::mirage::vertex_type::binormal:
                destination = vertex.binormal.data();
                size = vertex.binormal.size();
                break;

            case  hl::hh::mirage::vertex_type::texcoord:
                if (element->index == 0)
                {
                    destination = vertex.uv.data();
                    size = vertex.uv.size();
                }

                else if (element->index == 1)
                {
                    destination = vertex.vPos.data();
                    size = vertex.vPos.size();
                }

                break;

            case hl::hh::mirage::vertex_type::color:
                destination = vertex.color.data();
                size = vertex.color.size();
                break;

            default:
                element++;
                continue;
            }

            hl::vec4 value;
            element->convert_to_vec4(&((hl::u8*)mesh->vertices.get())[mesh->vertexSize * i + element->offset], value);
            memcpy(destination, &value, size * sizeof(float));

            element++;
        }
    }

    Affine3 rotation;
    rotation = transformation.rotation();

    bool anyInvalid = false;

    for (size_t i = 0; i < newMesh->vertexCount; i++)
    {
        Vertex& vertex = newMesh->vertices[i];
        anyInvalid |= vertex.tangent == Vector3::Zero() || vertex.binormal == Vector3::Zero();

        vertex.position = transformation * Eigen::Vector3f(vertex.position);
        vertex.normal = (rotation * Eigen::Vector3f(vertex.normal)).normalized();
        vertex.tangent = (rotation * Eigen::Vector3f(vertex.tangent)).normalized();
        vertex.binormal = (rotation * Eigen::Vector3f(vertex.binormal)).normalized();
    }

    std::vector<Triangle> triangles;
    triangles.reserve(mesh->faces.count);

    hl::u32 i = 0;

    hl::u16 a = mesh->faces[i++];
    hl::u16 b = mesh->faces[i++];
    int32_t direction = -1;

    while (i < mesh->faces.count)
    {
        const hl::u16 c = mesh->faces[i++];
        if (c == (hl::u16)-1)
        {
            a = mesh->faces[i++];
            b = mesh->faces[i++];
            direction = -1;
        }
        else
        {
            direction *= -1;
            if (a != b && b != c && c != a)
            {
                if (direction > 0)
                    triangles.push_back({ a, c, b });
                else
                    triangles.push_back({ a, b, c });
            }

            a = b;
            b = c;
        }
    }

    newMesh->triangleCount = (uint32_t)triangles.size();
    newMesh->triangles = std::make_unique<Triangle[]>(triangles.size());
    std::copy(triangles.begin(), triangles.end(), newMesh->triangles.get());

    for (auto& material : scene.materials)
    {
        if (strcmp(material->name.c_str(), mesh->materialName.get()) != 0)
            continue;

        newMesh->material = material.get();
        break;
    }

    if (!newMesh->material)
        Logger::logFormatted(LogType::Error, "Failed to find %s.material", mesh->materialName.get());

    if (anyInvalid)
        newMesh->generateTangents();

    newMesh->buildAABB();

    return newMesh;
}

std::unique_ptr<Model> SceneFactory::createModel(hl::hh::mirage::raw_skeletal_model_v5* model, Scene& scene)
{
    std::unique_ptr<Model> newModel = std::make_unique<Model>();

    auto addMesh = [&](hl::hh::mirage::raw_mesh* srcMesh, const MeshType type)
    {
        std::unique_ptr<Mesh> mesh = createMesh(srcMesh, Affine3::Identity(), scene);
        mesh->type = type;

        newModel->meshes.push_back(mesh.get());

        scene.meshes.push_back(std::move(mesh));
    };

    for (auto& meshGroup : model->meshGroups)
    {
        for (auto& solid : meshGroup->opaq)
            addMesh(solid.get(), MeshType::Opaque);

        for (auto& trans : meshGroup->trans)
            addMesh(trans.get(), MeshType::Transparent);

        for (auto& punch : meshGroup->punch)
            addMesh(punch.get(), MeshType::Punch);
    }

    return newModel;
}

std::unique_ptr<Instance> SceneFactory::createInstance(hl::hh::mirage::raw_terrain_instance_info_v0* instance, hl::hh::mirage::raw_terrain_model_v5* model, Scene& scene)
{
    std::unique_ptr<Instance> newInstance = std::make_unique<Instance>();

    newInstance->name = instance ? instance->instanceName.get() : model->name.get();

    Matrix4 transformationMatrix;

    if (instance != nullptr)
    {
        const hl::matrix4x4* matrix = instance->matrix.get();

        transformationMatrix <<
            matrix->m11, matrix->m12, matrix->m13, matrix->m14,
            matrix->m21, matrix->m22, matrix->m23, matrix->m24,
            matrix->m31, matrix->m32, matrix->m33, matrix->m34,
            matrix->m41, matrix->m42, matrix->m43, matrix->m44;
    }
    else
    {
        transformationMatrix = Matrix4::Identity();
    }

    Affine3 transformationAffine;
    transformationAffine = transformationMatrix;

    auto addMesh = [&](hl::hh::mirage::raw_mesh* srcMesh, const MeshType type)
    {
        std::unique_ptr<Mesh> mesh = createMesh(srcMesh, transformationAffine, scene);
        mesh->type = type;

        newInstance->meshes.push_back(mesh.get());

        scene.meshes.push_back(std::move(mesh));
    };

    for (auto& meshGroup : model->meshGroups)
    {
        for (auto& solid : meshGroup->opaq)
            addMesh(solid.get(), MeshType::Opaque);

        for (auto& trans : meshGroup->trans)
            addMesh(trans.get(), MeshType::Transparent);

        for (auto& punch : meshGroup->punch)
            addMesh(punch.get(), MeshType::Punch);

        for (const auto& specialMeshGroup : meshGroup->special)
        {
            for (size_t i = 0; i < specialMeshGroup.meshCount; i++)
                addMesh(specialMeshGroup.meshes[i].get(), MeshType::Special);
        }
    }

    newInstance->buildAABB();
    return newInstance;
}

std::unique_ptr<Light> SceneFactory::createLight(hl::hh::mirage::raw_light* light)
{
    std::unique_ptr<Light> newLight = std::make_unique<Light>();

    newLight->type = (LightType)light->type;
    newLight->position[0] = light->position.x;
    newLight->position[1] = light->position.y;
    newLight->position[2] = light->position.z;
    newLight->color[0] = light->color.x;
    newLight->color[1] = light->color.y;
    newLight->color[2] = light->color.z;

    if (newLight->type != LightType::Point)
    {
        newLight->position.normalize();
        return newLight;
    }

    newLight->range[0] = light->range.x;
    newLight->range[1] = light->range.y;
    newLight->range[2] = light->range.z;
    newLight->range[3] = light->range.w;

    return newLight;
}

std::unique_ptr<SHLightField> SceneFactory::createSHLightField(hl::hh::needle::raw_sh_light_field_node* shlf)
{
    std::unique_ptr<SHLightField> newSHLightField = std::make_unique<SHLightField>();
    newSHLightField->name = shlf->name.get();
    newSHLightField->resolution = Eigen::Array3i(shlf->probeCounts[0], shlf->probeCounts[1], shlf->probeCounts[2]);
    newSHLightField->position = { shlf->position.x, shlf->position.y, shlf->position.z };
    newSHLightField->rotation = { shlf->rotation.x, shlf->rotation.y, shlf->rotation.z };
    newSHLightField->scale = { shlf->scale.x, shlf->scale.y, shlf->scale.z };

    return newSHLightField;
}

void SceneFactory::loadLights(const hl::archive& archive, Scene& scene)
{
    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".light")) || hl::text::strstr(entry.name(), HL_NTEXT(".light-list")))
            continue;

        void* data = (void*)entry.file_data();

        hl::hh::mirage::fix(data);

        auto light = hl::hh::mirage::get_data<hl::hh::mirage::raw_light>(data);
        light->endian_swap();

        std::unique_ptr<Light> newLight = createLight(light);
        newLight->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

        scene.lights.push_back(std::move(newLight));
    }
}

void SceneFactory::loadResources(const hl::archive& archive, Scene& scene, const std::string& stageName)
{
    const auto rgbTableName = toNchar((stageName + "_rgb_table0.dds").c_str());

    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".dds")) && !hl::text::strstr(entry.name(), HL_NTEXT(".DDS")))
            continue;

        std::unique_ptr<Bitmap> bitmap = createBitmap(entry.file_data<uint8_t>(), entry.size());
        if (!bitmap)
        {
            Logger::logFormatted(LogType::Error, "Failed to load %s", toUtf8(entry.name()).data());
            continue;
        }

        bitmap->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

        if (hl::text::equal(entry.name(), rgbTableName.data()))
            scene.rgbTable = std::move(bitmap);

        else
            scene.bitmaps.push_back(std::move(bitmap));
    }

    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".material")))
            continue;

        void* data = (void*)entry.file_data();

        hl::hh::mirage::fix(data);

        hl::u32 version;
        auto material = hl::hh::mirage::get_data<hl::hh::mirage::raw_material_v3>(data, &version);

        // Allow version 0 to account for HedgeLib C#'s faulty writing
        if (!hl::hh::mirage::has_sample_chunk_header_fixed(data) && version != 0 && version != 3)
        {
            Logger::logFormatted(LogType::Error, "Failed to load %s, unsupported version %d", toUtf8(entry.name()).data(), version);
            continue;
        }

        material->fix();

        std::unique_ptr<Material> newMaterial = createMaterial(material, scene);
        newMaterial->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

        scene.materials.push_back(std::move(newMaterial));
    }

    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".model")))
            continue;

        void* data = (void*)entry.file_data();

        hl::hh::mirage::fix(data);

        hl::u32 version;
        auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_skeletal_model_v5>(data, &version);

        if (!hl::hh::mirage::has_sample_chunk_header_fixed(data) && version != 5)
        {
            Logger::logFormatted(LogType::Error, "Failed to load %s, unsupported version %d", toUtf8(entry.name()).data(), version);
            continue;
        }

        model->fix();

        std::unique_ptr<Model> newModel = createModel(model, scene);
        newModel->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

        scene.models.push_back(std::move(newModel));
    }

    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".shlf")))
            continue;

        void* data = (void*)entry.file_data();

        hl::bina::fix64(data, entry.size());

        auto shlf = hl::bina::get_data<hl::hh::needle::raw_sh_light_field>(data);

        for (size_t i = 0; i < shlf->count; i++)
            scene.shLightFields.push_back(std::move(createSHLightField(&shlf->entries[i])));
    }
}

void SceneFactory::loadTerrain(const std::vector<hl::archive>& archives, Scene& scene)
{
    std::vector<hl::hh::mirage::raw_terrain_model_v5*> models;

    for (auto& archive : archives)
    {
        for (auto& entry : archive)
        {
            if (!hl::text::strstr(entry.name(), HL_NTEXT(".terrain-model")))
                continue;

            void* data = (void*)entry.file_data();

            hl::hh::mirage::fix(data);

            const bool hasSampleChunkHeader = hl::hh::mirage::has_sample_chunk_header_fixed(data);

            if (hasSampleChunkHeader)
            {
                const auto header = (hl::hh::mirage::sample_chunk::raw_header*)data;
                const auto node = header->get_node("DisableC");

                if (node != nullptr && node->value != 0)
                    continue;
            }

            hl::u32 version;
            auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_model_v5>(data, &version);

            if (!hasSampleChunkHeader && version != 5)
            {
                Logger::logFormatted(LogType::Error, "Failed to load %s, unsupported version %d", toUtf8(entry.name()).data(), version);
                continue;
            }

            model->fix();

            model->flags = false;
            models.push_back(model);
        }
    }

    for (auto& archive : archives)
    {
        for (auto& entry : archive)
        {
            if (!hl::text::strstr(entry.name(), HL_NTEXT(".terrain-instanceinfo")))
                continue;

            void* data = (void*)entry.file_data();

            hl::hh::mirage::fix(data);

            auto instance = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_instance_info_v0>(data);
            instance->fix();

            for (auto& model : models)
            {
                if (strcmp(model->name.get(), instance->modelName.get()) != 0)
                    continue;

                model->flags = true;

                scene.instances.push_back(createInstance(instance, model, scene));
                break;
            }
        }
    }

    // Load models that aren't bound to any instances
    for (auto& model : models)
    {
        if (model->flags)
            continue;

        scene.instances.push_back(createInstance(nullptr, model, scene));
    }
}

void SceneFactory::loadSceneEffect(const hl::archive& archive, Scene& scene, const std::string& stageName)
{
    auto hhdName = toNchar((stageName + ".hhd").c_str());
    auto rflName = toNchar((stageName + ".rfl").c_str());

    for (auto& entry : archive)
    {
        if (hl::text::equal(entry.name(), HL_NTEXT("SceneEffect.prm.xml")))
        {
            tinyxml2::XMLDocument doc;

            if (doc.Parse(entry.file_data<char>(), entry.size()) == tinyxml2::XML_SUCCESS)
                scene.effect.load(doc);

            break;
        }

        if (hl::text::equal(entry.name(), hhdName.data()))
        {
            hl::bina::fix32((void*)entry.file_data(), entry.size());

            if (const auto data = hl::bina::get_data<sonic2013::FxSceneData>(entry.file_data()); data)
                scene.effect.load(*data);

            break;
        }

        if (hl::text::equal(entry.name(), rflName.data()))
        {
            hl::bina::fix64((void*)entry.file_data(), entry.size());

            if (const auto data = hl::bina::get_data<wars::NeedleFxSceneData>(entry.file_data()); data)
                scene.effect.load(*data);

            break;
        }
    }
}

std::unique_ptr<Scene> SceneFactory::createFromGenerations(const std::string& directoryPath)
{
    const std::string stageName = getFileNameWithoutExtension(directoryPath);

    std::unique_ptr<Scene> scene = std::make_unique<Scene>();

    auto filePath = toNchar((directoryPath + "/" + stageName + ".ar.00").c_str());

    const auto resArchive = hl::hh::ar::load(filePath.data());
    loadResources(resArchive, *scene, stageName);
    loadLights(resArchive, *scene);

    hl::hh::pfi::v0::header* pfi = nullptr;

    for (auto& entry : resArchive)
    {
        if (!hl::text::equal(entry.name(), HL_NTEXT("Stage.pfi")))
            continue;

        void* data = (void*)entry.file_data();

        hl::hh::mirage::fix(data);

        pfi = hl::hh::mirage::get_data<hl::hh::pfi::v0::header>(data);
        pfi->fix();

        break;
    }

    if (!pfi)
    {
        Logger::log(LogType::Error, "Failed to find Stage.pfi");
        return nullptr;
    }

    const FileStream pfdFile((directoryPath + "/Stage.pfd").c_str(), "rb");

    for (auto& entry : pfi->entries)
    {
        if (!strstr(entry->name.get(), "tg-"))
            continue;

        std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(entry->dataSize);
        pfdFile.seek(entry->dataPos, SEEK_SET);
        pfdFile.read(data.get(), entry->dataSize);

        loadTerrain({ CabinetCompression::load(data.get(), entry->dataSize) }, *scene);
    }

    scene->sortAndUnify();
    scene->buildAABB();
    scene->createLightBVH();

    auto arFilePath = toNchar((directoryPath + "/../../#" + stageName + ".ar.00").c_str());

    if (hl::path::exists(arFilePath.data()))
        loadSceneEffect(hl::hh::ar::load(arFilePath.data()), *scene, stageName);

    return scene;
}

std::unique_ptr<Scene> SceneFactory::createFromLostWorldOrForces(const std::string& directoryPath)
{
    std::unique_ptr<Scene> scene = std::make_unique<Scene>();

    std::vector<hl::archive> archives;

    const std::string stageName = getFileNameWithoutExtension(directoryPath);
    {
        auto filePath = toNchar((directoryPath + "/" + stageName + "_trr_cmn.pac").c_str());

        const auto archive = hl::pacx::load(filePath.data());

        loadResources(archive, *scene, stageName);
        archives.push_back(std::move(archive));
    }

    for (uint32_t i = 0; i < 100; i++)
    {
        char slot[4];
        sprintf(slot, "%02d", i);

        auto filePath = toNchar((directoryPath + "/" + stageName + "_trr_s" + slot + ".pac").c_str());

        if (!hl::path::exists(filePath.data()))
            continue;

        archives.push_back(std::move(hl::pacx::load(filePath.data())));
    }

    loadTerrain(archives, *scene);

    // Load lights from sectors
    for (auto& archive : archives)
        loadLights(archive, *scene);

    archives.clear();

    auto skyFilePath = toNchar((directoryPath + "/" + stageName + "_sky.pac").c_str());

    if (hl::path::exists(skyFilePath.data()))
        loadResources(hl::pacx::load(skyFilePath.data()), *scene, stageName);

    scene->sortAndUnify();
    scene->buildAABB();
    scene->createLightBVH();

    auto miscFilePath = toNchar((directoryPath + "/" + stageName + "_misc.pac").c_str());

    if (hl::path::exists(miscFilePath.data()))
        loadSceneEffect(hl::pacx::load(miscFilePath.data()), *scene, stageName);

    return scene;
}

std::unique_ptr<Scene> SceneFactory::create(const std::string& directoryPath)
{
    if (std::filesystem::exists(directoryPath + "/Stage.pfd"))
        return createFromGenerations(directoryPath);

    return createFromLostWorldOrForces(directoryPath);
}
