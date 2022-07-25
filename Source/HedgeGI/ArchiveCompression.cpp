#include "ArchiveCompression.h"

#include "CabinetCompression.h"
#include "Utilities.h"
#include "XCompression.h"

void ArchiveCompression::load(hl::archive& archive, void* data, size_t dataSize)
{
    if (CabinetCompression::checkSignature(data))
        CabinetCompression::load(archive, data, dataSize);
    
    else if (XCompression::checkSignature(data))
        XCompression::load(archive, data, dataSize);

    else
        loadArchive(archive, data, dataSize);
}

hl::archive ArchiveCompression::load(void* data, const size_t dataSize)
{
    hl::archive archive;
    load(archive, data, dataSize);
    return archive;
}
