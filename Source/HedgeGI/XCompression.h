#pragma once

class XCompression
{
public:
    static bool checkSignature(void* data);
    static void decompress(void* data, size_t dataSize, hl::stream& dstStream);
    static void compress(hl::stream& source, hl::stream& destination);
    static void save(const hl::archive& archive, hl::stream& destination);
};
