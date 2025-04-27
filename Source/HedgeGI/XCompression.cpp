#include "XCompression.h"
#include "CabinetCompression.h"
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

void XCompression::decompress(void* data, size_t dataSize, hl::stream& dstStream)
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

void XCompression::compress(hl::stream& source, hl::stream& destination)
{
    hl::mem_stream cabStream;
    char fileName[] = "XCompression";
    CabinetCompression::compress(source, cabStream, fileName, 17);

    size_t headerPos = destination.tell();
    destination.write_nulls(sizeof(XCompressHeader) + sizeof(uint32_t));

    uint8_t* dataPtr = cabStream.get_data_ptr<uint8_t>();
    uint32_t dataPos = *reinterpret_cast<uint32_t*>(dataPtr + 0x24);
    uint16_t dataCount = *reinterpret_cast<uint16_t*>(dataPtr + 0x28);

    dataPtr += dataPos;
    for (size_t i = 0; i < dataCount; i++)
    {
        dataPtr += sizeof(uint32_t);

        uint16_t compressedSize = *reinterpret_cast<uint16_t*>(dataPtr);
        dataPtr += sizeof(uint16_t);

        uint16_t uncompressedSize = *reinterpret_cast<uint16_t*>(dataPtr);
        dataPtr += sizeof(uint16_t);

        if (uncompressedSize != 0x8000) 
        {
            destination.write_obj<uint8_t>(0xFF);
            destination.write_obj(HL_SWAP_U16(uncompressedSize));
        }

        destination.write_obj(HL_SWAP_U16(compressedSize));
        destination.write(compressedSize, dataPtr);

        dataPtr += compressedSize;
    }

    destination.write_nulls(5);
    size_t endPos = destination.tell();

    XCompressHeader header = {};
    header.identifier = HL_SWAP_U32(0xFF512EE);
    header.version = HL_SWAP_U32(0x1030000);
    header.windowSize = HL_SWAP_U32(1 << 17);
    header.compressionPartitionSize = HL_SWAP_U32(0x80000);
    header.uncompressedSize = HL_SWAP_U64(source.get_size());
    header.compressedSize = HL_SWAP_U64(endPos - (headerPos + sizeof(XCompressHeader)));
    header.uncompressedBlockSize = HL_SWAP_U32(source.get_size());
    header.compressedBlockSizeMax = HL_SWAP_U32(endPos - (headerPos + sizeof(XCompressHeader) + sizeof(uint32_t)));

    destination.seek(hl::seek_mode::beg, headerPos);
    destination.write_obj(header);
    destination.write_obj(header.compressedBlockSizeMax);

    destination.seek(hl::seek_mode::beg, endPos);
}

void XCompression::save(const hl::archive& archive, hl::stream& destination)
{
    hl::mem_stream source;
    saveArchive(archive, source);

    compress(source, destination);
}
