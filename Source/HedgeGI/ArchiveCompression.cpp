#include "ArchiveCompression.h"

#include "CabinetCompression.h"
#include "Utilities.h"
#include "XCompression.h"

bool ArchiveCompression::decompress(void* data, size_t dataSize, hl::stream& destination)
{
    if (CabinetCompression::checkSignature(data))
    {
        CabinetCompression::decompress(data, dataSize, destination);
        return true;
    }

    if (XCompression::checkSignature(data))
    {
        XCompression::decompress(data, dataSize, destination);
        return true;
    }

    return false;
}

void ArchiveCompression::load(hl::archive& archive, void* data, size_t dataSize)
{
    hl::mem_stream destination;

    if (decompress(data, dataSize, destination))
        loadArchive(archive, destination.get_data_ptr(), destination.get_size());
    else
        loadArchive(archive, data, dataSize);
}

hl::archive ArchiveCompression::load(void* data, const size_t dataSize)
{
    hl::archive archive;
    load(archive, data, dataSize);
    return archive;
}
