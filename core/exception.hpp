#pragma once

#include <string>
#include <stdexcept>
#include <string_view>


namespace scymnous
{

class sc_exception : public std::runtime_error
{

public:
    sc_exception(const char * what) : std::runtime_error{what}
    {}

    sc_exception(const std::string& what) : std::runtime_error{what}
    {}

    sc_exception(std::string_view what) : std::runtime_error{std::string{what.data(), what.size()}}
    {}
};





} // namespace scymnous
