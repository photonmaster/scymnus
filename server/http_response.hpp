#pragma once
#include <string>
#include <memory_resource>

#include "headers_container.hpp"


namespace scymnus
{

class context;

struct http_response
{

    std::vector<std::pair<std::string, std::string>> headers;
    std::optional<uint16_t>  status_code;
    std::string body;

    void add_header(std::string key, std::string value)
    {
        headers.emplace_back(std::move(key), std::move(value));
    }


    void reset(){
        body.clear();
        headers.clear();
        status_code ={};
    }


    http_response() = default;

    explicit http_response(uint16_t sc) : status_code(sc) {}
    http_response(http_response&&) = default;
    http_response& operator= (http_response&& r) noexcept = default;
    http_response& operator=(const http_response& r)  = delete;
    http_response(const http_response& r) = delete;

};

} //scymnus
