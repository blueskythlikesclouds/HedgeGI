#pragma once

#include "Scene.h"

class SnapToClosestTriangle
{
public:
    struct UserData
    {
        const Scene* scene{};
        Vector3 position{};
        Vector3 newPosition{};
        float distance{};
    };

    static bool pointQueryFunc(RTCPointQueryFunctionArguments* args);

    template<typename TBakePoint>
    static void process(const RaytracingContext& raytracingContext, std::vector<TBakePoint>& bakePoints, const float radius = 1.0f)
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, bakePoints.size()), [&](const tbb::blocked_range<size_t> range)
        {
            for (size_t r = range.begin(); r < range.end(); r++)
            {
                auto& bakePoint = bakePoints[r];

                // Snap to the closest triangle
                RTCPointQueryContext context{};
                rtcInitPointQueryContext(&context);

                RTCPointQuery query{};
                query.x = bakePoint.position.x();
                query.y = bakePoint.position.y();
                query.z = bakePoint.position.z();
                query.radius = radius;

                UserData userData = { raytracingContext.scene, bakePoint.position, bakePoint.position, INFINITY };
                rtcPointQuery(raytracingContext.rtcScene, &query, &context, pointQueryFunc, &userData);

                bakePoint.position = userData.newPosition;
            }
        });
    }
};
