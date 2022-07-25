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
        hl::nstring splitPathBuf(filePath);

        for (auto splitPath : hl::path::split_iterator2<>(splitPathBuf))
        {
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
