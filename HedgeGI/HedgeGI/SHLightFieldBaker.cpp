#include "SHLightFieldBaker.h"
#include "BakePoint.h"
#include "BakingFactory.h"

const Eigen::Vector3f SHLF_DIRECTIONS[6] =
{
    Eigen::Vector3f(1, 0, 0),
    Eigen::Vector3f(-1, 0, 0),
    Eigen::Vector3f(0, 1, 0),
    Eigen::Vector3f(0, -1, 0),
    Eigen::Vector3f(0, 0, 1),
    Eigen::Vector3f(0, 0, -1)
};

struct SHLightFieldPoint : BakePoint<6, BAKE_POINT_FLAGS_NONE>
{
    uint16_t z { (uint16_t)-1 };

    static Eigen::Vector3f sampleDirection(const size_t index, const size_t sampleCount, const float u1, const float u2)
    {
        return sampleDirectionSphere(u1, u2);
    }

    bool valid() const
    {
        return true;
    }

    void addSample(const Eigen::Array3f& color, const Eigen::Vector3f& tangentSpaceDirection, const Eigen::Vector3f& worldSpaceDirection)
    {
        for (size_t i = 0; i < 6; i++)
            colors[i] += computeSHLightField(worldSpaceDirection, color, SHLF_DIRECTIONS[i]).array();
    }

    void end(const uint32_t sampleCount)
    {
        for (size_t i = 0; i < 6; i++)
            colors[i] = (colors[i] * (PI / sampleCount)).cwiseMax(0);
    }
};

std::vector<SHLightFieldPoint> SHLightFieldBaker::createBakePoints(const SHLightField& shlf)
{
    std::vector<SHLightFieldPoint> bakePoints;
    bakePoints.reserve(shlf.resolution.x() * shlf.resolution.y() * shlf.resolution.z());

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
                bakePoint.position = (shlf.matrix * Eigen::Vector4f(xNormalized, yNormalized, zNormalized, 1)).head<3>() / 10.0f;
                bakePoint.smoothPosition = bakePoint.position;
                bakePoint.tangentToWorldMatrix.setIdentity();
                bakePoint.x = (uint16_t)x;
                bakePoint.y = (uint16_t)y;
                bakePoint.z = (uint16_t)z;

                bakePoints.push_back(std::move(bakePoint));
            }
        }
    }

    return bakePoints;
}

std::unique_ptr<Bitmap> SHLightFieldBaker::paint(const std::vector<SHLightFieldPoint>& bakePoints, const SHLightField& shlf)
{
    std::unique_ptr<Bitmap> bitmap = std::make_unique<Bitmap>(shlf.resolution.x() * 9, shlf.resolution.y(), shlf.resolution.z(), BITMAP_TYPE_3D);

    for (auto& bakePoint : bakePoints)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            Eigen::Array4f color;
            color.head<3>() = bakePoint.colors[i];
            color.w() = 1.0f;

            bitmap->putColor(color, i * shlf.resolution.x() + bakePoint.x, bakePoint.y, bakePoint.z);
        }
    }

    return bitmap;
}

std::unique_ptr<Bitmap> SHLightFieldBaker::bake(const RaytracingContext& context, const SHLightField& shlf, const BakeParams& bakeParams)
{
    std::vector<SHLightFieldPoint> bakePoints = createBakePoints(shlf);

    BakingFactory::bake(context, bakePoints, bakeParams);

    return paint(bakePoints, shlf);
}
