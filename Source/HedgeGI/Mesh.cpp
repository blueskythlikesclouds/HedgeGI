#include "Mesh.h"

#include "FileStream.h"
#include "RaytracingDevice.h"
#include "Scene.h"

Matrix3 Vertex::getTangentToWorldMatrix() const
{
    Matrix3 tangentToWorld;
    tangentToWorld <<
        tangent[0], binormal[0], normal[0],
        tangent[1], binormal[1], normal[1],
        tangent[2], binormal[2], normal[2];

    return tangentToWorld;
}

void Mesh::buildAABB()
{
    aabb.setEmpty();

    for (size_t i = 0; i < vertexCount; i++)
        aabb.extend(vertices[i].position);
}

RTCGeometry Mesh::createRTCGeometry() const
{
    const RTCGeometry rtcGeometry = rtcNewGeometry(RaytracingDevice::get(), RTC_GEOMETRY_TYPE_TRIANGLE);

    rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, vertices.get(), 0, sizeof(Vertex), vertexCount);
    rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, triangles.get(), 0, sizeof(Triangle), triangleCount);

    rtcCommitGeometry(rtcGeometry);
    return rtcGeometry;
}

void Mesh::generateTangents() const
{
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        Vertex& vertex = vertices[i];

        const Vector3 t1 = vertex.normal.cross(Vector3(0, 0, 1));
        const Vector3 t2 = vertex.normal.cross(Vector3(0, 1, 0));
        vertex.tangent = (t1.norm() > t2.norm() ? t1 : t2).normalized();
        vertex.binormal = vertex.tangent.cross(vertex.normal).normalized();
    }
}

void Mesh::read(const FileStream& file, const Scene& scene)
{
    type = (MeshType)file.read<uint32_t>();
    vertexCount = file.read<uint32_t>();
    triangleCount = file.read<uint32_t>();

    vertices = std::make_unique<Vertex[]>(vertexCount);
    triangles = std::make_unique<Triangle[]>(triangleCount);

    file.read(vertices.get(), vertexCount);
    file.read(triangles.get(), triangleCount);

    const uint32_t index = file.read<uint32_t>();
    if (index != (uint32_t)-1)
        material = scene.materials[index].get();

    aabb = file.read<AABB>();
}

void Mesh::write(const FileStream& file, const Scene& scene) const
{
    file.write((uint32_t)type);
    file.write(vertexCount);
    file.write(triangleCount);
    file.write(vertices.get(), vertexCount);
    file.write(triangles.get(), triangleCount);

    uint32_t index = (uint32_t)-1;
    for (size_t i = 0; i < scene.materials.size(); i++)
    {
        if (material != scene.materials[i].get())
            continue;

        index = (uint32_t)i;
        break;
    }

    file.write(index);
    file.write(aabb);
}
