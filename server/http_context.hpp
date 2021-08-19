#pragma once
#include <filesystem>
#include <fstream>
#include <charconv>

#include "core/named_tuple.hpp"
#include "core/traits.hpp"
#include "date_manager.hpp"
#include "external/json.hpp"
#include "http/http_common.hpp"
#include "http/query_parser.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "mime/mime.hpp"
#include "server/memory_resource_manager.hpp"
//#include "url/url.hpp"
#include "external/decimal_from.hpp"

namespace scymnus {

template <int Status> using status_t = std::integral_constant<int, Status>;
template <int Status> constexpr status_t<Status> status;

template <http_content_type ContentType>
using content_t = std::integral_constant<http_content_type, ContentType>;
template <http_content_type ContentType>
constexpr content_t<ContentType> content_type;

struct no_content {};

template <int Status, class T,
         http_content_type ContentType = http_content_type::NONE>
struct meta_info {
    static constexpr int status = Status;
    static constexpr http_content_type content_type = ContentType;
};

struct context {

    using allocator_type = std::pmr::polymorphic_allocator<char>;

    std::pmr::string *output_buffer_;
    explicit context(std::pmr::string *output_buffer,
                     allocator_type allocator = {})
        : output_buffer_{output_buffer}, raw_url_{allocator}, req_{allocator},
        res_{allocator} {}

    const std::pmr::string &raw_url() const { return raw_url_; }

    http_method method() const { return method_; }

    // request related methods
    const http_request &request() const { return req_; }

    const std::pmr::string &request_body() const { return req_.body_; }

    void add_request_header(const std::pmr::string &field,
                            const std::pmr::string &value) {
        req_.headers_.emplace(field, value);
    }

    void add_request_header(std::pmr::string &&field, std::pmr::string &&value) {
        req_.headers_.emplace(std::move(field), std::move(value));
    }

    // response related methods

    const http_response &response() const { return res_; }

    std::string_view response_body() const { return res_.body_; }

    void add_response_header(const std::pmr::string &field,
                             const std::pmr::string &value) {
        res_.headers_.emplace(field, value);
    }

    void add_response_header(std::pmr::string &&field, std::pmr::string &&value) {
        res_.headers_.emplace(std::move(field), std::move(value));
    }

    template <http_content_type ContentType, int Status, int N, class... H>
    auto write_as(status_t<Status> st, const char (&body)[N], H &&...headers) {
        start_buffer_position_ = output_buffer_->size();

        using json = nlohmann::json;
        static_assert(ContentType != http_content_type::NONE,
                      "a content type, different from NONE, must be selected");

        content_type = ContentType;
        res_.status_code_ = st;

        output_buffer_->append(status_codes.at(Status));
        output_buffer_->append(to_string_view(ContentType));

        for (auto &v : res_.headers_) {
            output_buffer_->append(v.first);
            output_buffer_->append(":");
            output_buffer_->append(v.second);
            output_buffer_->append("\r\n");
        }

        output_buffer_->append("Content-Length:");
        char buff[24];

        if constexpr (ContentType == http_content_type::JSON) {
            json v = body;
            auto payload = v.dump();
            auto size = payload.size();
            output_buffer_->append(decimal_from(size, buff));
           //output_buffer_->append(fmt::format_int(size).c_str());
            output_buffer_->append("\r\nServer:scymnus\r\n");
            date_manager::instance().append_http_time(*output_buffer_);
            output_buffer_->append(payload);
            res_.body_ = {output_buffer_->end() - size, output_buffer_->end()};
        } else {
            output_buffer_->append(decimal_from(N-1, buff));
            //output_buffer_->append(fmt::format_int(N - 1).c_str());
            output_buffer_->append("\r\nServer:scymnus\r\n");
            date_manager::instance().append_http_time(*output_buffer_);
            output_buffer_->append(body);
            res_.body_ = {output_buffer_->end() - N - 1, output_buffer_->end()};
        }
        return meta_info<Status, const char *, ContentType>{};
    }

    template <http_content_type ContentType, int Status, class T, class... H>
    auto write_as(status_t<Status> st, T &&body, H &&...headers) {
        static_assert(ContentType != http_content_type::NONE,
                      "a content type, different from NONE, must be selected");
        start_buffer_position_ = output_buffer_->size();
        using json = nlohmann::json;
        content_type = ContentType;

        if constexpr (is_string_like_v<T>) {
            res_.status_code_ = st;

            if constexpr (ContentType == http_content_type::JSON) {
                if (json::accept(body)) {
                    write_response_headers(body.size());
                    output_buffer_->append(body);
                    res_.body_ = {output_buffer_->end() - body.size(),
                                  output_buffer_->end()};
                } else {
                    auto payload = json(std::forward<T>(body)).dump();
                    write_response_headers(payload.size());
                    output_buffer_->append(payload);
                    res_.body_ = {output_buffer_->end() - payload.size(),
                                  output_buffer_->end()};
                }
                return meta_info<sizeof(T)?Status:0, T, http_content_type::JSON>{};

            } else { // plain text
                write_response_headers(body.size());
                output_buffer_->append(body);
                res_.body_ = {output_buffer_->end() - body.size(),
                              output_buffer_->end()};
                return meta_info<sizeof(T)?Status:0, T, http_content_type::PLAIN_TEXT>{};
            }
        }

        else if constexpr (std::is_constructible_v<json, std::remove_cv_t<T>>) {
            static_assert(sizeof(T) && ContentType == http_content_type::JSON,
                          "content type must be JSON");

            auto payload = json(std::forward<T>(body)).dump();
            res_.status_code_ = st;
            write_response_headers(payload.size());
            output_buffer_->append(payload);
            res_.body_ = {output_buffer_->end() - payload.size(),
                          output_buffer_->end()};

            return meta_info<sizeof(T)?Status:0, T, ContentType>{};
        }

        else {
            static_assert(!sizeof(T), "Type not supported");
        }
    };

    template <int Status, int N, class... H>
    auto write(status_t<Status> st, const char (&body)[N], H &&...headers) {
        start_buffer_position_ = output_buffer_->size();
        content_type = http_content_type::PLAIN_TEXT;
        res_.status_code_ = st;

        output_buffer_->append(status_codes.at(Status));
        output_buffer_->append(to_string_view(http_content_type::PLAIN_TEXT));

        for (auto &v : res_.headers_) {
            output_buffer_->append(v.first);
            output_buffer_->append(":");
            output_buffer_->append(v.second);
            output_buffer_->append("\r\n");
        }

        output_buffer_->append("Content-Length:");
        char buff[24];
        output_buffer_->append(decimal_from(N-1, buff));

        //output_buffer_->append(fmt::format_int(N - 1).c_str());
        output_buffer_->append("\r\nServer:scymnus\r\n");
        date_manager::instance().append_http_time(*output_buffer_);
        output_buffer_->append(body);
        res_.body_ = {output_buffer_->end() - N - 1, output_buffer_->end()};
        return meta_info<Status, const char *, http_content_type::PLAIN_TEXT>{};
    }

    template <int Status, class T, class... H>
    auto write(status_t<Status> st, T &&body, H &&...headers) {
        using json = nlohmann::json;
        start_buffer_position_ = output_buffer_->size();
        if constexpr (std::is_same_v<std::remove_cv_t<T>, std::string>) {
            content_type = http_content_type::PLAIN_TEXT;
            res_.status_code_ = st;
            write_response_headers(body.size());
            output_buffer_->append(body);
            res_.body_ = {output_buffer_->end() - body.size(), output_buffer_->end()};
            return meta_info<Status, T, http_content_type::PLAIN_TEXT>{};
        } else if constexpr (std::is_constructible_v<json, std::remove_cv_t<T>>) {
            json v = std::forward<T>(body);
            auto payload = v.dump();

            content_type = http_content_type::JSON;
            res_.status_code_ = st;

            write_response_headers(payload.size());
            output_buffer_->append(payload);
            res_.body_ = {output_buffer_->end() - payload.size(),
                          output_buffer_->end()};

            return meta_info<Status, T, http_content_type::JSON>{};
        }

        else {
            static_assert(!sizeof(T), "Type is not supported");
        }
    };

    // for No Content

    template <int Status, class... H>
    [[deprecated("HTTP status code should be 204 when there is no content")]] auto
    write(status_t<Status> st, H &&...headers) requires(Status == 200) {
        write_no_content<Status>();
        return meta_info<Status, no_content, http_content_type::NONE>{};
    }

    template <int Status, class... H>
    auto write(status_t<Status> st, H &&...headers) requires(Status != 200) {
        write_no_content<Status>();
        return meta_info<Status, no_content, http_content_type::NONE>{};
    }

    void write_file(const std::filesystem::path &p) {
        namespace fs = std::filesystem;
        if (!p.has_filename() || !fs::exists(p)) {
            write(status<404>);
            return;
        }

        std::ifstream stream(p.c_str(), std::ios::binary);
        std::string body(std::istreambuf_iterator<char>{stream}, {});

        auto ct = mime_map::content_type(p.extension().string());
        if (ct) {
            std::pmr::string value{memory_resource_manager::instance().pool()};
            value.append(*ct);
            add_response_header("Content-Type", std::move(value));
        }

        res_.status_code_ = 200;
        write_response_headers(body.size());
        output_buffer_->append(body);
        res_.body_ = {output_buffer_->end() - body.size(), output_buffer_->end()};
    }

    void write_response_headers(std::size_t size) {

        if (res_.status_code_.value() == 200)
            output_buffer_->append("HTTP/1.1 200 OK\r\n");

        else {
            auto status = status_codes.at(res_.status_code_.value_or(500), 500);
            output_buffer_->append(status);
        }

        if (content_type != http_content_type::NONE)
            output_buffer_->append(to_string_view(content_type));

        for (auto &v : res_.headers_) {
            output_buffer_->append(v.first);
            output_buffer_->append(":");
            output_buffer_->append(v.second);
            output_buffer_->append("\r\n");
        }

        output_buffer_->append("Content-Length:");

        char buff[24];
        output_buffer_->append(decimal_from(size, buff));

        //output_buffer_->append(fmt::format_int(size).c_str());
        output_buffer_->append("\r\nServer:scymnus\r\n");
        date_manager::instance().append_http_time(*output_buffer_);
    }

    // in case of an exception clear is called to clean up
    // the output_buffer

    // erase might throw an out of range exception if start_buffer_position is
    // beyond end of string
    void clear() noexcept {
        if (is_response_written()){
            res_.reset();
            output_buffer_->erase(start_buffer_position_, std::string::npos);
        }
    }

    void reset() {
        query_.reset();
        raw_url_.clear();

        req_.reset();
        res_.reset();
        start_buffer_position_ = std::numeric_limits<size_t>::max();
    }

    bool is_response_written() {
        return start_buffer_position_ != std::numeric_limits<size_t>::max();
    }

    // write support
    query_string get_query_string() {
        if (query_)
            return query_.value();
        query_ = query_string();

        auto query_start = raw_url_.find('?');
        if (query_start != std::string::npos && query_start != raw_url_.size()) {
            query_.value().parse(
                {raw_url_.data() + query_start + 1, raw_url_.size()});
        }
        return query_.value();
    }

    http_content_type content_type{http_content_type::NONE};

private:
    friend class connection;
    template <int Status> void write_no_content() {
        start_buffer_position_ = output_buffer_->size();
        res_.status_code_ = Status;
        output_buffer_->append(status_codes.at(Status));

        for (auto &v : res_.headers_) {
            output_buffer_->append(v.first);
            output_buffer_->append(":");
            output_buffer_->append(v.second);
            output_buffer_->append("\r\n");
        }

        output_buffer_->append("Content-Length:0");
        output_buffer_->append("\r\nServer:scymnus\r\n");
        date_manager::instance().append_http_time(*output_buffer_);
    }

    http_request req_;
    http_response res_;
    std::optional<query_string> query_;

    http_method method_;
    std::pmr::string raw_url_;

    std::size_t start_buffer_position_{std::numeric_limits<size_t>::max()};
};

} // namespace scymnus
