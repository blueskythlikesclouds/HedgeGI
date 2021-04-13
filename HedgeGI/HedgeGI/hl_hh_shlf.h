#pragma once
#include <hedgelib/hl_math.h>

typedef struct HlHHSHLightField
{
    HL_OFF64_STR name;
    HlU32 probeCounts[3];
    HlVector3 position;
    HlVector3 rotation;
    HlVector3 scale;
} HlHHSHLightField;

typedef struct HlHHSHLightFieldSet
{
    HlU8 unknown[0x94];
    HlU32 count;
    HL_OFF64(HlHHSHLightField) items;
} HlHHSHLightFieldSet;