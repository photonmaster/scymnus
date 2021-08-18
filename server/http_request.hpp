#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <memory>
#include <memory_resource>
#include <string_view>
#include <unordered_map>

#include "headers_container.hpp"
#include "server/ct_settings.hpp"
#include "server/memory_resource_manager.hpp"

namespace scymnus {

class http_request {
public:
    using allocator_type = std::pmr::polymorphic_allocator<char>;

    const headers_t &headers() const { return headers_; }

    const std::pmr::string &body() const { return body_; }

    std::optional<std::pmr::string>
    get_header_value(const std::pmr::string &field) const {
        if (headers_.count(field)) {
            return headers_.find(field)->second;
        }
        return {};
    }

private:
    friend class context;
    friend class connection;

    void reset() {
        body_.clear();
        headers_.clear();
    }

    explicit http_request(allocator_type allocator = {})
        : headers_{allocator}, body_{allocator} {}

    http_request(http_request &&) = delete;
    http_request &operator=(http_request &&r) noexcept = delete;
    http_request &operator=(const http_request &r) = delete;
    http_request(const http_request &r) = delete;

    headers_t headers_;
    std::pmr::string body_;
};
} // namespace scymnus
