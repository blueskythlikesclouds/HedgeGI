#include "Utilities.h"
#include "ArchiveCompression.h"

namespace std
{
    template<>
    struct hash<const char*>
    {
        size_t operator()(const char* const& str) const noexcept
        {
            return strHash(str);
        }
    };
}

void loadArchive(hl::archive& archive, const hl::nchar* filePath)
{
    const hl::nchar* ext = hl::path::get_ext(filePath);

    if (hl::path::ext_is_split(ext))
    {
        hl::nstring splitPath(filePath);
        splitPath.resize(splitPath.size() - 3);

        size_t extensionPos = splitPath.size();
        splitPath += HL_NTEXT('l');

        size_t splitCount = ~0;
        
        if (hl::path::exists(splitPath))
        {
            hl::blob blob(splitPath);
            hl::mem_stream stream;

            void* data = blob.data();
            size_t dataSize = blob.size();

            if (ArchiveCompression::decompress(data, dataSize, stream))
            {
                data = stream.get_data_ptr();
                dataSize = stream.get_size();
            }

            if (dataSize >= 8 && *reinterpret_cast<uint32_t*>(data) == 0x324C5241)
                splitCount = *reinterpret_cast<uint32_t*>(hl::ptradd(data, sizeof(uint32_t)));
        }
        
        for (size_t splitIndex = 0; (splitCount == ~0) || (splitIndex < splitCount); splitIndex++)
        {
            wchar_t extension[0x100];
            swprintf_s(extension, L".%02lld", splitIndex);

            splitPath.resize(extensionPos);
            splitPath += extension;

            if (!hl::path::exists(splitPath))
                break;

            hl::blob blob(splitPath);
            ArchiveCompression::load(archive, blob.data(), blob.size());
        }
    }

    else
    {
        hl::blob blob(filePath);
        ArchiveCompression::load(archive, blob.data(), blob.size());
    }
}
