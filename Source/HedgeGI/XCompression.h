#pragma once

class XCompression
{
public:
    static bool checkSignature(void* data);
    static void decompress(void* data, size_t dataSize, hl::mem_stream& dstStream);
};
