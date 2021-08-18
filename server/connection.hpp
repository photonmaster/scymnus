#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/intrusive_ptr.hpp>
#include <iostream>
#include <memory_resource>

#include "boost/asio/write.hpp"
#include "ct_settings.hpp"
#include "date_manager.hpp"
#include "external/decimal_from.hpp"
#include "external/http_parser/llhttp.h"
#include "server/memory_resource_manager.hpp"
#include "server/router.hpp"

namespace scymnus {

namespace detail {

template <typename F> struct final_action {
    final_action(F f) : clean_{f} {}
    ~final_action() {
        if (enabled_)
            clean_();
    }
    void disable() { enabled_ = false; };

private:
    F clean_;
    bool enabled_{true};
};

} // namespace detail

template <typename F> detail::final_action<F> finally(F f) {
    return detail::final_action<F>(f);
}

class connection {

public:
    connection(boost::asio::io_context &context)
        : socket_{context}, timer_{context} {

        llhttp_init(&parser_, HTTP_REQUEST, &settings_);
        parser_.data = this;
    }

    ~connection() {
        cancel_timer();
        std::cout << "connection destructed in thread:"
                  << std::this_thread::get_id() << std::endl;
    }

    // called on first use
    void idle_timeout_setup(uint32_t seconds) {
        if constexpr (idle_time_tracking == false)
            return;
        if (seconds) {
            last_activity_tp_ = std::chrono::steady_clock::now();
            idle_timeout_ = seconds;
            set_timer(seconds);
        }
    }

    boost::asio::ip::tcp::socket &socket() { return socket_; }

    void read() {

        socket_.async_read_some(
            boost::asio::buffer(buffer_), [self = boost::intrusive_ptr(this), this](
                                              const boost::system::error_code &ec,
                                              std::size_t bytes_transferred) {
                if (!ec) {
                    if constexpr (idle_time_tracking) {
                        last_activity_tp_ = std::chrono::steady_clock::now();
                    }

                    llhttp_errno_t err = process(buffer_.data(), bytes_transferred);

                    if (err == HPE_OK) {
                        if (response_.empty()) {
                            read();
                            return;
                        }
                        do_write(should_keep_alive());
                    }

                    else {

                        ctx_.write(status<400>);
                        complete_request(ctx_, false);
                    }
                } else {
                    std::cout << "error in async_read_some(): " << ec.message()
                              << std::endl;
                }
            });
    }

    llhttp_errno exec() {

        router_.exec(ctx_);
        ctx_.reset();
        return HPE_OK;
    }

    void complete_request(context &ctx, bool keep_alive = true) {
        response_.clear();
        ctx.reset();
        do_write(keep_alive);
    }

    // boost::asio::buffer(response_.data(), response_.size())

    template <typename... Args> auto const_sequence(Args &&...args) {
        return std::array<boost::asio::const_buffer, sizeof...(Args)>{
            boost::asio::buffer(args)...};
    }

    void do_write(bool keep_alive = true) {
        boost::asio::async_write(
            socket_, boost::asio::buffer(response_),
            [self = boost::intrusive_ptr(this),
             keep_alive](const boost::system::error_code &ec, size_t s) {
                if (ec) {
                    return;
                }
                self->reset();

                if (keep_alive) {
                    self->read();
                }
            });
    }

    // intrusive_ptr handling

    inline friend void intrusive_ptr_add_ref(connection *c) noexcept {
        ++c->ref_count_;
    }

    inline friend void intrusive_ptr_release(connection *c) noexcept {
        if (--c->ref_count_ == 0)
            delete c;
    }

private:
    //        std::array<char, 4096> buffer{};
    //        std::pmr::monotonic_buffer_resource mbr{&buffer, 4096,
    //        memory_resource_manager::instance().pool()};

    std::pmr::memory_resource *pool_{memory_resource_manager::instance().pool()};

    enum class parser_state : uint8_t {
        Init,
        Url,
        HeaderField,
        HeaderValue,
        HeadersComplete,
        Body,
        TrailerField,
        TrailerValue,
        MessageComplete
    };

    static int on_message_begin(llhttp_t *parser) { return HPE_OK; }

    static int on_message_complete(llhttp_t *llhttp) {
        auto *self = static_cast<connection *>(llhttp->data);

        // TODO: check if method is not supprted
        // route and call handler

        self->parser_state_ = parser_state::MessageComplete;

        return self->exec();
    }

    static int on_chunk_header(llhttp_t *parser) { return HPE_OK; }

    static int on_chunk_complete(llhttp_t *parser) { return HPE_OK; }

    static int on_url(llhttp_t *llhttp, const unsigned char *at, size_t length) {
        auto *self = static_cast<connection *>(llhttp->data);
        self->ctx_.raw_url_.insert(self->ctx_.raw_url_.end(), at, at + length);

        return HPE_OK;
    }

    static int on_body(llhttp_t *llhttp, const unsigned char *at, size_t length) {

        auto *self = static_cast<connection *>(llhttp->data);
        self->ctx_.req_.body_.insert(self->ctx_.req_.body_.end(), at, at + length);
        return HPE_OK;
    }

    static int on_status(llhttp_t *, const unsigned char *at, size_t length) {
        return HPE_OK;
    }

    static int on_header_field(llhttp_t *llhttp, const unsigned char *at,
                               size_t length) {
        auto *self = static_cast<connection *>(llhttp->data);

        if (self->parser_state_ == parser_state::HeaderValue) {
            // a field-value pair is ready

            self->ctx_.add_request_header(std::move(self->header_field_),
                                          std::move(self->header_value_));
            // clear after move
            self->header_field_.clear();
            self->header_value_.clear();
        }

        self->header_field_.insert(self->header_field_.end(), at, at + length);

        self->parser_state_ = parser_state::HeaderField;

        return HPE_OK;
    }

    static int on_header_value(llhttp_t *llhttp, const unsigned char *at,
                               size_t length) {
        auto *self = static_cast<connection *>(llhttp->data);

        self->header_value_.insert(self->header_value_.end(), at, at + length);
        self->parser_state_ = parser_state::HeaderValue;

        return HPE_OK;
    }

    static int on_headers_complete(llhttp_t *llhttp) {

        auto *self = static_cast<connection *>(llhttp->data);
        // add last header
        self->ctx_.add_request_header(std::move(self->header_field_),
                                      std::move(self->header_value_));
        // clear after move
        self->header_field_.clear();
        self->header_value_.clear();
        self->ctx_.method_ = static_cast<http_method>(llhttp->method);

        return HPE_OK;
    }
    static constexpr llhttp_settings_t settings_{
        nullptr, // on_message_begin,
        on_url,
        nullptr, // on_status,
        on_header_field, on_header_value,     on_headers_complete,
        on_body,         on_message_complete,
        nullptr, // on_chunk_header,
        nullptr, // on_chunk_complete
    };

    llhttp_errno_t process(const char *buffer, int length) {

        return llhttp_execute(&parser_, buffer, length);
    }

    int should_keep_alive() { return llhttp_should_keep_alive(&parser_); }

    uint16_t major_version() const { return parser_.http_major; }

    uint16_t minor_version() const { return parser_.http_minor; }

    std::string get_error(llhttp_errno_t err) const {
        return llhttp_errno_name(err);
    }

    llhttp_errno_t get_error_code() { return llhttp_get_errno(&parser_); }

    http_method method() { return static_cast<http_method>(parser_.method); }

    http_method to_string(http_method method) {
        return static_cast<http_method>(parser_.method);
    }

    void close() {
        if (is_closed_)
            return;

        cancel_timer();
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        is_closed_ = true;
    }

    void set_timer(uint32_t seconds) {
        if constexpr (idle_time_tracking == false)
            return;
        if (!idle_timeout_)
            return;

        timer_.expires_from_now(std::chrono::seconds(seconds));
        timer_.async_wait([this](boost::system::error_code const &ec) {
            if (ec) {
                return;
            }
            auto now = std::chrono::steady_clock::now();

            if (last_activity_tp_ + std::chrono::seconds(idle_timeout_) > now) {

                auto f = std::chrono::duration_cast<std::chrono::seconds>(
                    (last_activity_tp_ + std::chrono::seconds(idle_timeout_)) - now);
                set_timer(f.count());

            } else {
                close();
            }
        });
    }

    void cancel_timer() {
        if constexpr (idle_time_tracking == false)
            return;
        if (!idle_timeout_)
            return;
        boost::system::error_code ec;
        timer_.cancel(ec);
    }

    void reset() { response_.clear(); }

    parser_state parser_state_{parser_state::Init};
    llhttp_t parser_{};

    bool is_closed_{false};

    std::size_t ref_count_{0};

    boost::asio::ip::tcp::socket socket_;
    std::array<char, read_buffer_size> buffer_;

    uint16_t header_size_{0};
    uint32_t idle_timeout_{0};
    boost::asio::steady_timer timer_;
    std::chrono::time_point<std::chrono::steady_clock> last_activity_tp_;

    std::pmr::string response_{pool_};
    std::pmr::string content_length_{pool_};
    std::pmr::string header_field_{pool_};
    std::pmr::string header_value_{pool_};

    context ctx_{&response_, pool_};
    router router_{};
    date_manager &date_manager_{date_manager::instance()};
};
} // namespace scymnus
