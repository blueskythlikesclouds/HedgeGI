#include "SHLightFieldBaker.h"

#include "BakePoint.h"
#include "BakingFactory.h"
#include "Bitmap.h"
#include "Math.h"
#include "SHLightField.h"

const Vector3 SHLF_DIRECTIONS[6] =
{
    Vector3(1, 0, 0),
    Vector3(-1, 0, 0),
    Vector3(0, 1, 0),
    Vector3(0, -1, 0),
    Vector3(0, 0, 1),
    Vector3(0, 0, -1)
};

// TODO: This value has been approximated. What's the formula for calculating this?
const float SHLF_FACTOR = 5.8369751043319704f;

struct SHLightFieldPoint : BakePoint<6, BAKE_POINT_FLAGS_SHADOW | BAKE_POINT_FLAGS_SOFT_SHADOW>
{
    uint16_t z { (uint16_t)-1 };

    static Vector3 sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
    {
        return sampleDirectionSphere(u1, u2);
    }

    bool valid() const
    {
        return true;
    }

    void addSample(const Color3& color, const Vector3& worldSpaceDirection)
    {
        const Vector3 direction(worldSpaceDirection.x(), worldSpaceDirection.y(), -worldSpaceDirection.z());

        for (size_t i = 0; i < 6; i++)
        {
            const float cosTheta = direction.dot(SHLF_DIRECTIONS[i]);
            if (cosTheta <= 0.0f)
                continue;

            colors[i] += color * exp((cosTheta - 1.0f) * 3.0f);
        }
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 6; i++)
            colors[i] *= SHLF_FACTOR / sampleCount;
    }
};

namespace
{
    struct PointQueryFuncUserData
    {
        const Scene* scene {};
        Vector3 position {};
        Vector3 newPosition {};
        float distance {};
    };

    bool pointQueryFunc(struct RTCPointQueryFunctionArguments* args)
    {
        PointQueryFuncUserData* userData = (PointQueryFuncUserData*)args->userPtr;

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
}

std::vector<SHLightFieldPoint> SHLightFieldBaker::createBakePoints(const RaytracingContext& raytracingContext, const SHLightField& shlf)
{
    std::vector<SHLightFieldPoint> bakePoints;
    bakePoints.reserve(shlf.resolution.x() * shlf.resolution.y() * shlf.resolution.z());

    const Matrix4 matrix = shlf.getMatrix();

    for (size_t z = 0; z < shlf.resolution.z(); z++)
    {
        const float zNormalized = (z + 0.5f) / shlf.resolution.z() - 0.5f;

        for (size_t y = 0; y < shlf.resolution.y(); y++)
        {
            const float yNormalized = (y + 0.5f) / shlf.resolution.y() - 0.5f;

            for (size_t x = 0; x < shlf.resolution.x(); x++)
            {
                const float xNormalized = (x + 0.5f) / shlf.resolution.x() - 0.5f;

                SHLightFieldPoint bakePoint {};
                bakePoint.position = (matrix * Vector4(xNormalized, yNormalized, zNormalized, 1)).head<3>() / 10.0f;
                bakePoint.x = (uint16_t)x;
                bakePoint.y = (uint16_t)y;
                bakePoint.z = (uint16_t)z;

                bakePoints.push_back(std::move(bakePoint));
            }
        }
    }

    const float radius = (shlf.scale.array() / shlf.resolution.cast<float>()).maxCoeff() / 10.0f * sqrtf(2.0f) / 2.0f;

    std::for_each(std::execution::par_unseq, bakePoints.begin(), bakePoints.end(), [&](SHLightFieldPoint& bakePoint)
    {
        // Snap to the closest triangle
        RTCPointQueryContext context {};
        rtcInitPointQueryContext(&context);

        RTCPointQuery query {};
        query.x = bakePoint.position.x();
        query.y = bakePoint.position.y();
        query.z = bakePoint.position.z();
        query.radius = radius;

        PointQueryFuncUserData userData = { raytracingContext.scene, bakePoint.position, bakePoint.position, INFINITY };
        rtcPointQuery(raytracingContext.rtcScene, &query, &context, pointQueryFunc, &userData);

        bakePoint.position = userData.newPosition;
    });

    return bakePoints;
}

std::unique_ptr<Bitmap> SHLightFieldBaker::paint(const std::vector<SHLightFieldPoint>& bakePoints, const SHLightField& shlf)
{
    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(shlf.resolution.x() * 9, shlf.resolution.y(), shlf.resolution.z(), BitmapType::Texture3D);

    for (auto& bakePoint : bakePoints)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            Color4 color;
            color.head<3>() = bakePoint.colors[i].cwiseMax(0.0f).cwiseMin(65504.0f);
            color.w() = i == 0 ? saturate(bakePoint.shadow) : 1.0f;

            bitmap->putColor(color, i * shlf.resolution.x() + bakePoint.x, bakePoint.y, bakePoint.z);
        }
    }

    return bitmap;
}

std::unique_ptr<Bitmap> SHLightFieldBaker::bake(const RaytracingContext& context, const SHLightField& shlf, const BakeParams& bakeParams)
{
    std::vector<SHLightFieldPoint> bakePoints = createBakePoints(context, shlf);

    BakingFactory::bake(context, bakePoints, bakeParams);

    return paint(bakePoints, shlf);
}
