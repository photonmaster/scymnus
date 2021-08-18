#pragma once

#include <boost/asio.hpp>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>

#include "core/exception.hpp"

namespace scymnus {

class io_info {
public:
    io_info() = default;
    io_info(io_info const &) = delete;
    io_info &operator=(io_info const &) = delete;

    void set(boost::asio::io_context *io) { io_ = io; }

    boost::asio::io_context &get() {
        if (io_)
            return *io_;
        throw std::runtime_error("not known io_context for this thread");
    }

private:
    static inline thread_local boost::asio::io_context *io_{nullptr};
};

} // namespace scymnus
