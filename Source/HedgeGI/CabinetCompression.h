﻿#pragma once

class CabinetCompression
{
public:
    static bool checkSignature(void* data);
    static void load(hl::archive& archive, void* data, size_t dataSize);
    static hl::archive load(void* data, size_t dataSize);
    static void save(const hl::archive& archive, hl::stream& destination, char* fileName);
};
