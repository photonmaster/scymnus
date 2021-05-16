#pragma once

#include <optional>


namespace scymnous {

template<class T>
class init
{

    T value_;

public:
    template<class F>
    constexpr init(F&& f) : value_{std::forward<F>(f)()}    {

    }

    constexpr auto value() const{
        return value_;
    }
};


//template<typename T, auto Initializer>
//class defaulted : public std::optional<T>
//{
//    using std::optional<T>::optional;


//public:

//    constexpr defaulted() noexcept
// : std::optional<T>{Initializer()}
//    {

//    }

//    using std::optional<T>::has_value;
//    using std::optional<T>::emplace;
//    using std::optional<T>::value;

//    constexpr T& value() & {
//        if (!std::optional<T>::has_value())
//            std::optional<T>::emplace(Initializer());
//        return  value();
//    }

//    template <class U>
//    constexpr T value_or( U&& default_value ) const& = delete;

//    template <class U>
//    constexpr T value_or( U&& default_value ) && = delete;

//    constexpr const T& value() const & {
//        return  value();
//    }



//    constexpr T&& value() && {
//        if (!std::optional<T>::has_value())
//            std::optional<T>::emplace(Initializer());
//        return  value();
//    }


//    constexpr const T&& value() const && {
//        return std::optional<T>::value();

//    }
//};

} //namespace scymnous
