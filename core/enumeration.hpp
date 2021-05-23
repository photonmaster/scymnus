#pragma once

#include <exception>
#include <array>
#include <boost/lexical_cast.hpp>



//enumeration
namespace scymnus
{

template <class T,class... sl> struct enumeration
{
    static_assert(sizeof...(sl) > 0, "enumeration type must have at least one element");

public:
    enumeration() {
        value_ = data()[0];
    }

    enumeration(T&& v) try : value_(std::move(v))
    {
        validate(value_);
    }
    catch(std::exception& exp){
        throw;
    }

    enumeration(const T& v)
    {
        try{
            validate(v);
            value_ = v;
        }


        catch(std::exception& exp){
            throw;
        }
    }

    std::ptrdiff_t index() const
    {
        return index_;
    }

    const T& value() const
    {
        return value_;
    }

    T& value()
    {
        return value_;
    }


    operator T() const //&
    {
        return value_;
    }

    static std::array<T,sizeof...(sl)>&  data() {
        static std::array<T,sizeof...(sl)> data_ = {boost::lexical_cast<T>(sl::str())...};
        return data_;
    }


private:

    void validate(const T& v){
        auto contains = std::find(std::begin(data()), std::end(data()), v);

        if (contains != std::end(data())){
            index_ = std::distance(std::begin(data()), contains);
            return;
        }
        else{
            throw std::logic_error("not a proper enumerator value");
        }
    }
    T value_;
    std::ptrdiff_t index_{-1};
};


} //namespace scymnus
