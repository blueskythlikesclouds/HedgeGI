#include "SceneFactory.h"
#include "Bitmap.h"
#include "Instance.h"
#include "Material.h"
#include "Scene.h"

std::unique_ptr<Bitmap> SceneFactory::createBitmap(const uint8_t* data, const size_t length)
{
    std::unique_ptr<DirectX::ScratchImage> scratchImage = std::make_unique<DirectX::ScratchImage>();

    DirectX::TexMetadata metadata;
    LoadFromDDSMemory(data, length, DirectX::DDS_FLAGS_NONE, &metadata, *scratchImage);

    if (!scratchImage->GetImages())
        return nullptr;

    if (DirectX::IsCompressed(metadata.format))
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Decompress(*scratchImage->GetImage(std::min<size_t>(2, metadata.mipLevels - 1), 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, *newScratchImage); // Try getting the second mip (we don't need much quality from textures)
        scratchImage.swap(newScratchImage);
    }
    else if (metadata.format != DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Convert(*scratchImage->GetImage(std::min<size_t>(2, metadata.mipLevels - 1), 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }

    if (!scratchImage->GetImages())
        return nullptr;

    metadata = scratchImage->GetMetadata();

    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>();

    bitmap->type =
        ((metadata.miscFlags & DirectX::TEX_MISC_TEXTURECUBE) != 0) ? BITMAP_TYPE_CUBE :
        metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D ? BITMAP_TYPE_3D :
        BITMAP_TYPE_3D;

    bitmap->width = (uint32_t)metadata.width;
    bitmap->height = (uint32_t)metadata.height;
    bitmap->arraySize = bitmap->type == BITMAP_TYPE_3D ? (uint32_t)metadata.depth : (uint32_t)metadata.arraySize;
    bitmap->data = std::make_unique<Eigen::Array4f[]>(bitmap->width * bitmap->height * bitmap->arraySize);

    for (size_t i = 0; i < bitmap->arraySize; i++)
        memcpy(&bitmap->data[bitmap->width * bitmap->height * i], scratchImage->GetImage(0, i, 0)->pixels, bitmap->width * bitmap->height * sizeof(Eigen::Vector4f));

    return bitmap;
}

std::unique_ptr<Material> SceneFactory::createMaterial(HlHHMaterialV3* material, const Scene& scene)
{
    std::unique_ptr<Material> newMaterial = std::make_unique<Material>();

    HL_OFF32(HlHHMaterialParameter)* parameters = (HL_OFF32(HlHHMaterialParameter)*)hlOff32Get(&material->vec4ParamsOffset);

    for (size_t i = 0; i < material->vec4ParamCount; i++)
    {
        HlHHMaterialParameter* parameter = (HlHHMaterialParameter*)hlOff32Get(&parameters[i]);
        char* name = (char*)hlOff32Get(&parameter->nameOffset);
        float* valuePtr = (float*)hlOff32Get(&parameter->valuesOffset);
        const Eigen::Array4f value(valuePtr[0], valuePtr[1], valuePtr[2], valuePtr[3]);

        if (strcmp(name, "diffuse") == 0) newMaterial->parameters.diffuse = value;
        else if (strcmp(name, "specular") == 0) newMaterial->parameters.specular = value;
        else if (strcmp(name, "ambient") == 0) newMaterial->parameters.ambient = value;
        else if (strcmp(name, "power_gloss_level") == 0) newMaterial->parameters.powerGlossLevel = value;
        else if (strcmp(name, "mrgLuminanceRange") == 0) newMaterial->parameters.luminanceRange = value;
        else if (strcmp(name, "Luminance") == 0) newMaterial->parameters.luminance = value;
        else if (strcmp(name, "PBRFactor") == 0) newMaterial->parameters.pbrFactor = value;
        else if (strcmp(name, "PBRFactor2") == 0) newMaterial->parameters.pbrFactor2 = value;
    }

    HL_OFF32(HlHHTextureV1)* textures = (HL_OFF32(HlHHTextureV1)*)hlOff32Get(&material->texturesOffset);

    for (size_t i = 0; i < material->textureCount; i++)
    {
        HlHHTextureV1* texture = (HlHHTextureV1*)hlOff32Get(&textures[i]);
        char* type = (char*)hlOff32Get(&texture->typeOffset);
        char* fileName = (char*)hlOff32Get(&texture->fileNameOffset);

        const Bitmap* bitmap = nullptr;

        for (auto& item : scene.bitmaps)
        {
            if (strcmp(fileName, item->name.c_str()) != 0)
                continue;

            bitmap = item.get();
            break;
        }

        if (bitmap == nullptr)
            continue;

        if (strcmp(type, "diffuse") == 0)
        {
            if (newMaterial->textures.diffuse != nullptr)
                newMaterial->textures.diffuseBlend = bitmap;
            else
                newMaterial->textures.diffuse = bitmap;
        }
        else if (strcmp(type, "specular") == 0)
        {
            if (newMaterial->textures.specular != nullptr)
                newMaterial->textures.specularBlend = bitmap;
            else
                newMaterial->textures.specular = bitmap;
        }
        else if (strcmp(type, "gloss") == 0)
        {
            if (newMaterial->textures.gloss != nullptr)
                newMaterial->textures.glossBlend = bitmap;
            else
                newMaterial->textures.gloss = bitmap;
        }
        else if (strcmp(type, "opacity") == 0 ||
            strcmp(type, "transparency") == 0)
        {
            newMaterial->textures.alpha = bitmap;
        }
        else if (strcmp(type, "displacement") == 0 ||
            strcmp(type, "emission") == 0)
        {
            newMaterial->textures.emission = bitmap;
        }
        else if (strcmp(type, "reflection") == 0)
        {
            newMaterial->textures.environment = bitmap;
        }
    }

    return newMaterial;
}

std::unique_ptr<Mesh> SceneFactory::createMesh(HlHHMesh* mesh, const Eigen::Affine3f& transformation,
                                               const Scene& scene)
{
    std::unique_ptr<Mesh> newMesh = std::make_unique<Mesh>();

    newMesh->vertexCount = mesh->vertexCount;
    newMesh->vertices = std::make_unique<Vertex[]>(mesh->vertexCount);

    HlU8* vertices = (HlU8*)hlOff32Get(&mesh->verticesOffset);

    for (size_t i = 0; i < mesh->vertexCount; i++)
    {
        HlHHVertexElement* element = (HlHHVertexElement*)hlOff32Get(&mesh->vertexElementsOffset);

        while (element->format != HL_HH_VERTEX_FORMAT_LAST_ENTRY)
        {
            Vertex& vertex = newMesh->vertices[i];

            float* destination = nullptr;
            size_t size = 0;

            switch (element->type)
            {
            case HL_HH_VERTEX_TYPE_POSITION:
                destination = vertex.position.data();
                size = vertex.position.size();
                break;

            case HL_HH_VERTEX_TYPE_NORMAL:
                destination = vertex.normal.data();
                size = vertex.normal.size();
                break;

            case HL_HH_VERTEX_TYPE_TEXCOORD:
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

            case HL_HH_VERTEX_TYPE_COLOR:
                destination = vertex.color.data();
                size = vertex.color.size();
                break;
            }

            if (!destination)
            {
                element++;
                continue;
            }

            HlU8* source = vertices + i * mesh->vertexSize + element->offset;

            switch (element->format)
            {
            case HL_HH_VERTEX_FORMAT_VECTOR2:
                size = std::min<size_t>(size, 2);

            case HL_HH_VERTEX_FORMAT_VECTOR3:
                size = std::min<size_t>(size, 3);

            case HL_HH_VERTEX_FORMAT_VECTOR4:
                size = std::min<size_t>(size, 4);

                for (size_t j = 0; j < size; j++)
                    destination[j] = ((float*)source)[j];

                break;

            case HL_HH_VERTEX_FORMAT_VECTOR4_BYTE:
                size = std::min<size_t>(size, 4);

                for (size_t j = 0; j < size; j++)
                    destination[j] = (float)source[j] / 255.0f;

                break;

            case HL_HH_VERTEX_FORMAT_VECTOR2_HALF:

                for (size_t j = 0; j < 2; j++)
                    destination[j] = half_to_float(((uint16_t*)source)[j]);

                break;

            case HL_HH_VERTEX_FORMAT_VECTOR3_HH2:
                const uint32_t value = ((uint32_t*)source)[0];

                destination[0] = ((value & 0x00000200 ? -1 : 0) + (float)((value) & 0x1FF) / 512.0f);
                destination[1] = ((value & 0x00080000 ? -1 : 0) + (float)((value >> 10) & 0x1FF) / 512.0f);
                destination[2] = ((value & 0x20000000 ? -1 : 0) + (float)((value >> 20) & 0x1FF) / 512.0f);

                break;
            }

            element++;
        }
    }

    Eigen::Affine3f rotation;
    rotation = transformation.rotation();

    for (size_t i = 0; i < newMesh->vertexCount; i++)
    {
        Vertex& vertex = newMesh->vertices[i];

        vertex.position = transformation * vertex.position;
        vertex.normal = (rotation * vertex.normal).normalized();
    }

    std::vector<Triangle> triangles;
    triangles.reserve(mesh->faceCount);

    size_t i = 0;
    HlU16* faces = (HlU16*)hlOff32Get(&mesh->facesOffset);

    HlU16 a = faces[i++];
    HlU16 b = faces[i++];
    int32_t direction = -1;

    while (i < mesh->faceCount)
    {
        const HlU16 c = faces[i++];
        if (c == (HlU16)-1)
        {
            a = faces[i++];
            b = faces[i++];
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

    const char* materialName = (const char*)hlOff32Get(&mesh->materialNameOffset);

    for (auto& material : scene.materials)
    {
        if (strcmp(material->name.c_str(), materialName) != 0)
            continue;

        newMesh->material = material.get();
        break;
    }

    newMesh->generateTangents();
    newMesh->buildAABB();

    return newMesh;
}

std::unique_ptr<Instance> SceneFactory::createInstance(HlHHTerrainInstanceInfoV0* instance, HlHHTerrainModelV5* model,
                                                       Scene& scene)
{
    std::unique_ptr<Instance> newInstance = std::make_unique<Instance>();

    newInstance->name = (const char*)(instance ?
        hlOff32Get(&instance->instanceInfoNameOffset) : hlOff32Get(&model->nameOffset));

    Eigen::Matrix4f transformationMatrix;

    if (instance != nullptr)
    {
        HlMatrix4x4* matrix = (HlMatrix4x4*)hlOff32Get(&instance->matrixOffset);

        transformationMatrix <<
            matrix->m11, matrix->m12, matrix->m13, matrix->m14,
            matrix->m21, matrix->m22, matrix->m23, matrix->m24,
            matrix->m31, matrix->m32, matrix->m33, matrix->m34,
            matrix->m41, matrix->m42, matrix->m43, matrix->m44;
    }
    else
    {
        transformationMatrix = Eigen::Matrix4f::Identity();
    }

    Eigen::Affine3f transformationAffine;
    transformationAffine = transformationMatrix;

    auto addMesh = [&transformationAffine, &scene, &newInstance](HlHHMesh* srcMesh, const MeshType type)
    {
        std::unique_ptr<Mesh> mesh = createMesh(srcMesh, transformationAffine, scene);
        mesh->type = type;

        newInstance->meshes.push_back(mesh.get());

        scene.meshes.push_back(std::move(mesh));
    };

    // me when hlOff32Get

    HL_OFF32(HlHHMeshGroup)* meshGroups = (HL_OFF32(HlHHMeshGroup)*)hlOff32Get(&model->meshGroupsOffset);

    for (size_t i = 0; i < model->meshGroupCount; i++)
    {
        HlHHMeshGroup* meshGroup = (HlHHMeshGroup*)hlOff32Get(&meshGroups[i]);

        HL_OFF32(HlHHMesh)* solidMeshes = (HL_OFF32(HlHHMesh)*)hlOff32Get(&meshGroup->solid.meshesOffset);
        HL_OFF32(HlHHMesh)* transparentMeshes = (HL_OFF32(HlHHMesh)*)hlOff32Get(&meshGroup->transparent.meshesOffset);
        HL_OFF32(HlHHMesh)* punchMeshes = (HL_OFF32(HlHHMesh)*)hlOff32Get(&meshGroup->punch.meshesOffset);

        for (size_t j = 0; j < meshGroup->solid.meshCount; j++)
            addMesh((HlHHMesh*)hlOff32Get(&solidMeshes[j]), MESH_TYPE_OPAQUE);

        for (size_t j = 0; j < meshGroup->transparent.meshCount; j++)
            addMesh((HlHHMesh*)hlOff32Get(&transparentMeshes[j]), MESH_TYPE_TRANSPARENT);

        for (size_t j = 0; j < meshGroup->punch.meshCount; j++)
            addMesh((HlHHMesh*)hlOff32Get(&punchMeshes[j]), MESH_TYPE_PUNCH);

        HL_OFF32(HlU32)* specialMeshGroupCounts = (HL_OFF32(HlU32)*)hlOff32Get(&meshGroup->special.meshCounts);
        HL_OFF32(HL_OFF32(HlHHMesh))* specialMeshGroupOffsets = (HL_OFF32(HL_OFF32(HlHHMesh))*)hlOff32Get(&meshGroup->special.meshesOffset);

        for (size_t j = 0; j < meshGroup->special.count; j++)
        {
            HlU32* specialMeshGroupCount = (HlU32*)hlOff32Get(&specialMeshGroupCounts[j]);
            HL_OFF32(HlHHMesh)* specialMeshGroupOffset = (HL_OFF32(HlHHMesh)*)hlOff32Get(&specialMeshGroupOffsets[j]);

            for (size_t k = 0; k < *specialMeshGroupCount; k++)
                addMesh((HlHHMesh*)hlOff32Get(&specialMeshGroupOffset[k]), MESH_TYPE_SPECIAL);
        }
    }

    newInstance->buildAABB();
    return newInstance;
}

std::unique_ptr<Light> SceneFactory::createLight(HlHHLightV1* light)
{
    std::unique_ptr<Light> newLight = std::make_unique<Light>();

    newLight->type = (LightType)light->type;
    newLight->positionOrDirection[0] = light->position.x;
    newLight->positionOrDirection[1] = light->position.y;
    newLight->positionOrDirection[2] = light->position.z;
    newLight->color[0] = light->color.x;
    newLight->color[1] = light->color.y;
    newLight->color[2] = light->color.z;

    if (newLight->type != LIGHT_TYPE_POINT)
    {
        newLight->positionOrDirection.normalize();
        return newLight;
    }

    newLight->innerRange = light->innerRange;
    newLight->outerRange = light->outerRange;

    return newLight;
}

std::unique_ptr<SHLightField> SceneFactory::createSHLightField(HlHHSHLightField* shlf)
{
    Eigen::Affine3f affine =
        Eigen::Translation3f(shlf->position.x, shlf->position.y, shlf->position.z) *
        Eigen::AngleAxisf(shlf->rotation.x, Eigen::Vector3f::UnitX()) *
        Eigen::AngleAxisf(shlf->rotation.y, Eigen::Vector3f::UnitY()) *
        Eigen::AngleAxisf(shlf->rotation.z, Eigen::Vector3f::UnitZ()) *
        Eigen::Scaling(shlf->scale.x, shlf->scale.y, shlf->scale.z);

    std::unique_ptr<SHLightField> newSHLightField = std::make_unique<SHLightField>();
    newSHLightField->name = (char*)hlOff64Get(&shlf->name);
    newSHLightField->resolution = Eigen::Array3i(shlf->probeCounts[0], shlf->probeCounts[1], shlf->probeCounts[2]);
    newSHLightField->matrix = affine.matrix();

    return newSHLightField;
}

void SceneFactory::loadResources(HlArchive* archive, Scene& scene)
{
    for (size_t i = 0; i < archive->entries.count; i++)
    {
        HlArchiveEntry* entry = &archive->entries.data[i];

        if (!hlNStrStr(entry->path, HL_NTEXT(".dds")))
            continue;

        std::unique_ptr<Bitmap> bitmap = createBitmap((uint8_t*)entry->data, entry->size);
        if (!bitmap)
            continue;

        char name[MAX_PATH];
        hlStrConvNativeToUTF8NoAlloc(entry->path, name, 0, MAX_PATH);
        bitmap->name = getFileNameWithoutExtension(name);

        scene.bitmaps.push_back(std::move(bitmap));
    }

    for (size_t i = 0; i < archive->entries.count; i++)
    {
        HlArchiveEntry* entry = &archive->entries.data[i];

        if (!hlNStrStr(entry->path, HL_NTEXT(".material")))
            continue;

        hlHHFix((void*)entry->data);

        uint32_t version;
        HlHHMaterialV3* material = (HlHHMaterialV3*)hlHHGetData((void*)entry->data, &version);

        if (version != 3)
            continue;

        hlHHMaterialV3Fix(material);

        std::unique_ptr<Material> newMaterial = createMaterial(material, scene);

        char name[MAX_PATH];
        hlStrConvNativeToUTF8NoAlloc(entry->path, name, 0, MAX_PATH);
        newMaterial->name = getFileNameWithoutExtension(name);

        scene.materials.push_back(std::move(newMaterial));
    }

    for (size_t i = 0; i < archive->entries.count; i++)
    {
        HlArchiveEntry* entry = &archive->entries.data[i];

        if (!hlNStrStr(entry->path, HL_NTEXT(".light")) || hlNStrStr(entry->path, HL_NTEXT(".light-list")))
            continue;

        hlHHFix((void*)entry->data);

        HlHHLightV1* light = (HlHHLightV1*)hlHHGetData((void*)entry->data, nullptr);
        hlHHLightV1Fix(light);

        scene.lights.push_back(std::move(createLight(light)));
    }

    for (size_t i = 0; i < archive->entries.count; i++)
    {
        HlArchiveEntry* entry = &archive->entries.data[i];

        if (!hlNStrStr(entry->path, HL_NTEXT(".shlf")))
            continue;

        hlBINAV2Fix((void*)entry->data, entry->size);

        HlHHSHLightFieldSet* shlfSet = (HlHHSHLightFieldSet*)hlBINAV2GetData((void*)entry->data);
        HlHHSHLightField* items = (HlHHSHLightField*)hlOff64Get(&shlfSet->items);

        for (size_t j = 0; j < shlfSet->count; j++)
            scene.shLightFields.push_back(std::move(createSHLightField(&items[j])));
    }
}

void SceneFactory::loadTerrain(HlArchive* archive, Scene& scene)
{
    std::vector<HlHHTerrainModelV5*> models;

    for (size_t i = 0; i < archive->entries.count; i++)
    {
        HlArchiveEntry* entry = &archive->entries.data[i];

        if (!hlNStrStr(entry->path, HL_NTEXT(".terrain-model")))
            continue;

        hlHHFix((void*)entry->data);

        HlHHTerrainModelV5* model = (HlHHTerrainModelV5*)hlHHGetData((void*)entry->data, nullptr);
        hlHHTerrainModelV5Fix(model);
        
        model->flags = false;
        models.push_back(model);
    }

    for (size_t i = 0; i < archive->entries.count; i++)
    {
        HlArchiveEntry* entry = &archive->entries.data[i];

        if (!hlNStrStr(entry->path, HL_NTEXT(".terrain-instanceinfo")))
            continue;

        hlHHFix((void*)entry->data);

        HlHHTerrainInstanceInfoV0* instance = (HlHHTerrainInstanceInfoV0*)hlHHGetData((void*)entry->data, nullptr);
        hlHHTerrainInstanceInfoV0Fix(instance);

        const char* instanceModelName = (const char*)hlOff32Get(&instance->modelNameOffset);

        for (auto& model : models)
        {
            const char* modelName = (const char*)hlOff32Get(&model->nameOffset);

            if (strcmp(modelName, instanceModelName) != 0)
                continue;

            model->flags = true;

            scene.instances.push_back(createInstance(instance, model, scene));
            break;
        }
    }

    // Load models that aren't bound to any instances
    for (auto& model : models)
    {
        if (model->flags)
            continue;

        scene.instances.push_back(createInstance(nullptr, model, scene));
    }

    scene.buildAABB();
}

std::unique_ptr<Scene> SceneFactory::createFromGenerations(const std::string& directoryPath)
{
    std::unique_ptr<Scene> scene = std::make_unique<Scene>();

    HlNChar filePath[MAX_PATH];
    hlStrConvUTF8ToNativeNoAlloc((directoryPath + "/" + getFileNameWithoutExtension(directoryPath) + ".ar.00").c_str(), filePath, 0, MAX_PATH);

    HlArchive* resArchive = nullptr;
    hlHHArchiveLoad(filePath, true, nullptr, &resArchive);

    loadResources(resArchive, *scene);

    HlHHPackedFileIndexV0* pfi = nullptr;

    for (size_t i = 0; i < resArchive->entries.count; i++)
    {
        HlArchiveEntry* entry = &resArchive->entries.data[i];

        if (!hlNStrsEqual(entry->path, HL_NTEXT("Stage.pfi")))
            continue;

        hlHHFix((void*)entry->data);

        pfi = (HlHHPackedFileIndexV0*)hlHHGetData((void*)entry->data, nullptr);
        hlHHPackedFileIndexV0Swap(pfi, false);
    }

    if (!pfi)
    {
        hlArchiveFree(resArchive);
        return nullptr;
    }

    const FileStream pfdFile((directoryPath + "/Stage.pfd").c_str(), "rb");

    HL_OFF32(HlHHPackedFileEntry)* entries = (HL_OFF32(HlHHPackedFileEntry)*)hlOff32Get(&pfi->entriesOffset);

    for (size_t i = 0; i < pfi->entryCount; i++)
    {
        HlHHPackedFileEntry* entry = (HlHHPackedFileEntry*)hlOff32Get(&entries[i]);
        hlHHPackedFileEntrySwap(entry, false);

        const char* fileName = (const char*)hlOff32Get(&entry->nameOffset);

        if (!strstr(fileName, "tg-"))
            continue;

        std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(entry->dataSize);
        pfdFile.seek(entry->dataPos, SEEK_SET);
        pfdFile.read(data.get(), entry->dataSize);

        {
            const FileStream tgFile("temp.cab", "wb");
            tgFile.write(data.get(), entry->dataSize);
        }

        std::system("expand temp.cab temp.ar > nul");

        HlArchive* tgArchive = nullptr;
        hlHHArchiveLoad(HL_NTEXT("temp.ar"), false, nullptr, &tgArchive);

        loadTerrain(tgArchive, *scene);

        hlArchiveFree(tgArchive);
    }

    hlArchiveFree(resArchive);

    std::system("del temp.cab");
    std::system("del temp.ar");

    scene->removeUnusedBitmaps();
    return scene;
}

std::unique_ptr<Scene> SceneFactory::createFromLostWorldOrForces(const std::string& directoryPath)
{
    std::unique_ptr<Scene> scene = std::make_unique<Scene>();

    const std::string stageName = getFileNameWithoutExtension(directoryPath);
    {
        HlNChar filePath[MAX_PATH];
        hlStrConvUTF8ToNativeNoAlloc((directoryPath + "/" + stageName + "_trr_cmn.pac").c_str(), filePath, 0, MAX_PATH);

        HlArchive* archive = nullptr;
        hlPACxLoad(filePath, 0, true, nullptr, &archive);

        loadResources(archive, *scene);
        loadTerrain(archive, *scene);

        hlArchiveFree(archive);
    }

    for (uint32_t i = 0; i < 100; i++)
    {
        char slot[4];
        sprintf(slot, "%02d", i);

        HlNChar filePath[MAX_PATH];
        hlStrConvUTF8ToNativeNoAlloc((directoryPath + "/" + stageName + "_trr_s" + slot + ".pac").c_str(), filePath, 0, MAX_PATH);

        if (!hlPathExists(filePath))
            continue;

        HlArchive* archive = nullptr;
        hlPACxLoad(filePath, 0, true, nullptr, &archive);

        loadTerrain(archive, *scene);

        hlArchiveFree(archive);
    }

    scene->removeUnusedBitmaps();
    return scene;
}
