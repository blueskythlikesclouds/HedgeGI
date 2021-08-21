#pragma once

class CabinetCompression
{
public:
    static hl::archive load(void* data, size_t dataSize);
    static void save(const hl::archive& archive, hl::stream& destination, char* fileName);
};
