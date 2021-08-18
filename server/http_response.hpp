#pragma once
#include <string>
#include <memory_resource>

#include "headers_container.hpp"
#include "memory_resource_manager.hpp"
#include "http/http_common.hpp"
namespace scymnus
{


class  http_response
{
public:
    using allocator_type = std::pmr::polymorphic_allocator<char>;


    const headers_t& headers() const {
        return headers_;
    }

    std::string_view body() const {
        return body_;
    }

    std::string_view status_code() const {
        return status_codes.at(status_code_.value_or(200));
    }


private:
    friend class context;


    void reset() {
        body_ = {};
        headers_.clear();
        status_code_ ={};
    }


    explicit http_response(allocator_type allocator = {}) :
        headers_{allocator} {
    }


    explicit http_response(uint16_t sc, allocator_type allocator = {}) :
        status_code_(sc),
        headers_{allocator} {
    }



    http_response(http_response&&) = delete;
    http_response& operator= (http_response&& r) noexcept = delete;
    http_response& operator=(const http_response& r)  = delete;
    http_response(const http_response& r) = delete;

    headers_t headers_;
    std::optional<uint16_t>  status_code_;
    std::string_view body_;

};

} //scymnus
