#pragma once

#include <HedgeLib/Offsets.h>

namespace hl
{
    struct HHPackedFileInfoEntry
    {
        StringOffset32 FileName;
        uint32_t Position;
        uint32_t Length;

        void EndianSwap()
        {
            Swap(Position);
            Swap(Length);
        }

        void EndianSwapRecursive(bool isBigEndian)
        {
            Swap(Position);
            Swap(Length);
        }
    };

    struct HHPackedFileInfo
    {
        ArrayOffset32<DataOffset32<HHPackedFileInfoEntry>> Entries;

        void EndianSwap()
        {
            Swap(Entries);
        }

        void EndianSwapRecursive(bool isBigEndian)
        {
            SwapRecursive(isBigEndian, Entries);
        }
    };
}
