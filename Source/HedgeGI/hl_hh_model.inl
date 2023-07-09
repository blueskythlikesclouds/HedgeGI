#pragma once

// This is taken directly from HedgeLib.

namespace hl::hh::mirage
{
static void in_swap_vertex(const raw_vertex_element& rawVtxElem, void* rawVtx)
{
    // Swap vertex based on vertex element.
    switch (rawVtxElem.format)
    {
    case raw_vertex_format::float1:
        endian_swap(*static_cast<float*>(rawVtx));
        break;

    case raw_vertex_format::float2:
        endian_swap(*static_cast<vec2*>(rawVtx));
        break;

    case raw_vertex_format::float3:
        endian_swap(*static_cast<vec3*>(rawVtx));
        break;

    case raw_vertex_format::float4:
        endian_swap(*static_cast<vec4*>(rawVtx));
        break;

    case raw_vertex_format::int1:
    case raw_vertex_format::int1_norm:
    case raw_vertex_format::uint1:
    case raw_vertex_format::uint1_norm:
    case raw_vertex_format::d3d_color:
    case raw_vertex_format::udec3:
    case raw_vertex_format::dec3:
    case raw_vertex_format::udec3_norm:
    case raw_vertex_format::dec3_norm:
    case raw_vertex_format::udec4:
    case raw_vertex_format::dec4:
    case raw_vertex_format::udec4_norm:
    case raw_vertex_format::dec4_norm:
    case raw_vertex_format::uhend3:
    case raw_vertex_format::hend3:
    case raw_vertex_format::uhend3_norm:
    case raw_vertex_format::hend3_norm:
    case raw_vertex_format::udhen3:
    case raw_vertex_format::dhen3:
    case raw_vertex_format::udhen3_norm:
    case raw_vertex_format::dhen3_norm:
        endian_swap(*static_cast<u32*>(rawVtx));
        break;

    case raw_vertex_format::int2:
    case raw_vertex_format::int2_norm:
    case raw_vertex_format::uint2:
    case raw_vertex_format::uint2_norm:
        endian_swap(*(static_cast<u32*>(rawVtx)));
        endian_swap(*(static_cast<u32*>(rawVtx) + 1));
        break;

    case raw_vertex_format::int4:
    case raw_vertex_format::int4_norm:
    case raw_vertex_format::uint4:
    case raw_vertex_format::uint4_norm:
        endian_swap(*(static_cast<u32*>(rawVtx)));
        endian_swap(*(static_cast<u32*>(rawVtx) + 1));
        endian_swap(*(static_cast<u32*>(rawVtx) + 2));
        endian_swap(*(static_cast<u32*>(rawVtx) + 3));
        break;

    case raw_vertex_format::ubyte4:
    case raw_vertex_format::byte4:
    case raw_vertex_format::ubyte4_norm:
    case raw_vertex_format::byte4_norm:
        endian_swap(*static_cast<u32*>(rawVtx));
        break;

    case raw_vertex_format::short2:
    case raw_vertex_format::short2_norm:
    case raw_vertex_format::ushort2:
    case raw_vertex_format::ushort2_norm:
    case raw_vertex_format::float16_2:
        endian_swap(*(static_cast<u16*>(rawVtx)));
        endian_swap(*(static_cast<u16*>(rawVtx) + 1));
        break;

    case raw_vertex_format::short4:
    case raw_vertex_format::short4_norm:
    case raw_vertex_format::ushort4:
    case raw_vertex_format::ushort4_norm:
    case raw_vertex_format::float16_4:
        endian_swap(*(static_cast<u16*>(rawVtx)));
        endian_swap(*(static_cast<u16*>(rawVtx) + 1));
        endian_swap(*(static_cast<u16*>(rawVtx) + 2));
        endian_swap(*(static_cast<u16*>(rawVtx) + 3));
        break;

    default:
        throw std::runtime_error("Unsupported HH vertex format");
    }
}

template<typename RawMeshType>
static void in_swap_recursive(RawMeshType& rawMesh)
{
    // Swap mesh.
    endian_swap<false>(rawMesh);

    // Swap faces.
    for (auto& face : rawMesh.faces)
    {
        endian_swap(face);
    }

    // Swap vertex format (array of vertex elements).
    raw_vertex_element* curVtxElem = rawMesh.vertexElements.get();
    do
    {
        curVtxElem->endian_swap<false>();
    }
    while ((curVtxElem++)->format != raw_vertex_format::last_entry);

    // Swap vertices based on vertex format.
    curVtxElem = rawMesh.vertexElements.get();
    while (curVtxElem->format != raw_vertex_format::last_entry)
    {
        // Swap vertices based on vertex element.
        void* curVtx = ptradd(rawMesh.vertices.get(), curVtxElem->offset);
        for (u32 i = 0; i < rawMesh.vertexCount; ++i)
        {
            // Swap vertex.
            in_swap_vertex(*curVtxElem, curVtx);

            // Increase vertices pointer.
            curVtx = ptradd(curVtx, rawMesh.vertexSize);
        }

        // Increase current vertex element pointer.
        ++curVtxElem;
    }

    // Swap bone node indices if necessary.
    if constexpr (std::is_same_v<RawMeshType, raw_mesh_r2>)
    {
        for (auto& boneNodeIndex : rawMesh.boneNodeIndices)
        {
            endian_swap(boneNodeIndex);
        }
    }
}

template<typename RawMeshType>
static void in_swap_recursive(raw_mesh_slot<RawMeshType>& rawMeshSlot)
{
    for (auto& meshOff : rawMeshSlot)
    {
        in_swap_recursive(*meshOff);
    }
}

template<typename RawMeshType>
static void in_swap_recursive(raw_special_meshes<RawMeshType>& rawSpecial)
{
    for (u32 i = 0; i < rawSpecial.count; ++i)
    {
        // Swap mesh count.
        endian_swap(*rawSpecial.meshCounts[i]);

        // Swap meshes.
        const u32 meshCount = *rawSpecial.meshCounts[i];
        for (u32 i2 = 0; i2 < meshCount; ++i2)
        {
            in_swap_recursive(*rawSpecial.meshes[i][i2]);
        }
    }
}

template<typename RawMeshType>
static void in_swap_recursive(raw_mesh_group<RawMeshType>& rawMeshGroup)
{
    // Swap the mesh group.
    endian_swap<false>(rawMeshGroup);

    // Recursively swap mesh slots.
    in_swap_recursive(rawMeshGroup.opaq);
    in_swap_recursive(rawMeshGroup.punch);
    in_swap_recursive(rawMeshGroup.trans);
    in_swap_recursive(rawMeshGroup.special);
}

template<typename RawMeshType>
static void in_swap_recursive(arr32<off32<raw_mesh_group<RawMeshType>>>& groups)
{
    for (auto& groupOff : groups)
    {
        in_swap_recursive(*groupOff);
    }
}
}