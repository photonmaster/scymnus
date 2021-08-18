#pragma once

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "core/traits.hpp"
#include "meta/ct_string.hpp"
#include "meta/ct_utils.hpp"
#include "properties/defaulted.hpp"
#include "typelist.hpp"

namespace scymnus {

// TODO: remove this from here
using std::tuple;

template <typename T, typename Tuple> struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>>
    : std::disjunction<std::is_same<T, Us>...> {};

template <unsigned N> struct name {

    char buf[N + 1] = {};
    constexpr unsigned size() const { return N; }
    constexpr name(char const *s) {
        for (unsigned i = 0; i != N; ++i)
            buf[i] = s[i];
    }

    constexpr operator char const *() const { return buf; }
    constexpr char const *str() const { return buf; }
};

template <unsigned N> name(char const (&)[N]) -> name<N - 1>;

template <unsigned N> struct description {

    char buf[N + 1] = {};
    constexpr unsigned size() const { return N; }
    constexpr description(char const *s) {
        for (unsigned i = 0; i != N; ++i)
            buf[i] = s[i];
    }

    constexpr operator char const *() const { return buf; }
    constexpr char const *str() const { return buf; }
};

template <unsigned N> description(char const (&)[N]) -> description<N - 1>;

template <class T> struct is_name : std::false_type {};

template <auto n> struct is_name<name<n>> : std::true_type {};

template <class T> struct is_description : std::false_type {};

template <auto n> struct is_description<description<n>> : std::true_type {};

template <typename Tuple> struct has_name : std::false_type {};

template <typename... Us>
struct has_name<std::tuple<Us...>> : std::disjunction<is_name<Us>...> {};

template <typename Tuple> struct has_description;

template <typename... Us>
struct has_description<std::tuple<Us...>>
    : std::disjunction<is_description<Us>...> {};

template <auto... p> class properties {
public:
    properties() = default;

    const char *get_name() { return "s"; }

    static constexpr std::tuple<std::remove_cvref_t<decltype(p)>...> properties_{
        p...};

    constexpr const char *name() {
        if constexpr (has_name<std::remove_cvref_t<decltype(properties_)>>::value) {
            constexpr int idx =
                tl::index_if<is_name,
                             std::remove_cvref_t<decltype(properties_)>>::value;
            return std::get<idx>(properties_).str();
        } else
            return "";
    }
    constexpr const char *description() {
        if constexpr (has_description<
                          std::remove_cvref_t<decltype(properties_)>>::value) {
            constexpr int idx =
                tl::index_if<is_description,
                             std::remove_cvref_t<decltype(properties_)>>::value;
            return std::get<idx>(properties_).str();
        } else
            return "";
    }
};

template <class T> struct is_properties : std::false_type {};
template <auto... Properties>
struct is_properties<properties<Properties...>> : std::true_type {};

template <meta::ct_string N, class U, auto... Properties> struct field {
    static constexpr char const *name = N;
    static constexpr std::tuple<std::remove_const_t<decltype(Properties)>...>
        properties{Properties...};
    using type = U;
};

template <class T> using internal_type = typename T::type;

template <class... N>
class model
    : public tl::transform<
          internal_type,
          tl::remove_if<is_properties, std::tuple<N...>, std::tuple<>>> {

    template <class Model, std::size_t... I>
    static constexpr decltype(auto) get_names(std::index_sequence<I...>) {
        return std::array<const char *const, sizeof...(I)>{
            std::remove_cvref_t<decltype(std::get<I>(Model{}))>::name...};
    }

    template <class Model, std::size_t... I>
    static constexpr decltype(auto) get_properties(std::index_sequence<I...>) {
        // TODO: invistigate, we can't use CTAD as this will not work if model has
        // one field.
        return std::tuple<decltype(std::remove_cvref_t<decltype(std::get<I>(
                                       Model{}))>::properties)...>{
            std::remove_cvref_t<decltype(std::get<I>(Model{}))>::properties...};
    }

    using fields_tuple =
        tl::remove_if<is_properties, tl::typelist<N...>, std::tuple<>>;
    using fields_internal_type = tl::transform<internal_type, fields_tuple>;

public:
    using fields_internal_type::tuple;
    constexpr model() = default;
    constexpr model(model &&obj) = default;
    model &operator=(model &&res) = default;
    constexpr model(const model &obj) = default;
    model &operator=(const model &) = default;

    static constexpr size_t object_size = std::tuple_size<fields_tuple>::value;

    static constexpr std::array<const char *const, object_size> names =
        get_names<fields_tuple>(std::make_index_sequence<object_size>{});

    static constexpr std::tuple properties =
        get_properties<fields_tuple>(std::make_index_sequence<object_size>{});

    constexpr bool has_field(const char *field) const noexcept {
        for (int i = 0; i < object_size; i++) {
            if (equal(names[i], field))
                return true;
        }
        return false;
    }

    template <meta::ct_string u> constexpr auto &get() {
        constexpr int idx = index(u.str());
        static_assert(idx >= 0,
                      "named_tuple: get<>() called with a non existing field name");

        return std::get<idx>(*this);
    }

    template <meta::ct_string u> constexpr const auto &get() const {
        constexpr int idx = index(u.str());
        static_assert(idx >= 0,
                      "named_tuple: get<>() called with a non existing field name");

        return std::get<idx>(*this);
    }

    /// if a field is of an optional type:
    /// if an init meta-property is present and a value has not been set, then the
    /// init meta-property is applied otherwise field is default constructed
    template <class T> constexpr auto &operator[](T field) {
        constexpr int idx = index(field());
        static_assert(
            idx >= 0,
            "named_tuple: operator[] called with a non existing field name");

        using type = std::remove_cvref_t<decltype(std::get<idx>(*this))>;

        if constexpr (is_optional_v<type>) {
            if (std::get<idx>(*this)) {
                return *(std::get<idx>(*this));
            }

            using properties_t =
                std::remove_cvref_t<decltype(std::get<idx>(properties))>;

            if constexpr (has_init<properties_t>::value) {
                constexpr int prop_idx = tl::index_if<is_init, properties_t>::value;
                using initializer =
                    typename std::tuple_element<prop_idx, properties_t>::type;
                std::get<idx>(*this) = initializer{}.value();
                return *(std::get<idx>(*this));
            } else {
                std::get<idx>(*this) = typename type::value_type{};
                return *(std::get<idx>(*this));
            }
        }

        else {
            return std::get<idx>(*this);
        }
    }

    static constexpr const char *name() {
        using prop =
            tl::select_if<is_properties, std::tuple<N...>, std::tuple<>>::type;
        if constexpr (std::tuple_size<prop>::value)
            return std::get<0>(prop{}).name();
        else
            return "";
    }

    static constexpr const char *description() {
        using prop =
            tl::select_if<is_properties, std::tuple<N...>, std::tuple<>>::type;

        if constexpr (std::tuple_size<prop>::value)
            return std::get<0>(prop{}).description();
        else
            return "";
    }

    static constexpr int index(char const *field) {
        for (int i = 0; i < names.size(); i++) {
            if (ct_strcmp(names[i], field))
                return i;
        }
        return -1;
    }
};

template <meta::ct_string N, class T, class Properties> struct field_wrapper {
    Properties properties;
    constexpr field_wrapper(Properties &&prop) : properties(std::move(prop)) {}

    static constexpr const char *name = N.str();
    //    static constexpr auto properties = Properties;
    using type = T;
};

template <size_t I, class... N> auto geta(const model<N...> &model) {

    using fields_ =
        tl::remove_if<is_properties, tl::typelist<N...>, std::tuple<>>;
    using fields_internal_type_ = tl::transform<internal_type, fields_>;
    using type = typename std::tuple_element<I, fields_internal_type_>::type;

    constexpr const char *str = model.names[I];
    constexpr size_t l = strlen(str);
    constexpr meta::ct_string<l> a(str);

    return field_wrapper<a, type, decltype(std::get<I>(model.properties))>(
        std::get<I>(model.properties));
}

namespace detail {

template <class... T, class F, std::size_t... I>
constexpr void for_each(model<T...> &&t, F &&f, std::index_sequence<I...>) {
    auto v = std::initializer_list<int>{
        (std::forward<F>(f)(geta<I>(t), std::get<I>(t)), 0)...};
}

template <class... T, class F, std::size_t... I>
constexpr void for_each(model<T...> &t, F &&f, std::index_sequence<I...>) {
    auto v = std::initializer_list<int>{
        (std::forward<F>(f)(geta<I>(t), std::get<I>(t)), 0)...};
}

template <class... T, class F, std::size_t... I>
constexpr void for_each(const model<T...> &t, F &&f,
                        std::index_sequence<I...>) {
    auto v = std::initializer_list<int>{
        (std::forward<F>(f)(geta<I>(t), std::get<I>(t)), 0)...};
}

template <class... T, class F, std::size_t... I>
constexpr void for_each(std::tuple<T...> &&t, F &&f,
                        std::index_sequence<I...>) {
    auto v =
        std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(t)), 0)...};
}

template <class... T, class F, std::size_t... I>
constexpr void for_each(std::tuple<T...> &t, F &&f, std::index_sequence<I...>) {
    auto v =
        std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(t)), 0)...};
}

template <class... T, class F, std::size_t... I>
constexpr void for_each(const std::tuple<T...> &t, F &&f,
                        std::index_sequence<I...>) {
    auto v =
        std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(t)), 0)...};
}
} // namespace detail

template <class NamedTuple, class F>
constexpr void for_each(NamedTuple &&t, F &&f) {
    detail::for_each(t, std::forward<F>(f),
                     std::make_index_sequence<
                         std::tuple_size<std::decay_t<NamedTuple>>::value>{});
}

namespace detail {
template <class T> struct is_model { static constexpr bool value = false; };

template <class... A> struct is_model<model<A...>> {
    static constexpr bool value = true;
};
} // namespace detail

template <class T> using is_model = detail::is_model<std::decay_t<T>>;

template <class T>
constexpr bool is_model_v = detail::is_model<std::decay_t<T>>::value;

} // namespace scymnus

// tuple_size support for named tuples
namespace std {

template <class... Types>
class tuple_size<scymnus::model<Types...>>
    : public std::integral_constant<std::size_t,
                                    scymnus::model<Types...>::object_size> {};
} // namespace std
