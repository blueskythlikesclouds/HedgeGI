#include "Utilities.h"

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