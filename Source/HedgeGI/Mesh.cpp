#include "Mesh.h"

#include "Math.h"
#include "RaytracingDevice.h"

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

// https://gist.github.com/pixnblox/5e64b0724c186313bc7b6ce096b08820

Vector3 getSmoothPosition(const Vertex& a, const Vertex& b, const Vertex& c, const Vector2& baryUV)
{
    const Vector3 position = barycentricLerp(a.position, b.position, c.position, baryUV);
    const Vector3 normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV);

    const Vector3 vecProj0 = position - (position - a.position).dot(a.normal) * a.normal;
    const Vector3 vecProj1 = position - (position - b.position).dot(b.normal) * b.normal;
    const Vector3 vecProj2 = position - (position - c.position).dot(c.normal) * c.normal;

    const Vector3 smoothPosition = barycentricLerp(vecProj0, vecProj1, vecProj2, baryUV);
    return (smoothPosition - position).dot(normal) > 0.0f ? smoothPosition : position;
}