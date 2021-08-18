#pragma once

#include "meta/ct_string.hpp"

namespace constraints {

// TODO: check if a hidden field is in a request. In case the field is not
// optional fire an assert.
// Maybe introduce a force_hidden that will not fire
template <class T> class hidden {

    static_assert(std::is_same_v<bool, T>,
                  "template parameter of hidden template must be boolean. That "
                  "is: hidden<bool>");

public:
    bool value_;
    constexpr hidden(T value) : value_(value) {}
    constexpr bool value() const { return value_; }
};

template <class T> class validation {
    static_assert(std::is_same_v<bool, T>,
                  "template parameter of hidden template must be boolean. That "
                  "is: hidden<bool>");
    bool value_;

public:
    constexpr validation(T value) : value_{value} {}

    constexpr bool value() const { return value_; }
};

template <class T> class min {

public:
    T value;
    constexpr min(T v) : value{v} {}
};

template <class T> class max {

public:
    T value;

    constexpr max(T v) : value{} {}
};

} // namespace constraints
