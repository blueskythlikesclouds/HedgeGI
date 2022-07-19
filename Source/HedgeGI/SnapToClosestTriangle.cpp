#include "SnapToClosestTriangle.h"

#include "Math.h"
#include "Mesh.h"

bool SnapToClosestTriangle::pointQueryFunc(RTCPointQueryFunctionArguments* args)
{
    UserData* userData = (UserData*)args->userPtr;

    const Mesh& mesh = *userData->scene->meshes[args->geomID];

    const Triangle& triangle = mesh.triangles[args->primID];
    const Vertex& a = mesh.vertices[triangle.a];
    const Vertex& b = mesh.vertices[triangle.b];
    const Vertex& c = mesh.vertices[triangle.c];

    const Vector3 closestPoint = closestPointTriangle(userData->position, a.position, b.position, c.position);
    const Vector2 baryUV = getBarycentricCoords(closestPoint, a.position, b.position, c.position);
    const Vector3 normal = barycentricLerp(a.normal, b.normal, c.normal, baryUV);

    const float distance = (closestPoint - userData->position).squaredNorm();
    if (distance > userData->distance)
        return false;

    userData->newPosition = closestPoint + normal.normalized() * 0.1f;
    userData->distance = distance;

    return false;
}
