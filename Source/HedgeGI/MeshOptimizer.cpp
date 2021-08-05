#include "MeshOptimizer.h"

void MeshOptimizer::process(hl::hh::mirage::terrain_model& model)
{
    std::vector<unsigned> indices;

    auto processMesh = [&](hl::hh::mirage::mesh& mesh)
    {
        indices.clear();

        // TODO: Duplicate code is nasty, make this a common function
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

        meshopt_optimizeVertexCacheStrip(indices.data(), indices.data(), indices.size(), mesh.vertexCount);
        meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), (const float*)mesh.vertices.get(), mesh.vertexCount, mesh.vertexSize, 1.05f);
        meshopt_optimizeVertexFetch(mesh.vertices.get(), indices.data(), indices.size(), mesh.vertices.get(), mesh.vertexCount, mesh.vertexSize);

        std::vector<unsigned> strips;
        strips.resize(meshopt_stripifyBound(indices.size()));
        mesh.faces.resize(meshopt_stripify(strips.data(), indices.data(), indices.size(), mesh.vertexCount, 0xFFFF));

        for (size_t i = 0; i < mesh.faces.size(); i++)
            mesh.faces[i] = (hl::u16)strips[i];

        // TODO: Use optimized vertex format?
    };

    for (auto& meshGroup : model.meshGroups)
    {
        for (auto& opaq : meshGroup.opaq)
            processMesh(opaq);

        for (auto& trans : meshGroup.trans)
            processMesh(trans);

        for (auto& punch : meshGroup.punch)
            processMesh(punch);

        for (auto& specialMeshGroup : meshGroup.special)
        {
            for (auto& special : specialMeshGroup)
                processMesh(special);
        }
    }
}
