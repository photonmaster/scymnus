#pragma once

#include <optional>
#include <string_view>
#include <charconv>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "external/json.hpp"
#include "server/headers_container.hpp"
#include "http/query_parser.hpp"
#include "utilities/utils.hpp"


using json = nlohmann::json;

//compile time parsing string to int

namespace cutil {

constexpr bool is_digit(char c) {
    return c <= '9' && c >= '0';
}

constexpr int stoi_impl(const char *str, int value = 0) {
    return *str ?
                is_digit(*str) ?
                               stoi_impl(str + 1, (*str - '0') + value * 10)
                               : throw "compile-time-error: not a digit"
                : value;
}

constexpr int stoi(const char *str) {
    return stoi_impl(str);
}

}

namespace scymnous {


namespace path {

template<typename T>
struct traits;

//"string", "number", "integer", "boolean", "array" or "file".


template<>
struct traits<int>
{
    static int get(std::string_view path, int index)
    {
        std::vector<std::string_view> parts = utils::split(path);
        std::string t{parts[index].data(), parts[index].size()};

        try {
            return  boost::lexical_cast<int>(t);
        }

        catch(boost::bad_lexical_cast& exp){
            throw std::runtime_error("Value for path parameter cannot be converted to the specified type");
        }

    }
};



template<>
struct traits<std::string>
{
    static std::string get(std::string_view path, int index)
    {

        std::vector<std::string_view> parts = utils::split(path);
        std::string t{parts[index].data(), parts[index].size()};

        return  t;

    }
};



}//namespace path


namespace header {

template<typename T>
struct traits;

//"string", "number", "integer", "boolean", "array" or "file".


#define SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(type) \
    template<> \
    struct traits<type> { \
    static type get(headers_container const &v, std::string_view field) \
{   \
   try { \
    if (v.count(field)) \
{ \
type value; \
\
if (std::holds_alternative<std::string_view>(v.find(field)->second)){ \
    auto sv = std::get<0>(v.find(field)->second); \
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value); \
    if (result.ec == std::errc::invalid_argument) { \
        std::cout << "Could not convert.\n"; \
    } \
} \
else { \
    std::string str = to_string(v.find(field)->second); \
    value = boost::lexical_cast<type>(str); \
} \
return value; \
} \
    else{ \
    std::string msg = "header parameter: "; \
    msg.append(field); \
    msg.append(" is missing but required"); \
    throw std::logic_error(msg); \
} \
} \
    catch(boost::bad_lexical_cast& exp){ \
    throw std::runtime_error("Value for header parameter with name: " + std::string(field) + " cannot be converted to the specified type"); \
} \
} \
    static std::optional<type> get_optional(headers_container const &v,const std::string_view field) {\
    if (!v.count(field)) \
    return {}; \
    std::string value = to_string(v.find(field)->second); \
    try { \
    return  boost::lexical_cast<type>(value); \
} \
    catch(boost::bad_lexical_cast& exp){ \
    throw std::runtime_error("Value for header parameter with name: " + std::string(field) + " cannot be converted to the specified type"); \
} \
} \
};


SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned char)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(signed char)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(short)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned short)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(int)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned int)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(long)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned long)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(long long)
SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned long long)
//SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(float)
//SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(double)
//SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE(long double)

#undef SCYMNOUS_HEADER_PARAMETER_SPECIALIZE_CORE_TYPE



template<>
struct traits<bool> {
    static bool get( const headers_container &v, std::string_view &field) {

        if (v.count(field)) {

            std::string value = to_string(v.find(field)->second);
            boost::algorithm::trim (value);

            if (value == "true" || value == "True") {
                return true;
            }

            if (value == "false" || value == "False") {
                return false;
            }

            std::string msg = "header parameter: ";
            msg.append(field);
            msg.append(" must be a boolean value (True/true or False/false)");
            throw std::runtime_error{msg};
        } else {
            std::string msg = "header parameter: ";
            msg.append(field);
            msg.append(" is missing but required");
            throw std::runtime_error{msg};
        }
    }

    static std::optional<bool> get_optional(headers_container const &v,
                                            std::string_view field) {
        if (!v.count(field)) {
            return {};
        }

        else {
            std::string value = to_string(v.find(field)->second);
            boost::algorithm::trim (value);

            if (value == "true" || value == "True") {
                return true;
            }

            if (value == "false" || value == "False") {
                return false;
            }

            std::string msg = "header parameter: ";
            msg.append(field);
            msg.append(" must be a boolean value");
            throw std::runtime_error{msg};
        }
    }
};


template<>
struct traits<std::string> {
    static std::string get(headers_container const &v, std::string_view field) {
//      std::cout << " requesting header: " << field << std::endl;
//      std::cout << "headers_cntainer contents: \n";
        for ( auto iter =v.begin(); iter != v.end(); ++iter )
              std::cout << to_string(iter->first) << '\t' << to_string(iter->second) << std::endl;

        if (v.count(field)) {
//            std::cout << " field was found " << std::endl;
//            std::cout << " found value was: " << to_string(v.find(field)->second) << std::endl;

            return to_string(v.find(field)->second);
        }

        else {
//          std::cout << " field was NOT found " << std::endl;
            std::string msg = "header parameter: ";
            msg.append(field);
            msg.append(" is missing but required");
            throw std::runtime_error{msg};
        }
    }

    static std::optional<std::string> get_optional(headers_container const &v,
                                                   std::string_view field) {
        if (!v.count(field)) {
            return {};
        } else {
            return to_string(v.find(field)->second);
        }
    }
};


//json array values
template<class T>
struct traits<std::vector<T>> {
    static std::vector<T> get(headers_container const &v, std::string_view field) {
        if (v.count(field)) {
            try {
                auto data = json::parse(to_string(v.find(field)->second)).get<std::vector<T>>();
                return data;
            }

            catch (std::exception &exp) {

                std::string msg = "error in header parameter ";
                msg.append(field);
                msg.append(": ");
                msg.append(exp.what());
                throw std::runtime_error(msg);
            }
        } else {
            std::string msg = "header parameter: ";
            msg.append(field);
            msg.append(" is missing but required");
            throw std::runtime_error(msg);
        }
    }

    static std::optional<std::vector<T>> get_optional(headers_container const &v,
                                                      std::string_view field) {
        if (!v.count(field)) {
            return {};
        }

        else {
            try {
                auto data = json::parse(to_string(v.find(field)->second)).get<std::vector<T>>();
                return data;
            }

            catch (std::exception &exp) {

                std::string msg = "error in header parameter ";
                msg.append(field);
                msg.append(": ");
                msg.append(exp.what());
                throw std::runtime_error(msg);
            }
        }
    }
};

template<class T>
struct traits<std::optional<T>> {

    static std::optional<T> get(headers_container const &v,
                                const std::string field) try {
        std::optional<T> value = traits<T>::get_optional(v, field);
        return value;
    } catch (std::runtime_error &exp) {
        throw;
    }
};


}//header

namespace query {

template<typename T>
struct traits;

//TODO: fix error handling when calling from_chars
#define SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(type) \
    template<> \
    struct traits<type> { \
    static type get(query_string const &v,const std::string_view key) \
{   \
    auto value = v.get(key); \
    if (!value) { \
    std::string msg = "query parameter: "; \
    msg.append(key); \
    msg.append("is missing but required"); \
    throw std::logic_error(msg); \
} \
 std::string_view sv = value.value(); \
    type ret; \
    auto result = std::from_chars(sv.data(), sv.data() + sv.size(), ret); \
    if (result.ec == std::errc::invalid_argument) { \
        std::cout << "Could not convert.\n"; \
    } \
return ret; \
} \
};


SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned char)
//SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(signed char)
//SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(wchar_t)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(short)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned short)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(int)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned int)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(long)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned long)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(long long)
SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(unsigned long long)
//SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(float)
//SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(double)
//SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE(long double)
#undef SCYMNOUS_QUERY_PARAMETER_SPECIALIZE_CORE_TYPE

}//query
} //namespace scymnous
