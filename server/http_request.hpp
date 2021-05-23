#pragma once

#include <string_view>

#include "headers_container.hpp"

namespace scymnus
{


struct http_request
{

    headers_container headers;
    message_data_t body;


    void add_header(std::string_view field,std::string_view value)
    {
        headers.emplace(field, value);
    }




    std::optional<std::string> get_header_value(std::string_view field)
    {
        if (headers.count(field))
        {

            return to_string(headers.find(field)->second);
        }
        return {};
    }
};
}




