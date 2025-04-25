#pragma once

class ArchiveCompression
{
public:
    static bool decompress(void* data, size_t dataSize, hl::mem_stream& destination);
    static void load(hl::archive& archive, void* data, size_t dataSize);
    static hl::archive load(void* data, size_t dataSize);
};