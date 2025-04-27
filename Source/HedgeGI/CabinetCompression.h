#pragma once

class CabinetCompression
{
public:
    static bool checkSignature(void* data);
    static void decompress(void* data, size_t dataSize, hl::stream& destination);
    static void compress(hl::stream& source, hl::stream& destination, char* fileName, int windowSize);
    static void save(const hl::archive& archive, hl::stream& destination, char* fileName);
};
