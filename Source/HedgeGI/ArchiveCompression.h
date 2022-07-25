#pragma once

class ArchiveCompression
{
public:
    static void load(hl::archive& archive, void* data, size_t dataSize);
    static hl::archive load(void* data, size_t dataSize);
};