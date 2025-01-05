#include "UV2Mapper.h"
#include "Logger.h"

void UV2Mapper::process(hl::hh::mirage::model& model)
{
    size_t meshCount = 0;
    for (auto& meshGroup : model.meshGroups)
    {
        meshCount += meshGroup.opaq.size();
        meshCount += meshGroup.trans.size();
        meshCount += meshGroup.punch.size();

        for (auto& specialMeshGroup : meshGroup.special)
            meshCount += specialMeshGroup.size();
    }

    if (meshCount == 0)
        return;

    xatlas::Atlas* atlas = xatlas::Create();

    std::vector<hl::vec4> positions;
    std::vector<hl::vec4> normals;
    std::vector<hl::vec4> texCoords;
    std::vector<uint16_t> indices;

    auto addMesh = [&](const hl::hh::mirage::mesh& mesh)
    {
        positions.clear();
        positions.resize(mesh.vertexCount);

        normals.clear();
        normals.resize(mesh.vertexCount);

        texCoords.clear();
        texCoords.resize(mesh.vertexCount);

        indices.clear();

        for (auto& element : mesh.vertexElements)
        {
            std::vector<hl::vec4>* vec = nullptr;

            switch (element.type)
            {
            case hl::hh::mirage::raw_vertex_type::position:
                vec = &positions;
                break;

            case hl::hh::mirage::raw_vertex_type::normal:
                vec = &normals;
                break;

            case hl::hh::mirage::raw_vertex_type::texcoord:
                if (element.index != 0)
                    continue;

                vec = &texCoords;
                break;

            default:
                continue;
            }

            for (size_t i = 0; i < mesh.vertexCount; i++)
                element.convert_to_vec4(&mesh.vertices[i * mesh.vertexSize + element.offset], vec->at(i));
        }

        hl::u16 f1 = mesh.faces[0];
        hl::u16 f2 = mesh.faces[1];
        hl::u16 f3;

        bool reverse = false;
        for (size_t i = 2; i < mesh.faces.size(); ++i)
        {
            f3 = mesh.faces[i];

            if (f3 == UINT16_MAX)
            {
                f1 = mesh.faces[i + 1];
                f2 = mesh.faces[i + 2];

                reverse = false;
                i += 2;
            }
            else
            {
                if (f1 != f2 && f2 != f3 && f3 != f1)
                {
                    if (reverse)
                    {
                        indices.push_back(f1);
                        indices.push_back(f3);
                        indices.push_back(f2);
                    }
                    else
                    {
                        indices.push_back(f1);
                        indices.push_back(f2);
                        indices.push_back(f3);
                    }
                }

                f1 = f2;
                f2 = f3;
                reverse = !reverse;
            }
        }

        xatlas::MeshDecl decl;
        decl.vertexPositionData = positions.data();
        decl.vertexNormalData = normals.data();
        decl.vertexUvData = texCoords.data();
        decl.indexData = indices.data();
        decl.vertexCount = (uint32_t)positions.size();
        decl.vertexPositionStride = sizeof(positions[0]);
        decl.vertexNormalStride = sizeof(normals[0]);
        decl.vertexUvStride = sizeof(texCoords[0]);
        decl.indexCount = (uint32_t)indices.size();

        xatlas::AddMesh(atlas, decl, (uint32_t)meshCount);
    };

    for (auto& meshGroup : model.meshGroups)
    {
        for (auto& opaq : meshGroup.opaq)
            addMesh(opaq);

        for (auto& trans : meshGroup.trans)
            addMesh(trans);

        for (auto& punch : meshGroup.punch)
            addMesh(punch);

        for (auto& specialMeshGroup : meshGroup.special)
        {
            for (auto& special : specialMeshGroup)
                addMesh(special);
        }
    }

    xatlas::Generate(atlas);

    auto updateMesh = [&](hl::hh::mirage::mesh& mesh, const xatlas::Mesh& atlasMesh)
    {
        // Vertices
        mesh.vertexCount = atlasMesh.vertexCount;
        uint32_t originalVertexSize = mesh.vertexSize;

        // Find the UV2 slot. If we cannot find it, we'll have to create it.
        int32_t texCoordOffset = -1;
        auto texCoordFormat = hl::hh::mirage::raw_vertex_format::float2;

        for (auto& element : mesh.vertexElements)
        {
            if (element.type == hl::hh::mirage::raw_vertex_type::texcoord && element.index == 1)
            {
                texCoordOffset = element.offset;
                texCoordFormat = element.format;
                break;
            }
        }

        if (texCoordOffset == -1)
        {
            // Find any UV slot and pick the format from it.
            for (auto& element : mesh.vertexElements)
            {
                if (element.type == hl::hh::mirage::raw_vertex_type::texcoord && element.format == hl::hh::mirage::raw_vertex_format::float16_2)
                {
                    texCoordFormat = element.format;
                    break;
                }
            }

            // Add the UV slot.
            texCoordOffset = int32_t(mesh.vertexSize);
            mesh.vertexElements.emplace_back(hl::hh::mirage::raw_vertex_element
                {   0, // stream
                    hl::u16(texCoordOffset), // offset
                    texCoordFormat, // format
                    hl::hh::mirage::raw_vertex_method::normal, // method
                    hl::hh::mirage::raw_vertex_type::texcoord, // type
                    1, // index
                    0 // padding
                });

            switch (texCoordFormat)
            {
            case hl::hh::mirage::raw_vertex_format::float2:
                mesh.vertexSize += (sizeof(float) * 2);
                break;

            case hl::hh::mirage::raw_vertex_format::float16_2:
                mesh.vertexSize += (sizeof(uint16_t) * 2);
                break;

            default:
                assert(false && "Unrecognized vertex format.");
                break;
            }

            Logger::logFormatted(LogType::Warning, "Added lightmap UV slot for mesh with material \"%s\"", mesh.material.name().c_str());
        }

        std::unique_ptr<hl::u8[]> vertices = std::make_unique<hl::u8[]>(atlasMesh.vertexCount * mesh.vertexSize);

        for (size_t i = 0; i < atlasMesh.vertexCount; i++)
        {
            const xatlas::Vertex& vertex = atlasMesh.vertexArray[i];
            memcpy(&vertices[i * mesh.vertexSize], &mesh.vertices[vertex.xref * originalVertexSize], originalVertexSize);

            const float x = vertex.uv[0] / (float)atlas->width;
            const float y = vertex.uv[1] / (float)atlas->height;

            void* vtx = &vertices[i * mesh.vertexSize + texCoordOffset];
            
            switch (texCoordFormat)
            {
            case hl::hh::mirage::raw_vertex_format::float2:
                ((float*)vtx)[0] = x;
                ((float*)vtx)[1] = y;
                break;
            
            case hl::hh::mirage::raw_vertex_format::float16_2:
                ((unsigned short*)vtx)[0] = meshopt_quantizeHalf(x);
                ((unsigned short*)vtx)[1] = meshopt_quantizeHalf(y);
                break;            
            }
        }

        mesh.vertices = std::move(vertices);

        // Faces
        std::vector<unsigned> faces;
        faces.resize(meshopt_stripifyBound(atlasMesh.indexCount));

        const size_t faceCount = meshopt_stripify(faces.data(), atlasMesh.indexArray, atlasMesh.indexCount, atlasMesh.vertexCount, 0xFFFF);

        mesh.faces.clear();
        for (size_t i = 0; i < faceCount; i++)
            mesh.faces.push_back((hl::u16)faces[i]);
    };

    size_t meshIndex = 0;

    for (auto& meshGroup : model.meshGroups)
    {
        for (auto& opaq : meshGroup.opaq)
            updateMesh(opaq, atlas->meshes[meshIndex++]);

        for (auto& trans : meshGroup.trans)
            updateMesh(trans, atlas->meshes[meshIndex++]);

        for (auto& punch : meshGroup.punch)
            updateMesh(punch, atlas->meshes[meshIndex++]);

        for (auto& specialMeshGroup : meshGroup.special)
        {
            for (auto& special : specialMeshGroup)
                updateMesh(special, atlas->meshes[meshIndex++]);
        }
    }

    xatlas::Destroy(atlas);
}
