#pragma once
#include <hedgelib/hl_math.h>

typedef enum HlHHLightType
{
    HL_HH_LIGHT_TYPE_DIRECTIONAL = 0,
    HL_HH_LIGHT_TYPE_POINT = 1
}
HlHHLightType;

typedef struct HlHHLightV1
{
    HlU32 type;
    HlVector3 position;
    HlVector3 color;
    HlU32 unknown0;
    HlU32 unknown1;
    HlU32 unknown2;
    float innerRange;
    float outerRange;
}
HlHHLightV1;

static HL_API void hlHHLightV1Fix(HlHHLightV1* light)
{
#ifndef HL_IS_BIG_ENDIAN
    hlSwapU32P(&light->type);
    hlVector3Swap(&light->position);
    hlVector3Swap(&light->color);

    if (light->type != HL_HH_LIGHT_TYPE_POINT)
        return;

    hlSwapU32P(&light->unknown0);
    hlSwapU32P(&light->unknown1);
    hlSwapU32P(&light->unknown2);
    hlSwapFloatP(&light->innerRange);
    hlSwapFloatP(&light->outerRange);
#endif
}