#pragma once

class FileStream
{
    FILE* file{};

public:
    FileStream(const char* filePath, const char* mode)
    {
        fopen_s(&file, filePath, mode);
    }

    ~FileStream()
    {
        close();
    }

    bool isOpen() const
    {
        return file != nullptr;
    }

    void close()
    {
        if (!file)
            return;
        
        fclose(file);
        file = nullptr;
    }

    long tell() const
    {
        return ftell(file);
    }

    void seek(const long position, const int origin) const
    {
        fseek(file, position, origin);
    }

    template <typename T>
    T read() const
    {
        T value;
        fread(&value, sizeof(T), 1, file);
        return value;
    }

    template <typename T>
    void read(T* value, const size_t count) const
    {
        fread(value, sizeof(T), count, file);
    }

    std::string readString() const
    {
        std::string value;

        uint32_t length;
        fread(&length, sizeof(uint32_t), 1, file);

        value.resize(length);

        fread((void*)value.data(), sizeof(std::string::value_type), length, file);

        align();

        return value;
    }

    template <typename T>
    void write(const T& value) const
    {
        fwrite(&value, sizeof(T), 1, file);
    }

    template <typename T>
    void write(T* value, const size_t count) const
    {
        fwrite(value, sizeof(T), count, file);
    }

    void write(const std::string& value) const
    {
        uint32_t length = (uint32_t)value.size();

        fwrite(&length, sizeof(uint32_t), 1, file);
        fwrite(value.data(), sizeof(std::string::value_type), length, file);

        align();
    }

    void align(const size_t alignment = 4) const
    {
        const size_t position = ftell(file);
        const size_t amount = alignment - position % alignment;
        if (amount != alignment)
            fseek(file, (long)amount, SEEK_CUR);
    }
};
