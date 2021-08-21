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

    hl::readonly_mem_stream source(data, dataSize);
    hl::mem_stream destination;

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

    return loadArchive(destination.get_data_ptr(), destination.get_size());
}

void CabinetCompression::save(const hl::archive& archive, hl::stream& destination, char* fileName)
{
    // HedgeLib has no stream overloads for archive save functions, yay...
    hl::mem_stream source;
    {
        const hl::hh::ar::header header =
        {
            0,
            sizeof(hl::hh::ar::header),
            sizeof(hl::hh::ar::file_entry),
            16
        };

        source.write_obj(header);

        for (auto& entry : archive)
        {
            const size_t hhEntryPos = source.tell();
            const size_t nameLen = hl::text::len(entry.name());
            const size_t dataPos = hl::align(hhEntryPos + sizeof(hl::hh::ar::file_entry) + nameLen + 1, 16);

            const hl::hh::ar::file_entry hhEntry =
            {
                (hl::u32)(dataPos + entry.size() - hhEntryPos),
                (hl::u32)entry.size(),
                (hl::u32)(dataPos - hhEntryPos),
                0,
                0
            };

            source.write_obj(hhEntry);

            const auto name = toUtf8(entry.name());
            source.write_arr(nameLen + 1, name.data());

            source.pad(16);
            source.write(entry.size(), entry.file_data());
        }

        source.seek(hl::seek_mode::beg, 0);
    }

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
