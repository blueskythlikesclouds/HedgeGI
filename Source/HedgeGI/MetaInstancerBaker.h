#pragma once

struct BakeParams;
struct RaytracingContext;
class MetaInstancer;

class MetaInstancerBaker
{
public:
    static void bake(MetaInstancer& metaInstancer, const RaytracingContext& raytracingContext, const BakeParams& bakeParams);
};
