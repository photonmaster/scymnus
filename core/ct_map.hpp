#pragma once

#include <array>
#include <exception>

namespace scymnus {

template <class Key, class Value, std::size_t Size> struct ct_map {
    
    constexpr Value at(const Key &key) const {
        auto it = std::find_if(begin(data), end(data),
                               [&key](const auto &v) { return v.first == key; });
        
        if (it != end(data)) {
            return it->second;
        }
        return "HTTP/1.1 500 Internal Server Error\r\n";
    }
    
    constexpr Value at(const Key &key, const Key &default_key) const {
        auto it = std::find_if(begin(data), end(data),
                               [&key](const auto &v) { return v.first == key; });
        
        if (it != end(data)) {
            return it->second;
        }
        return at(default_key); // double pass but rare case
    }
    
    std::array<std::pair<Key, Value>, Size> data;
};

} // namespace scymnus
