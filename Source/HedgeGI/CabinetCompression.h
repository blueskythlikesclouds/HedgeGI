#pragma once

class CabinetCompression
{
public:
    static bool checkSignature(void* data);
    static void decompress(void* data, size_t dataSize, hl::mem_stream& destination);
    static void save(const hl::archive& archive, hl::stream& destination, char* fileName);
};
