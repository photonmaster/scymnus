#pragma once

#include <optional>


namespace scymnus {

template<auto Callable>
class init
{

public:
    decltype(Callable) value_;
    constexpr init() : value_{Callable}    {

    }

    constexpr auto value() const{
        return value_();
    }
};

template<class T>
struct is_init : std::false_type
{};


template<auto whatever>
struct is_init<init<whatever>> : std::true_type
{};


template <typename Tuple>
struct has_init;

template <typename... Us>
struct has_init<std::tuple<Us...>> : std::disjunction<is_init<Us>...> {};


} //namespace scymnus
