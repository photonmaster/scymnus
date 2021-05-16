#pragma once
#include <string_view>

namespace scymnous {
namespace utils {


//REF: https://github.com/envoyproxy (envoy proxy)

constexpr std::string_view whitespace{" \t\f\v\n\r"};

inline std::string_view ltrim(std::string_view source) {
    const std::string_view::size_type pos = source.find_first_not_of(whitespace);
    if (pos != std::string_view::npos) {
        source.remove_prefix(pos);
    } else {
        source.remove_prefix(source.size());
    }
    return source;
}

inline std::string_view rtrim(std::string_view source) {
    const std::string_view::size_type pos = source.find_last_not_of(whitespace);
    if (pos != std::string_view::npos) {
        source.remove_suffix(source.size() - pos - 1);
    } else {
        source.remove_suffix(source.size());
    }
    return source;
}

inline std::string_view trim(std::string_view source) { return ltrim(rtrim(source)); }




//REF: https://www.bfilipek.com/2018/07/string-view-perf.html (based on JFT's comment)

[[nodiscard]] inline std::vector<std::string_view> split(std::string_view sv, std::string_view delimeter = "/")
{
    std::vector<std::string_view> output;
    for (auto first = sv.data(), second = sv.data(), last = first + sv.size();
         second != last && first != last; first = second + 1) {
        second = std::find_first_of(first, last, std::cbegin(delimeter), std::cend(delimeter));

        if (first != second)
            output.emplace_back(first, second - first);
    }

    return output;
}


} //namespace utils
} //namespace scymnous
