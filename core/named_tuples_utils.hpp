#pragma once

#include <optional>

#include "core/enumeration.hpp"
#include "core/traits.hpp"
#include "external/json.hpp"
#include "named_tuple.hpp"
#include "properties/defaulted.hpp"

namespace nlohmann {

template <typename T> struct adl_serializer<std::optional<T>> {
    static void to_json(json &j, const std::optional<T> &opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt;
        }
    }
    
    static void from_json(const json &j, std::optional<T> &opt) {
        
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();
        }
    }
};

template <typename... T> struct adl_serializer<scymnus::model<T...>> {
    static void to_json(json &j, const scymnus::model<T...> &p) {
        // foreach
        for_each(p, [&j](const auto &f, const auto &v) {
            using properties = std::remove_cvref_t<decltype(f.properties)>;
            using type = std::remove_cvref_t<decltype(v)>;
            
            if constexpr (scymnus::is_optional_v<type>) {
                if (!v) {
                    using nested_type = type::value_type;
                    
                    if constexpr (scymnus::has_init<properties>::value) {
                        constexpr int idx =
                            scymnus::tl::index_if<scymnus::is_init, properties>::value;
                        j[f.name] = std::get<idx>(f.properties).value();
                        return;
                    }
                } else {
                    j[f.name] = *v;
                    return;
                }
            }
            
            j[f.name] = v;
        });
    }
    
    static void from_json(const json &j, scymnus::model<T...> &p) {
        for_each(p, [&](auto &&f, auto &&v) {
            using properties = std::remove_cvref_t<decltype(f.properties)>;
            using type = std::remove_cvref_t<decltype(v)>;
            
            if constexpr (scymnus::is_optional_v<type>) {
                if (!j.contains(f.name)) {
                    using nested_type = type::value_type;
                    
                    if constexpr (scymnus::has_init<properties>::value) {
                        constexpr int idx =
                            scymnus::tl::index_if<scymnus::is_init, properties>::value;
                        v = std::get<idx>(f.properties).value();
                    }
                    
                } else {
                    j.at(f.name).get_to(v);
                }
            }
            
            else {
                j.at(f.name).get_to(v);
            }
        });
    }
};

} // namespace nlohmann
