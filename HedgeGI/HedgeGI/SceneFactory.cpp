#include "SceneFactory.h"
#include "Bitmap.h"
#include "HHPackedFileInfo.h"
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
        Decompress(*scratchImage->GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }
    else if (metadata.format != DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        std::unique_ptr<DirectX::ScratchImage> newScratchImage = std::make_unique<DirectX::ScratchImage>();
        Convert(*scratchImage->GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *newScratchImage);
        scratchImage.swap(newScratchImage);
    }

    if (!scratchImage->GetImages())
        return nullptr;

    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>();
    bitmap->width = (uint32_t)metadata.width;
    bitmap->height = (uint32_t)metadata.height;
    bitmap->arraySize = 1;
    bitmap->data = std::make_unique<Eigen::Vector4f[]>(bitmap->width * bitmap->height * bitmap->arraySize);

    for (size_t i = 0; i < bitmap->arraySize; i++)
        memcpy(&bitmap->data[bitmap->width * bitmap->height * i], scratchImage->GetImage(0, i, 0)->pixels, bitmap->width * bitmap->height * sizeof(Eigen::Vector4f));

    return bitmap;
}

std::unique_ptr<Material> SceneFactory::createMaterial(hl::HHMaterialV3* material, const Scene& scene)
{
    std::unique_ptr<Material> newMaterial = std::make_unique<Material>();

    for (size_t i = 0; i < material->TextureCount; i++)
    {
        hl::HHTexture* unit = material->Textures.Get()[i].Get();
        if (strcmp(unit->Type.Get(), "diffuse") != 0 &&
            strcmp(unit->Type.Get(), "displacement") != 0)
            continue;

        bool found = false;
        for (auto& bitmap : scene.bitmaps)
        {
            if (strcmp(unit->TextureName.Get(), bitmap->name.c_str()) != 0)
                continue;

            if (strcmp(unit->Type.Get(), "diffuse") != 0)
                newMaterial->diffuse = bitmap.get();
            else
                newMaterial->emission = bitmap.get();

            found = true;
            break;
        }

        if (found)
            break;
    }

    return newMaterial;
}

std::unique_ptr<Mesh> SceneFactory::createMesh(hl::HHMesh* mesh, const Eigen::Affine3f& transformation,
                                               const Scene& scene)
{
    std::unique_ptr<Mesh> newMesh = std::make_unique<Mesh>();

    newMesh->vertexCount = mesh->VertexCount;
    newMesh->vertices = std::make_unique<Vertex[]>(mesh->VertexCount);

    for (size_t i = 0; i < mesh->VertexCount; i++)
    {
        hl::HHVertexElement* element = mesh->VertexElements;
        while (element->Format != hl::HHVERTEX_FORMAT_LAST_ENTRY)
        {
            Vertex& vertex = newMesh->vertices[i];

            float* destination = nullptr;
            size_t size = 0;

            switch (element->Type)
            {
            case hl::HHVERTEX_TYPE_POSITION:
                destination = vertex.position.data();
                size = vertex.position.size();
                break;

            case hl::HHVERTEX_TYPE_NORMAL:
                destination = vertex.normal.data();
                size = vertex.normal.size();
                break;

            case hl::HHVERTEX_TYPE_UV:
                if (element->Index == 0)
                {
                    destination = vertex.uv.data();
                    size = vertex.uv.size();
                }

                else if (element->Index == 1)
                {
                    destination = vertex.vPos.data();
                    size = vertex.vPos.size();
                }

                break;

            case hl::HHVERTEX_TYPE_COLOR:
                destination = vertex.color.data();
                size = vertex.color.size();
                break;
            }

            if (!destination)
            {
                element++;
                continue;
            }

            uint8_t* source = mesh->Vertices + i * mesh->VertexSize + element->Offset;

            switch (element->Format)
            {
            case hl::HHVERTEX_FORMAT_VECTOR2:
                size = std::min<size_t>(size, 2);

            case hl::HHVERTEX_FORMAT_VECTOR3:
                size = std::min<size_t>(size, 3);

            case hl::HHVERTEX_FORMAT_VECTOR4:
                size = std::min<size_t>(size, 4);

                for (size_t j = 0; j < size; j++)
                    destination[j] = ((float*)source)[j];

                break;

            case hl::HHVERTEX_FORMAT_VECTOR4_BYTE:
                size = std::min<size_t>(size, 4);

                for (size_t j = 0; j < size; j++)
                    destination[j] = (float)source[j] / 255.0f;

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
        vertex.tangent = (rotation * vertex.tangent).normalized();
        vertex.binormal = (rotation * vertex.binormal).normalized();
    }

    std::vector<Triangle> triangles;
    triangles.reserve(mesh->Faces.Count);

    size_t i = 0;

    uint16_t a = mesh->Faces[i++];
    uint16_t b = mesh->Faces[i++];
    int32_t direction = -1;

    while (i < mesh->Faces.Count)
    {
        const uint16_t c = mesh->Faces[i++];
        if (c == (uint16_t)-1)
        {
            a = mesh->Faces[i++];
            b = mesh->Faces[i++];
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
        if (strcmp(material->name.c_str(), mesh->MaterialName.Get()) != 0)
            continue;

        newMesh->material = material.get();
        break;
    }

    newMesh->generateTangents();
    return newMesh;
}

std::unique_ptr<Instance> SceneFactory::createInstance(hl::HHTerrainInstanceInfoV0* instance, hl::HHTerrainModel* model,
                                                       Scene& scene)
{
    std::unique_ptr<Instance> newInstance = std::make_unique<Instance>();

    newInstance->name = instance->Filename.Get();

    Eigen::Matrix4f transformationMatrix;
    transformationMatrix <<
        instance->Matrix->M11, instance->Matrix->M12, instance->Matrix->M13, instance->Matrix->M14,
        instance->Matrix->M21, instance->Matrix->M22, instance->Matrix->M23, instance->Matrix->M24,
        instance->Matrix->M31, instance->Matrix->M32, instance->Matrix->M33, instance->Matrix->M34,
        instance->Matrix->M41, instance->Matrix->M42, instance->Matrix->M43, instance->Matrix->M44;

    Eigen::Affine3f transformationAffine;
    transformationAffine = transformationMatrix;

    auto addMesh = [&transformationAffine, &scene, &newInstance](hl::HHMesh* srcMesh, const MeshType type)
    {
        std::unique_ptr<Mesh> mesh = createMesh(srcMesh, transformationAffine, scene);
        mesh->type = type;

        newInstance->meshes.push_back(mesh.get());

        scene.meshes.push_back(std::move(mesh));
    };

    for (size_t i = 0; i < model->MeshGroups.Count; i++)
    {
        for (size_t j = 0; j < model->MeshGroups[i]->Solid.Count; j++)
            addMesh(model->MeshGroups[i]->Solid[j], MESH_TYPE_OPAQUE);

        for (size_t j = 0; j < model->MeshGroups[i]->Transparent.Count; j++)
            addMesh(model->MeshGroups[i]->Transparent[j], MESH_TYPE_TRANSPARENT);

        for (size_t j = 0; j < model->MeshGroups[i]->Boolean.Count; j++)
            addMesh(model->MeshGroups[i]->Boolean[j], MESH_TYPE_PUNCH);

        for (size_t j = 0; j < model->MeshGroups[i]->Special.Count; j++)
        {
            for (size_t k = 0; k < *model->MeshGroups[i]->Special.MeshCounts[j].Get(); k++)
                addMesh(model->MeshGroups[i]->Special.Meshes[j][k].Get(), MESH_TYPE_SPECIAL);
        }
    }

    return newInstance;
}

std::unique_ptr<Light> SceneFactory::createLight(hl::HHLight* light)
{
    std::unique_ptr<Light> newLight = std::make_unique<Light>();

    newLight->type = (LightType)light->Type;
    newLight->positionOrDirection[0] = light->Position.X;
    newLight->positionOrDirection[1] = light->Position.Y;
    newLight->positionOrDirection[2] = light->Position.Z;
    newLight->color[0] = light->Color.X;
    newLight->color[1] = light->Color.Y;
    newLight->color[2] = light->Color.Z;

    if (newLight->type != LIGHT_TYPE_POINT)
    {
        newLight->positionOrDirection.normalize();
        return newLight;
    }

    newLight->innerRange = light->InnerRange;
    newLight->outerRange = light->OuterRange;

    return newLight;
}

void SceneFactory::loadResources(const hl::Archive& archive, Scene& scene)
{
    for (auto& file : archive.Files)
    {
        if (!strstr(file.Path.get(), ".dds"))
            continue;

        std::unique_ptr<Bitmap> bitmap = createBitmap(file.Data.get(), file.Size);
        if (!bitmap)
            continue;

        bitmap->name = getFileNameWithoutExtension(file.Path.get());

        scene.bitmaps.push_back(std::move(bitmap));
    }

    for (auto& file : archive.Files)
    {
        if (!strstr(file.Path.get(), ".material"))
            continue;

        hl::DHHFixData(file.Data.get());

        hl::HHMaterialV3* material = hl::DHHGetData<hl::HHMaterialV3>(file.Data.get());
        material->EndianSwapRecursive(true);

        std::unique_ptr<Material> newMaterial = createMaterial(material, scene);
        newMaterial->name = getFileNameWithoutExtension(file.Path.get());

        scene.materials.push_back(std::move(newMaterial));
    }

    for (auto& file : archive.Files)
    {
        if (!strstr(file.Path.get(), ".light") || strstr(file.Path.get(), ".light-list"))
            continue;

        hl::DHHFixData(file.Data.get());

        hl::HHLight* light = hl::DHHGetData<hl::HHLight>(file.Data.get());
        light->EndianSwap();

        scene.lights.push_back(std::move(createLight(light)));
    }
}

void SceneFactory::loadTerrain(const hl::Archive& archive, Scene& scene)
{
    std::vector<hl::HHTerrainModel*> models;

    for (auto& file : archive.Files)
    {
        if (!strstr(file.Path.get(), ".terrain-model"))
            continue;

        hl::DHHFixData(file.Data.get());

        hl::HHTerrainModel* model = hl::DHHGetData<hl::HHTerrainModel>(file.Data.get());
        model->EndianSwapRecursive(true);

        models.push_back(model);
    }

    for (auto& file : archive.Files)
    {
        if (!strstr(file.Path.get(), ".terrain-instanceinfo"))
            continue;

        hl::DHHFixData(file.Data.get());

        hl::HHTerrainInstanceInfoV0* instance = hl::DHHGetData<hl::HHTerrainInstanceInfoV0>(file.Data.get());
        instance->EndianSwapRecursive(true);

        for (auto& model : models)
        {
            if (strcmp(model->Name.Get(), instance->ModelName.Get()) != 0)
                continue;

            std::unique_ptr<Instance> newInstance = createInstance(instance, model, scene);
            newInstance->name = instance->Filename.Get();

            scene.instances.push_back(std::move(newInstance));
            break;
        }
    }
}

std::unique_ptr<Scene> SceneFactory::createFromGenerations(const std::string& directoryPath)
{
    std::unique_ptr<Scene> scene = std::make_unique<Scene>();

    hl::Archive resArchive((directoryPath + "/" + getFileNameWithoutExtension(directoryPath) + ".ar.00").c_str());

    loadResources(resArchive, *scene);

    hl::HHPackedFileInfo* pfi = nullptr;
    for (auto& file : resArchive.Files)
    {
        if (strcmp(file.Path.get(), "Stage.pfi") != 0)
            continue;

        hl::DHHFixData(file.Data.get());

        pfi = hl::DHHGetData<hl::HHPackedFileInfo>(file.Data.get());
        pfi->EndianSwapRecursive(true);
    }

    if (!pfi)
        return nullptr;

    const FileStream pfdFile((directoryPath + "/Stage.pfd").c_str(), "rb");
    for (size_t i = 0; i < pfi->Entries.Count; i++)
    {
        hl::HHPackedFileInfoEntry* entry = pfi->Entries[i].Get();
        if (!strstr(entry->FileName.Get(), "tg-"))
            continue;

        std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(entry->Length);
        pfdFile.seek(entry->Position, SEEK_SET);
        pfdFile.read(data.get(), entry->Length);

        {
            const FileStream tgFile("temp.cab", "wb");
            tgFile.write(data.get(), entry->Length);
        }

        std::system("expand temp.cab temp.ar");

        hl::Archive tgArchive("temp.ar");
        loadTerrain(tgArchive, *scene);
    }

    return scene;
}
