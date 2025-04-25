#include "CabinetCompression.h"
#include "Utilities.h"

#include <fdi.h>
#include <fci.h>

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
        hl::stream* stream;
        sscanf(pszFile, "%p", &stream);

        return (INT_PTR)stream;
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

    FNFCIFILEPLACED(fciFilePlaced)
    {
        return 0;
    }

    FNFCIALLOC(fciAlloc)
    {
        return fdiAlloc(cb);
    }

    FNFCIFREE(fciFree)
    {
        return fdiFree(memory);
    }

    FNFCIOPEN(fciOpen)
    {
        return fdiOpen(pszFile, oflag, pmode);
    }

    FNFCIREAD(fciRead)
    {
        return fdiRead(hf, memory, cb);
    }

    FNFCIWRITE(fciWrite)
    {
        return fdiWrite(hf, memory, cb);
    }

    FNFCICLOSE(fciClose)
    {
        return fdiClose(hf);
    }

    FNFCISEEK(fciSeek)
    {
        return fdiSeek(hf, dist, seektype);
    }

    FNFCIDELETE(fciDelete)
    {
        hl::mem_stream* stream;
        sscanf(pszFile, "%p", &stream);
        delete stream;

        return 0;
    }

    FNFCIGETTEMPFILE(fciGetTempFile)
    {
        hl::mem_stream* stream = new hl::mem_stream();
        sprintf(pszTempName, "%p", stream);

        return TRUE;
    }

    FNFCIGETNEXTCABINET(fciGetNextCabinet)
    {
        return false;
    }

    FNFCISTATUS(fciStatus)
    {
        return 0;
    }

    FNFCIGETOPENINFO(fciGetOpenInfo)
    {
        return fdiOpen(pszName, 0, 0);
    }
}

bool CabinetCompression::checkSignature(void* data)
{
    return *(uint32_t*)data == MAKEFOURCC('M', 'S', 'C', 'F');
}

void CabinetCompression::decompress(void* data, size_t dataSize, hl::mem_stream& destination)
{
    hl::readonly_mem_stream source(data, dataSize);

    char cabinet[1]{};
    char cabPath[24]{};

    sprintf(cabPath, "%p", &source);

    ERF erf;

    HFDI fdi = FDICreate(
        fdiAlloc,
        fdiFree,
        fdiOpen,
        fdiRead,
        fdiWrite,
        fdiClose,
        fdiSeek,
        cpuUNKNOWN,
        &erf);

    FDICopy(fdi, cabinet, cabPath, 0, fdiNotify, nullptr, &destination);
    FDIDestroy(fdi);
}

void CabinetCompression::save(const hl::archive& archive, hl::stream& destination, char* fileName)
{
    hl::mem_stream source;
    saveArchive(archive, source);

    CCAB ccab;
    ZeroMemory(&ccab, sizeof(ccab));

    sprintf(ccab.szCabPath, "%p", &destination);

    ERF erf;

    HFDI fci = FCICreate(
        &erf,
        fciFilePlaced,
        fciAlloc,
        fciFree,
        fciOpen,
        fciRead,
        fciWrite,
        fciClose,
        fciSeek,
        fciDelete,
        fciGetTempFile,
        &ccab,
        nullptr
    );

    char sourceFile[24] {};
    sprintf(sourceFile, "%p", &source);

    FCIAddFile(
        fci,
        sourceFile,
        fileName,
        FALSE,
        fciGetNextCabinet,
        fciStatus,
        fciGetOpenInfo,
        tcompTYPE_MSZIP);

    FCIFlushCabinet(
        fci,
        FALSE,
        fciGetNextCabinet,
        fciStatus);

    FCIDestroy(fci);
}
