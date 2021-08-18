#pragma once
#include <iostream>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace scymnus {
template <class T>
concept string_like = std::is_convertible_v<T, std::string_view> ||
                      std::is_convertible_v<T, std::string_view> ||
                      std::is_convertible_v<T, std::wstring_view> ||
                      std::is_convertible_v<T, std::u8string_view> ||
                      std::is_convertible_v<T, std::u16string_view> ||
                      std::is_convertible_v<T, std::u32string_view>;

namespace detail {

// trait for types that must be treated as strings
template <class T> struct is_string_like : std::false_type {};

template <string_like T> struct is_string_like<T> : std::true_type {};

} // namespace detail

template <class T> using is_string_like = detail::is_string_like<T>;

template <class T>
constexpr bool is_string_like_v = detail::is_string_like<T>::value;

template <class T> struct is_optional : std::false_type {};

template <class T> struct is_optional<std::optional<T>> : std::true_type {};

template <class T> constexpr bool is_optional_v = is_optional<T>::value;

template <typename> struct is_vector : std::false_type {};

template <typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type {};

template <class T> constexpr bool is_vector_v = is_vector<T>::value;


} // namespace scymnus
