#include "Pch.h"

namespace hl
{
    std::exception make_exception(error_type err) { __debugbreak(); return std::exception(); }
    std::exception make_exception(error_type err, const char* what_arg) { __debugbreak(); return std::exception(); }
}

