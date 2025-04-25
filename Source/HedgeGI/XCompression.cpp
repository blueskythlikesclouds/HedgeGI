#include "XCompression.h"
#include "Utilities.h"

// Reference: https://github.com/gildor2/UEViewer/blob/master/Unreal/UnCoreCompression.cpp

namespace
{
    int msPackRead(mspack_file* file, void* buffer, int bytes)
    {
        const auto stream = reinterpret_cast<hl::stream*>(file);

        uint16_t size;
        stream->read_obj(size);
        hl::endian_swap(size);

        if ((size & 0xFF00) == 0xFF00)
        {
            stream->seek(hl::seek_mode::cur, 1);
            stream->read_obj(size);
            hl::endian_swap(size);
        }

        assert(bytes > size);

        return (int) stream->read(size, buffer);
    }

    int msPackWrite(mspack_file* file, void* buffer, int bytes)
    {
        return (int) reinterpret_cast<hl::stream*>(file)->write((size_t) bytes, buffer);
    }
    
    void* msPackAlloc(mspack_system* self, size_t bytes)
    {
        return operator new(bytes);
    }

    void msPackFree(void* ptr)
    {
        operator delete(ptr);
    }

    void msPackCopy(void* src, void* dst, size_t bytes)
    {
        memcpy(dst, src, bytes);
    }
    
    mspack_system msPackSystem =
    {
        nullptr,
        nullptr,
        msPackRead,
        msPackWrite,
        nullptr,
        nullptr,
        nullptr,
        msPackAlloc,
        msPackFree,
        msPackCopy
    };
}

struct XCompressHeader
{
    uint32_t identifier;

    uint16_t version;
    uint16_t reserved;

    uint32_t contextFlags;

    uint32_t flags;
    uint32_t windowSize;
    uint32_t compressionPartitionSize;

    size_t uncompressedSize;
    size_t compressedSize;

    uint32_t uncompressedBlockSize;
    uint32_t compressedBlockSizeMax;
};

bool XCompression::checkSignature(void* data)
{
    return *(uint32_t*)data == 0xEE12F50F;
}

void XCompression::decompress(void* data, size_t dataSize, hl::mem_stream& dstStream)
{
    const XCompressHeader* header = static_cast<XCompressHeader*>(data);

    const int windowSize = (int)log2(HL_SWAP_U32(header->windowSize));
    const int compressionPartitionSize = (int)HL_SWAP_U32(header->compressionPartitionSize);
    const size_t uncompressedSize = HL_SWAP_U64(header->uncompressedSize);
    const off_t uncompressedBlockSize = HL_SWAP_U32(header->uncompressedBlockSize);

    uint8_t* srcBytes = (uint8_t*)(header + 1);

    for (size_t i = 0; dstStream.get_size() < uncompressedSize && srcBytes < (uint8_t*)data + dataSize; i += uncompressedBlockSize)
    {
        const uint32_t compressedSize = HL_SWAP_U32(*(uint32_t*)srcBytes);
        srcBytes += sizeof(int);

        hl::readonly_mem_stream srcStream(srcBytes, compressedSize);

        lzxd_stream* lzx = lzxd_init(
            &msPackSystem,
            reinterpret_cast<mspack_file*>(&srcStream),
            reinterpret_cast<mspack_file*>(&dstStream),
            windowSize,
            0,
            compressionPartitionSize,
            uncompressedBlockSize,
            FALSE);

        const auto result = lzxd_decompress(lzx, uncompressedBlockSize);
        assert(result == MSPACK_ERR_OK);
        lzxd_free(lzx);

        srcBytes += compressedSize;
    }
}