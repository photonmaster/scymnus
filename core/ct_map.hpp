#pragma once

#include <exception>
#include <array>



namespace scymnous
{

template<class Key, class Value, std::size_t Size>
struct ct_map
{

    constexpr Value at(const Key& key) const {
        auto it = std::find_if(begin(data), end(data),
                               [&key](const auto& v) {
                                   return v.first == key;
                               });

        if (it != end(data)) {
            return it->second;
        }
        throw std::range_error("Value not found");

    }

    constexpr Value at(const Key& key, const Key& default_key) const {
        auto it = std::find_if(begin(data), end(data),
                               [&key](const auto& v) {
                                   return v.first == key;
                               });

        if (it != end(data)) {
            return it->second;
        }
        return at(default_key); //double pass but rare case
    }



    std::array<std::pair<Key,Value>,Size> data;
};

}

