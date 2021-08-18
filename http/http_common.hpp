#pragma once

#include "core/ct_map.hpp"
#include <string_view>

namespace scymnus {

enum class http_method : uint16_t {
    DELETE = 0,
    GET = 1,
    HEAD = 2,
    POST = 3,
    PUT = 4,
    CONNECT = 5,
    OPTIONS = 6,
    TRACE = 7,
    COPY = 8,
    LOCK = 9,
    MKCOL = 10,
    MOVE = 11,
    PROPFIND = 12,
    PROPPATCH = 13,
    SEARCH = 14,
    UNLOCK = 15,
    BIND = 16,
    REBIND = 17,
    UNBIND = 18,
    ACL = 19,
    REPORT = 20,
    MKACTIVITY = 21,
    CHECKOUT = 22,
    MERGE = 23,
    MSEARCH = 24,
    NOTIFY = 25,
    SUBSCRIBE = 26,
    UNSUBSCRIBE = 27,
    PATCH = 28,
    PURGE = 29,
    MKCALENDAR = 30,
    LINK = 31,
    UNLINK = 32,
    SOURCE = 33,
    PRI = 34,
    DESCRIBE = 35,
    ANNOUNCE = 36,
    SETUP = 37,
    PLAY = 38,
    PAUSE = 39,
    TEARDOWN = 40,
    GET_PARAMETER = 41,
    SET_PARAMETER = 42,
    REDIRECT = 43,
    RECORD = 44,
    FLUSH = 45,
    ENUM_MEMBERS_COUNT // Always at the end
};

//   ctx.res.add_header("Content-Type", "text/plain; charset=UTF-8");
enum class http_content_type : uint8_t { NONE, JSON, PLAIN_TEXT };

constexpr std::string_view describe(http_content_type ct) {
    switch (ct) {

    case http_content_type::JSON:
        return "application/json";
    case http_content_type::PLAIN_TEXT:
        return "text/plain; charset=utf-8";
    case http_content_type::NONE:
    default:
        return {};
    }
}

constexpr std::string_view to_string_view(http_content_type ct) {
    switch (ct) {

    case http_content_type::JSON:
        return "Content-Type:application/json\r\n";
    case http_content_type::PLAIN_TEXT:
        return "Content-Type:text/plain; charset=UTF-8\r\n";
    case http_content_type::NONE:
    default:
        return {};
    }
};

[[nodiscard]] inline std::string to_string(http_method name) {
    // using enum http_method;

    switch (name) {
    case http_method::DELETE:
        return "delete";
    case http_method::GET:
        return "get";
    case http_method::HEAD:
        return "head";
    case http_method::POST:
        return "post";
    case http_method::PUT:
        return "put";
        [[unlikely]] case http_method::CONNECT : return "connect";
        [[unlikely]] case http_method::OPTIONS : return "options";
        [[unlikely]] case http_method::TRACE : return "trace";
        [[unlikely]] case http_method::COPY : return "copy";
        [[unlikely]] case http_method::LOCK : return "lock";
        [[unlikely]] case http_method::MKCOL : return "mkcol";
        [[unlikely]] case http_method::MOVE : return "move";
        [[unlikely]] case http_method::PROPFIND : return "propfind";
        [[unlikely]] case http_method::PROPPATCH : return "proppatch";
        [[unlikely]] case http_method::SEARCH : return "search";
        [[unlikely]] case http_method::UNLOCK : return "unlock";
        [[unlikely]] case http_method::BIND : return "bind";
        [[unlikely]] case http_method::REBIND : return "rebind";
        [[unlikely]] case http_method::UNBIND : return "unbind";
        [[unlikely]] case http_method::ACL : return "acl";
        [[unlikely]] case http_method::REPORT : return "report";
        [[unlikely]] case http_method::MKACTIVITY : return "mkactivity";
        [[unlikely]] case http_method::CHECKOUT : return "checkout";
        [[unlikely]] case http_method::MERGE : return "merge";
        [[unlikely]] case http_method::MSEARCH : return "msearch";
        [[unlikely]] case http_method::NOTIFY : return "notify";
        [[unlikely]] case http_method::SUBSCRIBE : return "subscribe";
        [[unlikely]] case http_method::UNSUBSCRIBE : return "unsubscribe";
        [[unlikely]] case http_method::PATCH : return "patch";
        [[unlikely]] case http_method::PURGE : return "purge";
        [[unlikely]] case http_method::MKCALENDAR : return "mkcalendar";
        [[unlikely]] case http_method::LINK : return "link";
        [[unlikely]] case http_method::UNLINK : return "unlink";
        [[unlikely]] case http_method::SOURCE : return "source";
        [[unlikely]] case http_method::PRI : return "pri";
        [[unlikely]] case http_method::DESCRIBE : return "describe";
        [[unlikely]] case http_method::ANNOUNCE : return "announce";
        [[unlikely]] case http_method::SETUP : return "setup";
        [[unlikely]] case http_method::PLAY : return "play";
        [[unlikely]] case http_method::PAUSE : return "pause";
        [[unlikely]] case http_method::TEARDOWN : return "teardown";
        [[unlikely]] case http_method::GET_PARAMETER : return "get_parameter";
        [[unlikely]] case http_method::SET_PARAMETER : return "set_parameter";
        [[unlikely]] case http_method::REDIRECT : return "redirect";
        [[unlikely]] case http_method::RECORD : return "record";
        [[unlikely]] case http_method::FLUSH : return "flush";
        [[unlikely]] default : return "unsupported";
    }
}

// http status reasons

constexpr const char *http_reason_phrase(int c) {
    switch (c) {
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 102:
        return "Processing";
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-Authoritative Information";
    case 204:
        return "No Content";
    case 205:
        return "Reset Content";
    case 206:
        return "Partial Content";
    case 207:
        return "Multi-Status";
    case 208:
        return "Already Reported";
    case 226:
        return "IM Used";
    case 300:
        return "Multiple Choices";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 305:
        return "Use Proxy";
    case 307:
        return "Temporary Redirect";
    case 308:
        return "Permanent Redirect"; // rfc7538
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 406:
        return "Not Acceptable";
    case 407:
        return "Proxy Authentication Required";
    case 408:
        return "Request Timeout";
    case 409:
        return "Conflict";
    case 410:
        return "Gone";
    case 411:
        return "Length Required";
    case 412:
        return "Precondition Failed";
    case 413:
        return "Request Entity Too Large";
    case 414:
        return "Request-URI Too Long";
    case 415:
        return "Unsupported Media Type";
    case 416:
        return "Requested Range Not Satisfiable";
    case 417:
        return "Expectation Failed";
    case 422:
        return "Unprocessable Entity"; // rfc4918
    case 423:
        return "Locked"; // rfc4918
    case 424:
        return "Failed Dependency"; // rfc4918
    case 426:
        return "Upgrade Required"; // rfc7231
    case 428:
        return "Precondition Required";
    case 429:
        return "Too Many Requests";
    case 431:
        return "Request Header Fields Too Large";
    case 444:
        return "No Response";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    case 505:
        return "HTTP Version Not Supported";
    case 506:
        return "Variant Also Negotiates";
    case 507:
        return "Insufficient Storage";
    case 508:
        return "Loop Detected";
    case 509:
        return "Bandwidth Limit Exceeded";
    case 510:
        return "Not Extended";
    case 511:
        return "Network Authentication Required";
    case 598:
        return "Network read timeout error";
    case 599:
        return "Network connect timeout error";
    default:
        return "";
    }
}

// static constexpr std::string_view header_seperator = ": ";
// static std::string crlf = "\r\n";

static constexpr auto status_codes = scymnus::ct_map<int, std::string_view, 61>{
    {{{100, "HTTP/1.1 100 Continue\r\n"},
      {101, "HTTP/1.1 101 Switching Protocols\r\n"},
      {102, "HTTP/1.1 102 Processing\r\n"},
      {200, "HTTP/1.1 200 OK\r\n"},
      {201, "HTTP/1.1 201 Created\r\n"},
      {202, "HTTP/1.1 202 Accepted\r\n"},
      {203, "HTTP/1.1 203 Non-Authoritative Information\r\n"},
      {204, "HTTP/1.1 204 No Content\r\n"},
      {205, "HTTP/1.1 205 Reset Content\r\n"},
      {206, "HTTP/1.1 206 Partial Content\r\n"},
      {207, "HTTP/1.1 207 Multi-Status\r\n"},
      {208, "HTTP/1.1 208 Already Reported\r\n"},
      {226, "HTTP/1.1 226 IM Used\r\n"},
      {300, "HTTP/1.1 300 Multiple Choices\r\n"},
      {301, "HTTP/1.1 301 Moved Permanently\r\n"},
      {302, "HTTP/1.1 302 Found\r\n"},
      {303, "HTTP/1.1 303 See Other\r\n"},
      {304, "HTTP/1.1 304 Not Modified\r\n"},
      {305, "HTTP/1.1 305 Use Proxy\r\n"},
      {307, "HTTP/1.1 307 Temporary Redirect\r\n"},
      {308, "HTTP/1.1 308 Permanent Redirect\r\n"}, // rfc7538
      {400, "HTTP/1.1 400 Bad Request\r\n"},
      {401, "HTTP/1.1 401 Unauthorized\r\n"},
      {402, "HTTP/1.1 402 Payment Required\r\n"},
      {403, "HTTP/1.1 403 Forbidden\r\n"},
      {404, "HTTP/1.1 404 Not Found\r\n"},
      {405, "HTTP/1.1 405 Method Not Allowed\r\n"},
      {406, "HTTP/1.1 406 Not Acceptable\r\n"},
      {407, "HTTP/1.1 407 Proxy Authentication Required\r\n"},
      {408, "HTTP/1.1 408 Request Timeout\r\n"},
      {409, "HTTP/1.1 409 Conflict\r\n"},
      {410, "HTTP/1.1 410 Gone\r\n"},
      {411, "HTTP/1.1 411 Length Required\r\n"},
      {412, "HTTP/1.1 412 Precondition Failed\r\n"},
      {413, "HTTP/1.1 413 Request Entity Too Large\r\n"},
      {414, "HTTP/1.1 414 Request-URI Too Long\r\n"},
      {415, "HTTP/1.1 415 Unsupported Media Type\r\n"},
      {416, "HTTP/1.1 416 Requested Range Not Satisfiable\r\n"},
      {417, "HTTP/1.1 417 Expectation Failed\r\n"},
      {422, "HTTP/1.1 422 Unprocessable Entity\r\n"}, // rfc4918
      {423, "HTTP/1.1 423 Locked\r\n"},               // rfc4918
      {424, "HTTP/1.1 424 Failed Dependency\r\n"},    // rfc4918
      {426, "HTTP/1.1 426 Upgrade Required\r\n"},     // rfc7231
      {428, "HTTP/1.1 428 Precondition Required\r\n"},
      {429, "HTTP/1.1 429 Too Many Requests\r\n"},
      {431, "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
      {444, "HTTP/1.1 444 No Response\r\n"},
      {500, "HTTP/1.1 500 Internal Server Error\r\n"},
      {501, "HTTP/1.1 501 Not Implemented\r\n"},
      {502, "HTTP/1.1 502 Bad Gateway\r\n"},
      {503, "HTTP/1.1 503 Service Unavailable\r\n"},
      {504, "HTTP/1.1 504 Gateway Timeout\r\n"},
      {505, "HTTP/1.1 505 HTTP Version Not Supported\r\n"},
      {506, "HTTP/1.1 506 Variant Also Negotiates\r\n"},
      {507, "HTTP/1.1 507 Insufficient Storage\r\n"},
      {508, "HTTP/1.1 508 Loop Detected\r\n"},
      {509, "HTTP/1.1 509 Bandwidth Limit Exceeded\r\n"},
      {510, "HTTP/1.1 510 Not Extended\r\n"},
      {511, "HTTP/1.1 511 Network Authentication Required\r\n"},
      {598, "HTTP/1.1 598 Network read timeout error\r\n"},
      {599, "HTTP/1.1 599 Network connect timeout error\r\n"}}}};

} // namespace scymnus
