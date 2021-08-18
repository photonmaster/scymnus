#pragma once

#include "named_tuple.hpp"

namespace scymnus {

class validation_exception : public std::exception {
public:
    validation_exception(std::string msg) : msg_(std::move(msg)) {}

    ~validation_exception() noexcept {}

    const char *what() const noexcept { return msg_.c_str(); }

private:
    std::string msg_;
};

template <class T, class Constraint, typename Enable = void>
struct validation_visitor;

// template<class T, class U>
// struct validation_visitor<T, constraints::min<U>>
//{
//    static_assert(false, "type of field and field of min<> are different");
//};

// template<class T, class U>
// struct validation_visitor<T, constraints::max<U>>
//{

//    static_assert(false, "type of field and field of max<> are different");
//};

template <class T>
struct validation_visitor<T, constraints::min<T>,
                          std::enable_if_t<std::is_integral_v<T>>> {
    static bool validate(T value, constraints::min<T> constraint) {

        if (value < constraint.value())
            throw validation_exception("min value constraint violation");
        return true;
    }
};

template <class T>
struct validation_visitor<T, constraints::max<T>,
                          std::enable_if_t<std::is_integral_v<T>>> {

    static bool validate(T value, constraints::max<T> constraint) {

        if (value < constraint.value())

            throw validation_exception("max value constraint violation");
        return true;
    }
};

template <> struct validation_visitor<int, constraints::validation<bool>> {
    static bool validate(int value, constraints::validation<bool> constraint) {
        return true;
    }
};

class validator {

public:
    validator(){};

    // it gets a field type from a model object
    template <class T, class V> void validate(T &&field, V &&value) {

        // TODO: static assert for non field types

        using properties = std::remove_cvref_t<decltype(field.properties)>;
        using field_type = std::remove_cvref_t<decltype(field)>;

        for_each(field.properties, [&value](auto &&constraint) {
            using constraint_type = std::remove_cvref_t<decltype(constraint)>;

            validation_visitor<typename field_type::type, constraint_type> val{};
            val.validate(value, constraint);
        });
    }

    // iterate on properties
};
} // namespace scymnus
