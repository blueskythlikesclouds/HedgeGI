#pragma once
#include <HedgeLib/Math/Vector.h>

namespace hl
{
    enum HHLightType : uint32_t
    {
        HH_LIGHT_TYPE_DIRECTIONAL,
        HH_LIGHT_TYPE_POINT
    };

    struct HHLight
    {
        uint32_t Type;
        Vector3 Position;
        Vector3 Color;
        uint32_t Unknown0;
        uint32_t Unknown1;
        uint32_t Unknown2;
        float InnerRange;
        float OuterRange;

        void EndianSwap()
        {
            Swap(Type);
            Swap(Position);
            Swap(Color);

            if (Type == HH_LIGHT_TYPE_POINT)
            {
                Swap(Unknown0);
                Swap(Unknown1);
                Swap(Unknown2);
                Swap(InnerRange);
                Swap(OuterRange);
            }
        }
    };
}
