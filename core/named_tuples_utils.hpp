#pragma once

#include <optional>

#include "external/json.hpp"
#include "named_tuple.hpp"
#include "core/enumeration.hpp"
#include "core/traits.hpp"
#include "properties/defaulted.hpp"


namespace nlohmann {
template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt;
        }
    }

    static void from_json(const json& j, std::optional<T>& opt) {

        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();
        }
    }
};


}


namespace scymnus {

using json = nlohmann::json;




template<class NamedTuple, typename = std::enable_if_t<is_model_v<NamedTuple>,void>>
void to_json(json& j, const NamedTuple& p) {
    //foreach
    for_each(p,[&j](const auto& f, const auto& v) {
        //to_json(j, v);
        j[f.name] = v;
    });
}



template<class NamedTuple, typename = std::enable_if_t<is_model_v<NamedTuple>,void>>
void from_json(const json& j, NamedTuple& p) {
    for_each(p,[&](auto&& f, auto&& v) {

        using properties = std::remove_cvref_t<decltype(f.properties)>;
        using type = std::remove_cvref_t<decltype(v)>;


        if constexpr (is_optional_v<type>){
            if (!j.contains(f.name)){
                using nested_type = type::value_type;
                if constexpr (scymnus::has_type<init<nested_type>, properties>::value){

                    v = std::get<init<nested_type>>(f.properties).value();
                }
            }
            else {
                j.at(f.name).get_to(v);
            }
        }

        else {
            j.at(f.name).get_to(v);
        }
    });
}
}// namespace scymnus
