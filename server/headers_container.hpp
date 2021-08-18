#pragma once

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <iostream>
#include <map>
#include <memory_resource>
#include <string_view>
#include <unordered_map>

namespace scymnus {

// only ascii
char to_lower(char c) {
    if (c <= 'Z' && c >= 'A')
        return c - ('Z' - 'z');
    return c;
}

inline bool iless(char lhs, char rhs) { return to_lower(lhs) < to_lower(rhs); }

//'A': 65, 'a': 97
// comapre ascii chars
inline bool icompare(char lhs, char rhs) {
    if (lhs == rhs)
        return true;
    return lhs > rhs ? static_cast<unsigned>(rhs) + 32L == lhs
                     : static_cast<unsigned>(lhs) + 32L == rhs;
}

inline char toupper(char c) { return ('a' <= c && c <= 'z') ? c ^ 0x20 : c; }

struct icomp {

    bool operator()(const std::pmr::string &l, const std::pmr::string &r) const {
        return std::lexicographical_compare(std::cbegin(l), std::cend(l),
                                            std::cbegin(r), std::cend(r), iless);
    }
};

using headers_t = std::pmr::multimap<std::pmr::string, std::pmr::string, icomp>;

} // namespace scymnus
