#pragma once
#include <string>

#include "headers_container.hpp"


namespace scymnus
{
struct http_response
{

    res_headers headers;
    std::optional<uint16_t>  status_code;
    std::string body;

    void set_header(std::string key, std::string value)
    {
        headers.erase(key);
        headers.emplace(std::move(key), std::move(value));
    }
    void add_header(std::string key, std::string value)
    {
        headers.emplace(std::move(key), std::move(value));
    }




//        std::optional<std::string> get_header_value(std::string_view field)
//        {
//            if (headers.count(field))
//            {
//                return to_string(headers.find(field)->second);
//            }
//            return {};
//        }

//        void set_header_value(std::string_view field, std::string_view value)
//        {
//            //TODO: implement this
//        }

        http_response() = default;
        explicit http_response(uint16_t sc) : status_code(sc) {}
        http_response(http_response&&) = default;
        http_response& operator= (http_response&& r) noexcept = default;
        http_response& operator=(const http_response& r)  = delete;
        http_response(const http_response& r) = delete;
};

} //scymnus
