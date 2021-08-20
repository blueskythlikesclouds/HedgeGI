#include "CabinetCompression.h"

#include <fdi.h>

namespace
{
    FNALLOC(fdiAlloc)
    {
        return operator new(cb);
    }

    FNFREE(fdiFree)
    {
        operator delete(pv);
    }

    FNOPEN(fdiOpen)
    {
        void* ptr;
        sscanf(pszFile, "%p", &ptr);

        return (INT_PTR)ptr;
    }

    FNREAD(fdiRead)
    {
        return (UINT)((hl::stream*)hf)->read(cb, pv);
    }

    FNWRITE(fdiWrite)
    {
        return (UINT)((hl::stream*)hf)->write(cb, pv);
    }

    FNCLOSE(fdiClose)
    {
        return 0;
    }

    FNSEEK(fdiSeek)
    {
        hl::stream* stream = (hl::stream*)hf;
        stream->seek((hl::seek_mode)seektype, dist);
        return (long)stream->tell();
    }

    FNFDINOTIFY(fdiNotify)
    {
        return fdint == fdintCOPY_FILE ? (INT_PTR)pfdin->pv : 0;
    }

    hl::archive loadArchive(void* data, const size_t dataSize)
    {
        hl::archive archive;

        auto header = (hl::hh::ar::header*)data;

        header->fix(dataSize);
        header->parse(dataSize, archive);

        return archive;
    }
}

hl::archive CabinetCompression::load(void* data, const size_t dataSize)
{
    if (*(uint32_t*)data != MAKEFOURCC('M', 'S', 'C', 'F'))
        return loadArchive(data, dataSize);

    hl::readonly_mem_stream inStream(data, dataSize);
    hl::mem_stream outStream;

    char cabinet[1]{};
    char cabPath[24]{};

    sprintf(cabPath, "%p", &inStream);

    ERF erf;

    const HFDI fdi = FDICreate(
        fdiAlloc,
        fdiFree,
        fdiOpen,
        fdiRead,
        fdiWrite,
        fdiClose,
        fdiSeek,
        cpuUNKNOWN,
        &erf);

    FDICopy(fdi, cabinet, cabPath, 0, fdiNotify, nullptr, &outStream);
    FDIDestroy(fdi);

    return loadArchive(outStream.get_data_ptr(), outStream.get_size());
}