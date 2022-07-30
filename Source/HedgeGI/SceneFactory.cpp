#include "SceneFactory.h"

#include "ArchiveCompression.h"
#include "Bitmap.h"
#include "Instance.h"
#include "Light.h"
#include "Logger.h"
#include "Material.h"
#include "Mesh.h"
#include "MetaInstancer.h"
#include "Model.h"
#include "Scene.h"
#include "SHLightField.h"
#include "Utilities.h"

std::unique_ptr<Bitmap> SceneFactory::createBitmap(const uint8_t* data, const size_t length) const
{
    std::unique_ptr<DirectX::ScratchImage> scratchImage = std::make_unique<DirectX::ScratchImage>();

    DirectX::TexMetadata metadata;
    LoadFromDDSMemory(data, length, DirectX::DDS_FLAGS_NONE, &metadata, *scratchImage);

    if (!scratchImage->GetImages())
        return nullptr;

    DXGI_FORMAT format;

    switch (metadata.format)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

    default:
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    }

    // Try getting the second mip (we don't need much quality from textures)
    const size_t mipLevel = scratchImage->IsAlphaAllOpaque() ? std::min<size_t>(2, metadata.mipLevels - 1) : 0;

    if (DirectX::IsCompressed(metadata.format))
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Decompress(*scratchImage->GetImage(mipLevel, 0, 0), format, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }
    else if (metadata.format != format)
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Convert(*scratchImage->GetImage(mipLevel, 0, 0), format, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }

    if (!scratchImage->GetImages())
        return nullptr;

    metadata = scratchImage->GetMetadata();

    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>();

    bitmap->type =
        ((metadata.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0) ? BITMAP_TYPE_CUBE :
        metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D ? BITMAP_TYPE_3D :
        BITMAP_TYPE_2D;

    bitmap->format =
        format == DXGI_FORMAT_R32G32B32A32_FLOAT ? 
            BitmapFormat::F32 :
            BitmapFormat::U8;

    bitmap->width = metadata.width;
    bitmap->height = metadata.height;
    bitmap->arraySize = bitmap->type == BITMAP_TYPE_3D ? metadata.depth : metadata.arraySize;
    bitmap->data = operator new(bitmap->width * bitmap->height * bitmap->arraySize * (size_t)bitmap->format);

    for (size_t i = 0; i < bitmap->arraySize; i++)
        memcpy(bitmap->getColorPtr(bitmap->width * bitmap->height * i), scratchImage->GetImage(0, i, 0)->pixels, bitmap->width * bitmap->height * (size_t)bitmap->format);

    return bitmap;
}

template<typename T>
std::unique_ptr<Material> SceneFactory::createMaterial(T* material, const hl::archive& archive) const
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

    const auto createTexture = [&](const hl::hh::mirage::raw_texture_entry_v1* texture)
    {
        const Bitmap* bitmap = nullptr;

        for (auto& item : scene->bitmaps)
        {
            if (strcmp(texture->texName.get(), item->name.c_str()) != 0)
                continue;

            bitmap = item.get();
            break;
        }

        if (bitmap == nullptr)
        {
            Logger::logFormatted(LogType::Error, "Failed to find %s.dds", texture->texName.get());
            return;
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
    };

    if constexpr (std::is_same_v<T, hl::hh::mirage::raw_material_v1>)
    {
        const auto texsetFileName = toNchar((std::string(material->texsetName.get()) + ".texset").c_str());

        for (auto& texsetEntry : archive)
        {
            if (!hl::text::equal(texsetEntry.name(), texsetFileName.data()))
                continue;

            void* data = (void*)texsetEntry.file_data();
            uint32_t* marker = (uint32_t*)data + 1;

            // Mark manually to not corrupt the data with repeating calls.
            if (*marker != ~0)
                hl::hh::mirage::fix(data);

            const auto texset = hl::hh::mirage::get_data<hl::hh::mirage::raw_texset_v0>(data);
            if (*marker != ~0)
            {
                texset->fix();
                *marker = ~0;
            }

            for (auto& texName : texset->textureEntryNames)
            {
                const auto texFileName = toNchar((std::string(texName.get()) + ".texture").c_str());

                for (auto& texEntry : archive)
                {
                    if (!hl::text::equal(texEntry.name(), texFileName.data()))
                        continue;

                    data = (void*)texEntry.file_data();
                    marker = (uint32_t*)data + 1;

                    // Same logic as texset for textures.
                    if (*marker != ~0)
                        hl::hh::mirage::fix(data);

                    const auto texture = hl::hh::mirage::get_data<hl::hh::mirage::raw_texture_entry_v1>(data);
                    if (*marker != ~0)
                    {
                        texture->fix();
                        *marker = ~0;
                    }

                    createTexture(texture);
                }
            }

            break;
        }
    }

    else
    {
        for (size_t i = 0; i < material->textureEntryCount; i++)
            createTexture(material->textureEntries[i].get());
    }

    return newMaterial;
}

std::unique_ptr<Mesh> SceneFactory::createMesh(hl::hh::mirage::raw_mesh* mesh, const Affine3& transformation) const
{
    std::unique_ptr<Mesh> newMesh = std::make_unique<Mesh>();

    newMesh->vertexCount = mesh->vertexCount;
    newMesh->vertices = std::make_unique<Vertex[]>(mesh->vertexCount);

    for (size_t i = 0; i < mesh->vertexCount; i++)
    {
        auto element = &mesh->vertexElements[0];

        while (element->format != (hl::u32)hl::hh::mirage::raw_vertex_format::last_entry)
        {
            Vertex& vertex = newMesh->vertices[i];

            float* destination = nullptr;
            size_t size = 0;

            switch (element->type)
            {
            case hl::hh::mirage::raw_vertex_type::position:
                destination = vertex.position.data();
                size = vertex.position.size();
                break;

            case hl::hh::mirage::raw_vertex_type::normal:
                destination = vertex.normal.data();
                size = vertex.normal.size();
                break;

            case hl::hh::mirage::raw_vertex_type::tangent:
                destination = vertex.tangent.data();
                size = vertex.tangent.size();
                break;

            case hl::hh::mirage::raw_vertex_type::binormal:
                destination = vertex.binormal.data();
                size = vertex.binormal.size();
                break;

            case  hl::hh::mirage::raw_vertex_type::texcoord:
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

            case hl::hh::mirage::raw_vertex_type::color:
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
    
    newMesh->vertexCount = (uint32_t) meshopt_optimizeVertexFetch(
        newMesh->vertices.get(), (unsigned*) newMesh->triangles.get(), newMesh->triangleCount * 3, newMesh->vertices.get(), newMesh->vertexCount, sizeof(Vertex));

    for (auto& material : scene->materials)
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

std::unique_ptr<Model> SceneFactory::createModel(hl::hh::mirage::raw_skeletal_model_v5* model)
{
    std::unique_ptr<Model> newModel = std::make_unique<Model>();

    auto addMesh = [&](hl::hh::mirage::raw_mesh* srcMesh, const MeshType type)
    {
        std::unique_ptr<Mesh> mesh = createMesh(srcMesh, Affine3::Identity());
        mesh->type = type;

        newModel->meshes.push_back(mesh.get());

        std::lock_guard lock(criticalSection);
        scene->meshes.push_back(std::move(mesh));
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

std::unique_ptr<Instance> SceneFactory::createInstance(hl::hh::mirage::raw_terrain_instance_info_v0* instance, hl::hh::mirage::raw_terrain_model_v5* model)
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
        std::unique_ptr<Mesh> mesh = createMesh(srcMesh, transformationAffine);
        mesh->type = type;

        newInstance->meshes.push_back(mesh.get());

        std::lock_guard lock(criticalSection);
        scene->meshes.push_back(std::move(mesh));
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

std::unique_ptr<Light> SceneFactory::createLight(hl::hh::mirage::raw_light* light) const
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

std::unique_ptr<SHLightField> SceneFactory::createSHLightField(hl::hh::needle::raw_sh_light_field_node* shlf) const
{
    std::unique_ptr<SHLightField> newSHLightField = std::make_unique<SHLightField>();
    newSHLightField->name = shlf->name.get();
    newSHLightField->resolution = Eigen::Array3i(shlf->probeCounts[0], shlf->probeCounts[1], shlf->probeCounts[2]);
    newSHLightField->position = { shlf->position.x, shlf->position.y, shlf->position.z };
    newSHLightField->rotation = { shlf->rotation.x, shlf->rotation.y, shlf->rotation.z };
    newSHLightField->scale = { shlf->scale.x, shlf->scale.y, shlf->scale.z };

    return newSHLightField;
}

void SceneFactory::loadLights(const hl::archive& archive)
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

        std::lock_guard lock(criticalSection);
        scene->lights.push_back(std::move(newLight));
    }
}

void SceneFactory::loadResources(const hl::archive& archive)
{
    tbb::task_group group;

    const auto rgbTableName = toNchar((stageName + "_rgb_table0.dds").c_str());

    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".dds")) && !hl::text::strstr(entry.name(), HL_NTEXT(".DDS")))
            continue;

        group.run([&]
        {
            std::unique_ptr<Bitmap> bitmap = createBitmap(entry.file_data<uint8_t>(), entry.size());
            if (!bitmap)
            {
                Logger::logFormatted(LogType::Error, "Failed to load %s", toUtf8(entry.name()).data());
                return;
            }

            bitmap->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

            std::lock_guard lock(criticalSection);

            if (hl::text::equal(entry.name(), rgbTableName.data()))
                scene->rgbTable = std::move(bitmap);

            else
                scene->bitmaps.push_back(std::move(bitmap));
        });
    }

    group.wait();

    for (auto& entry : archive)
    {
        if (!hl::text::strstr(entry.name(), HL_NTEXT(".material")))
            continue;

        group.run([&]
        {
            void* data = (void*)entry.file_data();

            hl::hh::mirage::fix(data);

            hl::u32 version;
            auto material = hl::hh::mirage::get_data<hl::hh::mirage::raw_material_v3>(data, &version);

            std::unique_ptr<Material> newMaterial;

            if (hl::hh::mirage::has_sample_chunk_header_fixed(data) || version == 3)
            {
                material->fix();
                newMaterial = createMaterial(material, archive);
            }
            else if (version == 1)
            {
                const auto mat = reinterpret_cast<hl::hh::mirage::raw_material_v1*>(material);
                mat->fix();

                newMaterial = createMaterial(mat, archive);
            }
            else
            {
                Logger::logFormatted(LogType::Error, "Failed to load %s, unsupported version %d", toUtf8(entry.name()).data(), version);
                return;
            }

            newMaterial->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

            std::lock_guard lock(criticalSection);
            scene->materials.push_back(std::move(newMaterial));
        });
    }

    group.wait();

    for (auto& entry : archive)
    {
        if (hl::text::strstr(entry.name(), HL_NTEXT(".model")))
        {
            group.run([&]
            {
                void* data = (void*)entry.file_data();

                hl::hh::mirage::fix(data);

                hl::u32 version;
                auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_skeletal_model_v5>(data, &version);

                if (!hl::hh::mirage::has_sample_chunk_header_fixed(data) && version != 5)
                {
                    Logger::logFormatted(LogType::Error, "Failed to load %s, unsupported version %d", toUtf8(entry.name()).data(), version);
                    return;
                }

                model->fix();

                std::unique_ptr<Model> newModel = createModel(model);
                newModel->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

                std::lock_guard lock(criticalSection);
                scene->models.push_back(std::move(newModel));
            });
        }

        else if (hl::text::equal(entry.name(), HL_NTEXT("light-field.lft")))
            scene->lightField.read((void*)entry.file_data());

        else if (hl::text::strstr(entry.name(), HL_NTEXT(".shlf")))
        {
            void* data = (void*)entry.file_data();

            hl::bina::fix64(data, entry.size());

            auto shlf = hl::bina::get_data<hl::hh::needle::raw_sh_light_field>(data);

            for (size_t i = 0; i < shlf->count; i++)
                scene->shLightFields.push_back(std::move(createSHLightField(&shlf->entries[i])));
        }

        else if (hl::text::strstr(entry.name(), HL_NTEXT(".mti")))
        {
            hl::readonly_mem_stream stream(entry.file_data(), entry.size());

            std::unique_ptr<MetaInstancer> newMetaInstancer = std::make_unique<MetaInstancer>();
            newMetaInstancer->read(stream);

            if (newMetaInstancer->instances.empty())
                continue;

            newMetaInstancer->name = getFileNameWithoutExtension(toUtf8(entry.name()).data());

            scene->metaInstancers.push_back(std::move(newMetaInstancer));
        }
    }

    group.wait();
}

void SceneFactory::loadTerrain(const std::vector<hl::archive>& archives)
{
    tbb::task_group group;

    std::vector<std::pair<hl::hh::mirage::raw_terrain_model_v5*, bool>> models;
    CriticalSection critSect;

    for (auto& archive : archives)
    {
        for (auto& entry : archive)
        {
            if (!hl::text::strstr(entry.name(), HL_NTEXT(".terrain-model")))
                continue;

            group.run([&]
            {
                void* data = (void*)entry.file_data();

                hl::hh::mirage::fix(data);

                const bool hasSampleChunkHeader = hl::hh::mirage::has_sample_chunk_header_fixed(data);

                if (hasSampleChunkHeader)
                {
                    const auto header = (hl::hh::mirage::sample_chunk::raw_header*)data;
                    const auto node = header->get_node("DisableC");

                    if (node != nullptr && node->value != 0)
                        return;
                }

                hl::u32 version;
                auto model = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_model_v5>(data, &version);

                if (!hasSampleChunkHeader && version != 5)
                {
                    Logger::logFormatted(LogType::Error, "Failed to load %s, unsupported version %d", toUtf8(entry.name()).data(), version);
                    return;
                }

                model->fix();

                std::lock_guard lock(critSect);
                models.push_back(std::make_pair(model, false));
            });
        }
    }

    group.wait();

    for (auto& archive : archives)
    {
        for (auto& entry : archive)
        {
            if (!hl::text::strstr(entry.name(), HL_NTEXT(".terrain-instanceinfo")))
                continue;

            group.run([&]
            {
                void* data = (void*)entry.file_data();

                hl::hh::mirage::fix(data);

                auto instance = hl::hh::mirage::get_data<hl::hh::mirage::raw_terrain_instance_info_v0>(data);
                instance->fix();

                for (auto& pair : models)
                {
                    if (strcmp(pair.first->name.get(), instance->modelName.get()) != 0)
                        continue;

                    pair.second = true;

                    std::unique_ptr<Instance> newInstance = createInstance(instance, pair.first);

                    std::lock_guard lock(criticalSection);
                    scene->instances.push_back(std::move(newInstance));

                    break;
                }
            });
        }
    }

    group.wait();

    // Load models that aren't bound to any instances
    for (auto& model : models)
    {
        if (model.second)
            continue;

        group.run([&]
        {
            std::unique_ptr<Instance> newInstance = createInstance(nullptr, model.first);

            std::lock_guard lock(criticalSection);
            scene->instances.push_back(std::move(newInstance));
        });
    }

    group.wait();
}

void SceneFactory::loadResolutions(const hl::archive& archive) const
{
    struct Res
    {
        uint32_t height;
        uint32_t width;
    };

    std::unordered_map<std::string, Res> resolutions;

    for (auto& entry : archive)
    {
        const hl::nchar* substr = hl::text::strstr(entry.name(), HL_NTEXT(".dds"));

        if (!substr)
            substr = hl::text::strstr(entry.name(), HL_NTEXT(".DDS"));

        if (!substr)
            continue;

        char name[0x400]{};
        WideCharToMultiByte(CP_UTF8, 0, entry.name(), (int)(substr - entry.name()), name, sizeof(name), nullptr, nullptr);

        resolutions[name] = *(Res*)((char*)entry.file_data() + 0xC);
    }

    for (auto& entry : archive)
    {
        if (!hl::text::equal(entry.name(), HL_NTEXT("atlasinfo")))
            continue;
        
        hl::readonly_mem_stream stream(entry.file_data(), entry.size());

        hl::hh::mirage::atlas_info atlasInfo;
        atlasInfo.read(stream);

        for (auto& atlas : atlasInfo.atlases)
        {
            const auto& res = resolutions.find(atlas.name);
            if (res == resolutions.end())
                continue;

            for (auto& texture : atlas.textures)
            {
                resolutions[texture.name] = {
                    (uint32_t)(res->second.height * texture.height), (uint32_t)(res->second.width * texture.width) };
            }
        }

        break;
    }

    for (auto& instance : scene->instances)
    {
        if (instance->originalResolution > 0)
            continue;

        constexpr const char* suffixes[] = 
        {
            "-level2",
            "_sg-level2",
            "_occlusion-level2",

            "_sg",
            "_occlusion",
            ""
        };

        for (size_t i = 0; i < _countof(suffixes); i++)
        {
            const auto& res = resolutions.find(instance->name + suffixes[i]);
            if (res == resolutions.end())
                continue;

            instance->originalResolution = std::max(res->second.width, res->second.height);

            if (i < 3) // -level2
                instance->originalResolution *= 4;

            break;
        }
    }
}

void SceneFactory::loadSceneEffect(const hl::archive& archive) const
{
    auto hhdName = toNchar((stageName + ".hhd").c_str());
    auto rflName = toNchar((stageName + ".rfl").c_str());

    for (auto& entry : archive)
    {
        if (hl::text::equal(entry.name(), HL_NTEXT("SceneEffect.prm.xml")))
        {
            tinyxml2::XMLDocument doc;

            if (doc.Parse(entry.file_data<char>(), entry.size()) == tinyxml2::XML_SUCCESS)
                scene->effect.load(doc);

            break;
        }

        if (hl::text::equal(entry.name(), hhdName.data()))
        {
            hl::bina::fix32((void*)entry.file_data(), entry.size());

            if (const auto data = hl::bina::get_data<sonic2013::FxSceneData>(entry.file_data()); data)
                scene->effect.load(*data);

            break;
        }

        if (hl::text::equal(entry.name(), rflName.data()))
        {
            hl::bina::fix64((void*)entry.file_data(), entry.size());

            if (const auto data = hl::bina::get_data<wars::NeedleFxSceneData>(entry.file_data()); data)
                scene->effect.load(*data);

            break;
        }
    }
}

void SceneFactory::createFromUnleashedOrGenerations(const std::string& directoryPath)
{
    // Load resources
    const auto rootDirPath = directoryPath + "/../../";
    const auto packedDirPath = directoryPath + "/";
    
    const auto highPrioArFilePath = rootDirPath + "#" + stageName + ".ar.00";
    {
        hl::archive archive;

        const auto loadArchiveIfExist = [&](const std::string& filePath)
        {
            if (std::filesystem::exists(filePath))
                loadArchive(archive, toNchar(filePath.c_str()).data());
        };

        const auto resArchiveFilePath = packedDirPath + stageName + ".ar.00";

        if (!std::filesystem::exists(resArchiveFilePath)) // Very likely Unleashed
        {
            loadArchiveIfExist(rootDirPath + stageName + ".ar.00");
            loadArchiveIfExist(highPrioArFilePath);

            std::string type;
            std::string region;

            // Try extracting the stage type and region.
            size_t index = stageName.find("Act");
            if (index != std::string::npos)
            {
                const char typeChar = stageName[index + 3];
                if (typeChar == 'D' || typeChar == 'N')
                    type = typeChar;
            }

            index = stageName.find('_');
            if (index != std::string::npos)
            {
                region = stageName.substr(index + 1);

                index = region.find("Sub");
                if (index != std::string::npos)
                    region.erase(index, 3);

                const auto eraseSuffix = [&](const char* suffix)
                {
                    index = region.find(suffix);
                    if (index != std::string::npos)
                        region.erase(index, region.size() - index);
                };

                eraseSuffix("_");
                eraseSuffix("Act1");
                eraseSuffix("Act2");
                eraseSuffix("Evil");
            }

            if (!region.empty())
            {
                loadArchiveIfExist(rootDirPath + "CmnAct_" + region + ".ar.00");
                if (!type.empty())
                    loadArchiveIfExist(rootDirPath + "CmnAct" + type + "_Terrain_" + region + ".ar.00");

                loadArchiveIfExist(rootDirPath + "Cmn" + region + ".ar.00");
            }
        }

        else // Generations
        {
            loadArchiveIfExist(resArchiveFilePath);
            loadArchiveIfExist(highPrioArFilePath);
        }

        loadResources(archive);
        loadLights(archive);
        loadSceneEffect(archive);

        scene->createLightBVH();
    }

    // Load packed file data
    {
        tbb::task_group group;

        auto pfdArchive = hl::hh::pfd::load(toNchar((directoryPath + "/Stage.pfd").c_str()).data());

        for (auto& entry : pfdArchive)
        {
            if (!hl::text::strstr(entry.name(), HL_NTEXT("tg-")))
                continue;

            group.run([&]
            {
                loadTerrain({ ArchiveCompression::load(entry.file_data(), entry.size()) });
            });
        }

        group.wait();

        for (auto& entry : pfdArchive)
        {
            if (!hl::text::strstr(entry.name(), HL_NTEXT("gia-")) && !hl::text::strstr(entry.name(), HL_NTEXT("gi-texture-")))
                continue;

            group.run([&]
            {
                loadResolutions({ ArchiveCompression::load(entry.file_data(), entry.size()) });
            });
        }

        group.wait();
    }

    scene->sortAndUnify();
    scene->buildAABB();
}

void SceneFactory::createFromLostWorldOrForces(const std::string& directoryPath)
{
    std::vector<hl::archive> archives;
    {
        auto filePath = toNchar((directoryPath + "/" + stageName + "_trr_cmn.pac").c_str());
        auto archive = hl::pacx::load(filePath.data());

        loadResources(archive);
        archives.push_back(std::move(archive));
    }

    tbb::task_group group;

    for (size_t i = 0; i < 100; i++)
    {
        char slot[4];
        sprintf(slot, "%02lld", i);

        auto filePath = toNchar((directoryPath + "/" + stageName + "_trr_s" + slot + ".pac").c_str());

        if (!hl::path::exists(filePath.data()))
            continue;

        group.run([&, filePath]
        {
            hl::archive archive = hl::pacx::load(filePath.data());

            std::lock_guard lock(criticalSection);
            archives.push_back(std::move(archive));
        });
    }

    group.wait();

    loadTerrain(archives);
    
    // Load lights/resolutions from sectors
    for (auto& archive : archives)
    {
        loadLights(archive);
        loadResolutions(archive);
    }

    archives.clear();

    auto skyFilePath = toNchar((directoryPath + "/" + stageName + "_sky.pac").c_str());

    if (hl::path::exists(skyFilePath.data()))
        loadResources(hl::pacx::load(skyFilePath.data()));

    scene->sortAndUnify();
    scene->buildAABB();
    scene->createLightBVH();

    auto miscFilePath = toNchar((directoryPath + "/" + stageName + "_misc.pac").c_str());

    if (hl::path::exists(miscFilePath.data()))
        loadSceneEffect(hl::pacx::load(miscFilePath.data()));
}

std::unique_ptr<Scene> SceneFactory::create(const std::string& directoryPath)
{
    SceneFactory factory;

    factory.scene = std::make_unique<Scene>();
    factory.stageName = getFileNameWithoutExtension(directoryPath);

    if (std::filesystem::exists(directoryPath + "/Stage.pfd"))
        factory.createFromUnleashedOrGenerations(directoryPath);
    else
        factory.createFromLostWorldOrForces(directoryPath);

    return std::move(factory.scene);
}
