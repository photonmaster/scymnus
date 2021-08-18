#pragma once
#include <string_view>

#define CT_(...) ([]() { return __VA_ARGS__; })

namespace scymnus {

// compare 2 strings
constexpr bool ct_strcmp(const char *lhs, const char *rhs) {
    return std::string_view(lhs) == rhs;
}

int constexpr ct_str_length(const char *str) {
    return *str ? 1 + ct_str_length(str + 1) : 0;
}

} // namespace scymnus
