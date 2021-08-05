#include "VertexColorRemover.h"

void VertexColorRemover::process(hl::hh::mirage::terrain_model& model)
{
    auto processMesh = [](hl::hh::mirage::mesh& mesh)
    {
        for (auto& element : mesh.vertexElements)
        {
            if (element.type != hl::hh::mirage::vertex_type::color)
                continue;

            switch (element.format)
            {
            case (hl::u32)hl::hh::mirage::vertex_format::float4:

                for (size_t i = 0; i < mesh.vertexCount; i++)
                {
                    float* vtx = (float*)&mesh.vertices[mesh.vertexSize * i + element.offset];

                    vtx[0] = 1.0f;
                    vtx[1] = 1.0f;
                    vtx[2] = 1.0f;
                    vtx[3] = 1.0f;
                }

                break;

            case (hl::u32)hl::hh::mirage::vertex_format::ubyte4_norm:

                for (size_t i = 0; i < mesh.vertexCount; i++)
                {
                    hl::u8* vtx = &mesh.vertices[mesh.vertexSize * i + element.offset];

                    vtx[0] = 0xFF;
                    vtx[1] = 0xFF;
                    vtx[2] = 0xFF;
                    vtx[3] = 0xFF;
                }

                break;
            }
        }
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
