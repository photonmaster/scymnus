#pragma once

#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//#include <span> gcc 10

namespace scymnus {

class uri_exception : public std::exception {
public:
    uri_exception(std::string msg) : msg_(std::move(msg)) {}

    ~uri_exception() noexcept {}

    const char *what() const noexcept { return msg_.c_str(); }

private:
    std::string msg_;
};

namespace details {
// ref: cpprestsdk
// https://github.com/microsoft/cpprestsdk/blob/53dcf8b34f8879ed3bd8f9ed6d489b730b156335/Release/include/cpprest/asyncrt_utils.h

/// <summary>
/// Our own implementation of alpha numeric instead of std::isalnum to avoid
/// taking global lock for performance reasons.
/// </summary>
///

// ref:
// https://stackoverflow.com/questions/313970/how-to-convert-stdstring-to-lower-case
char tolower_ascii(char in) {
    if (in <= 'Z' && in >= 'A')
        return in - ('Z' - 'z');
    return in;
}

inline bool constexpr is_alnum(
    const unsigned char uch) noexcept { // test if uch is an alnum character
    // special casing char to avoid branches
    // clang-format off
     constexpr bool is_alnum_table[std::numeric_limits<unsigned char>::max() + 1] = {
        /*        X0 X1 X2 X3 X4 X5 X6 X7 X8 X9 XA XB XC XD XE XF */
        /* 0X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 1X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 2X */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 3X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0-9 */
        /* 4X */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* A-Z */
        /* 5X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
        /* 6X */   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* a-z */
        /* 7X */   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
        /* non-ASCII values initialized to 0 */
    };
    // clang-format on
    return (is_alnum_table[uch]);
}
/// <summary>
/// Unreserved characters are those that are allowed in a URI but do not have a
/// reserved purpose. They include:
/// - A-Z
/// - a-z
/// - 0-9
/// - '-' (hyphen)
/// - '.' (period)
/// - '_' (underscore)
/// - '~' (tilde)
/// </summary>
inline bool is_unreserved(int c) {
    return is_alnum((char)c) || c == '-' || c == '.' || c == '_' || c == '~';
}

/// <summary>
/// General delimiters serve as the delimiters between different uri components.
/// General delimiters include:
/// - All of these :/?#[]@
/// </summary>
inline bool is_gen_delim(int c) {
    return c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' ||
           c == '@';
}

/// <summary>
/// Subdelimiters are those characters that may have a defined meaning within
/// component of a uri for a particular scheme. They do not serve as delimiters
/// in any case between uri segments. sub_delimiters include:
/// - All of these !$&'()*+,;=
/// </summary>
inline bool is_sub_delim(int c) {
    switch (c) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=':
        return true;
    default:
        return false;
    }
}

/// <summary>
/// Reserved characters includes the general delimiters and sub delimiters. Some
/// characters are neither reserved nor unreserved, and must be percent-encoded.
/// </summary>
inline bool is_reserved(int c) { return is_gen_delim(c) || is_sub_delim(c); }

/// <summary>
/// Legal characters in the scheme portion include:
/// - Any alphanumeric character
/// - '+' (plus)
/// - '-' (hyphen)
/// - '.' (period)
///
/// Note that the scheme must BEGIN with an alpha character.
/// </summary>
inline bool is_scheme_character(int c) {
    return is_alnum((char)c) || c == '+' || c == '-' || c == '.';
}

/// <summary>
/// Legal characters in the user information portion include:
/// - Any unreserved character
/// - The percent character ('%'), and thus any percent-endcoded octet
/// - The sub-delimiters
/// - ':' (colon)
/// </summary>
inline bool is_user_info_character(int c) {
    return is_unreserved(c) || is_sub_delim(c) || c == '%' || c == ':';
}

/// <summary>
/// Legal characters in the authority portion include:
/// - Any unreserved character
/// - The percent character ('%'), and thus any percent-endcoded octet
/// - The sub-delimiters
/// - ':' (colon)
/// - IPv6 requires '[]' allowed for it to be valid URI and passed to underlying
/// platform for IPv6 support
/// </summary>
inline bool is_authority_character(int c) {
    return is_unreserved(c) || is_sub_delim(c) || c == '%' || c == '@' ||
           c == ':' || c == '[' || c == ']';
}

/// <summary>
/// Legal characters in the path portion include:
/// - Any unreserved character
/// - The percent character ('%'), and thus any percent-endcoded octet
/// - The sub-delimiters
/// - ':' (colon)
/// - '@' (at sign)
/// </summary>
inline bool is_path_character(int c) {
    return is_unreserved(c) || is_sub_delim(c) || c == '%' || c == '/' ||
           c == ':' || c == '@';
}

/// <summary>
/// Legal characters in the query portion include:
/// - Any path character
/// - '?' (question mark)
/// </summary>
inline bool is_query_character(int c) {
    return is_path_character(c) || c == '?';
}

/// <summary>
/// Legal characters in the fragment portion include:
/// - Any path character
/// - '?' (question mark)
/// </summary>
inline bool is_fragment_character(int c) {
    // this is intentional, they have the same set of legal characters
    return is_query_character(c);
}
} // namespace details

static constexpr char single_slash[] = "/";

template <class parameter_type = std::string_view> class uri {

public:
    uri(std::string_view raw);

    class components {
    public:
        enum component { user_info, host, path, query, fragment, full_uri };
    };

    static std::string
    encode_uri(const std::string &raw,
               typename uri::components::component = components::full_uri);

    static std::string encode_data_string(const std::string &data);

    static std::string decode(const std::string &encoded);

    static std::vector<std::string> split_path(const std::string &path);

    static std::map<std::string, std::string>
    split_query(const std::string &query);

    static bool validate(const std::string &uri_string);

    // uri(const utility::char_t* uri_string);

    uri() : raw_{single_slash, 1} {}

    uri(const uri &) = default;

    uri &operator=(const uri &) = default;

    uri(uri &&other) noexcept
        : raw_(std::move(other.raw_)), parameters_(std::move(other.parameters_)) {

    }

    uri &operator=(uri &&other) noexcept {
        if (this != &other) {
            raw_ = std::move(other.raw_);
            parameters_ = std::move(other.parameters_);
        }
        return *this;
    }

    std::string_view scheme() const { return parameters_.scheme; }

    std::string_view user_info() const { return parameters_.user_info; }

    std::string_view host() const { return parameters_.host; }
    int port() const { return parameters_.port; }

    std::string_view path() const { return parameters_.path; }

    std::string_view query() const { return parameters_.query; }
    std::string_view fragment() const { return parameters_.fragment; }

    uri authority() const;

    //    uri resource() const;

    bool is_empty() const { return raw_.empty() || raw_ == "/"; }
    bool is_host_loopback() const {
        return !is_empty() &&
               ((host() == "localhost") ||
                (host().size() > 4 && host().substr(0, 4) == "127."));
    }

    bool is_host_wildcard() const {
        return !is_empty() && (host() == "*" || host() == "+");
    }

    bool is_host_portable() const {
        return !(is_empty() || is_host_loopback() || is_host_wildcard());
    }

    bool is_port_default() const { return !is_empty() && this->port() == 0; }

    bool is_authority() const {
        return !is_empty() && is_path_empty() && query().empty() &&
               fragment().empty();
    }

    bool has_same_authority(const uri &other) const {
        return !is_empty() && (authority() == other.authority());
    }

    bool is_path_empty() const { return path().empty() || path() == "/"; }

    // utility::string_t to_string() const { return m_uri; }

    // utility::string_t resolve_uri(const utility::string_t& relativeUri) const;

    bool operator==(const uri &other) const {
        // Each individual URI component must be decoded before performing
        // comparison. TFS # 375865

        if (this->is_empty() && other.is_empty()) {
            return true;
        } else if (this->is_empty() || other.is_empty()) {
            return false;
        } else if (this->scheme() != other.scheme()) {
            // scheme is canonicalized to lowercase
            return false;
        } else if (uri::decode(this->user_info()) !=
                   uri::decode(other.user_info())) {
            return false;
        } else if (uri::decode(this->host()) != uri::decode(other.host())) {
            // host is canonicalized to lowercase
            return false;
        } else if (this->port() != other.port()) {
            return false;
        } else if (uri::decode(this->path()) != uri::decode(other.path())) {
            return false;
        } else if (uri::decode(this->query()) != uri::decode(other.query())) {
            return false;
        } else if (uri::decode(this->fragment()) != uri::decode(other.fragment())) {
            return false;
        }

        return true;
    }

    bool operator<(const uri &other) const { return raw_ < other.raw_; }

    bool operator!=(const uri &other) const { return !(operator==(other)); }

    std::string to_string() {

        std::string ret;

        if (!parameters_.scheme.empty()) {
            ret.append(parameters_.scheme);
            ret.push_back(':');
        }

        if (!parameters_.host.empty()) {
            ret.append("//");

            if (!parameters_.user_info.empty()) {
                ret.append(parameters_.user_info).append({"@"});
            }

            ret.append(parameters_.host);

            if (parameters_.port > 0) {
                ret.append(":").append(std::to_string(parameters_.port));
            }
        }

        if (!parameters_.path.empty()) {
            // only add the leading slash when the host is present
            if (!parameters_.host.empty() && parameters_.path.front() != '/') {
                ret.push_back('/');
            }

            ret.append(parameters_.path);
        }

        if (!parameters_.query.empty()) {
            ret.push_back('?');
            ret.append(parameters_.query);
        }

        if (!parameters_.fragment.empty()) {
            ret.push_back('#');
            ret.append(parameters_.fragment);
        }

        return ret;
    }

private:
    struct parameters {
        parameter_type scheme;
        parameter_type userinfo;
        parameter_type host;
        std::uint16_t port;
        parameter_type path;
        parameter_type query;
        parameter_type fragment;

        /// A Boolean value indicating whether this URL can be used as a
        /// base URL
        bool cannot_be_a_base_url = false;

        /// A Boolean value indicating whether a non-fatal validation
        /// error occurred during parsing
        bool validation_error = false;

        /// Default constructor
        parameters() = default;
    };

    friend class uri_builder;
    friend class uri_parser;

    parameters parameters_;
    std::string_view raw_;
};

class uri_parser {
public:
    const char *scheme_begin = nullptr;
    const char *scheme_end = nullptr;
    const char *uinfo_begin = nullptr;
    const char *uinfo_end = nullptr;
    const char *host_begin = nullptr;
    const char *host_end = nullptr;
    int port = 0;
    const char *path_begin = nullptr;
    const char *path_end = nullptr;
    const char *query_begin = nullptr;
    const char *query_end = nullptr;
    const char *fragment_begin = nullptr;
    const char *fragment_end = nullptr;

    /// <summary>
    /// Parses the uri, setting the given pointers to locations inside the given
    /// buffer. 'encoded' is expected to point to an encoded zero-terminated
    /// string containing a uri
    /// </summary>
    bool parse_from(const char *encoded) {
        const char *p = encoded;

        // IMPORTANT -- A uri may either be an absolute uri, or an
        // relative-reference Absolute: 'http://host.com' Relative-Reference:
        // '//:host.com', '/path1/path2?query', './path1:path2' A Relative-Reference
        // can be disambiguated by parsing for a ':' before the first slash

        bool is_relative_reference = true;

        const char *p2 = p;

        for (; *p2 != '/' && *p2 != '\0'; p2++) {
            if (*p2 == ':') {
                // found a colon, the first portion is a scheme
                is_relative_reference = false;
                break;
            }
        }

        if (!is_relative_reference) {
            // the first character of a scheme must be a letter
            if (!isalpha(*p)) {
                return false;
            }

            // start parsing the scheme, it's always delimited by a colon (must be
            // present)
            scheme_begin = p++;
            for (; *p != ':'; p++) {
                if (!details::is_scheme_character(*p)) {
                    return false;
                }
            }
            scheme_end = p;

            // skip over the colon
            p++;
        }

        // if we see two slashes next, then we're going to parse the authority
        // portion later on we'll break up the authority into the port and host
        const char *authority_begin = nullptr;
        const char *authority_end = nullptr;
        if (*p == '/' && p[1] == '/') {
            // skip over the slashes
            p += 2;
            authority_begin = p;

            // the authority is delimited by a slash (resource), question-mark (query)
            // or octothorpe (fragment) or by EOS. The authority could be empty
            // ('file:///C:\file_name.txt')
            for (; *p != '/' && *p != '?' && *p != '#' && *p != '\0'; p++) {
                // We're NOT currently supporting IPvFuture or username/password in
                // authority IPv6 as the host (i.e. http://[:::::::]) is allowed as
                // valid URI and passed to subsystem for support.
                if (!details::is_authority_character(*p)) {
                    return false;
                }
            }
            authority_end = p;

            // now lets see if we have a port specified -- by working back from the
            // end
            if (authority_begin != authority_end) {
                // the port is made up of all digits
                const char *port_begin = authority_end - 1;
                for (; isdigit(*port_begin) && port_begin != authority_begin;
                     port_begin--) {
                }

                if (*port_begin == ':') {
                    // has a port
                    host_begin = authority_begin;
                    host_end = port_begin;

                    // skip the colon
                    port_begin++;
                    port = std::stoi(std::string(port_begin, authority_end));
                }

                else {
                    // no port
                    host_begin = authority_begin;
                    host_end = authority_end;
                }

                // look for a user_info component
                const char *u_end = host_begin;
                for (; details::is_user_info_character(*u_end) && u_end != host_end;
                     u_end++) {
                }

                if (*u_end == '@') {
                    host_begin = u_end + 1;
                    uinfo_begin = authority_begin;
                    uinfo_end = u_end;
                }
            }
        }

        // if we see a path character or a slash, then the
        // if we see a slash, or any other legal path character, parse the path next
        if (*p == '/' || details::is_path_character(*p)) {
            path_begin = p;

            // the path is delimited by a question-mark (query) or octothorpe
            // (fragment) or by EOS
            for (; *p != '?' && *p != '#' && *p != '\0'; p++) {
                if (!details::is_path_character(*p)) {
                    return false;
                }
            }
            path_end = p;
        }

        // if we see a ?, then the query is next
        if (*p == '?') {
            // skip over the question mark
            p++;
            query_begin = p;

            // the query is delimited by a '#' (fragment) or EOS
            for (; *p != '#' && *p != '\0'; p++) {
                if (!details::is_query_character(*p)) {
                    return false;
                }
            }
            query_end = p;
        }

        // if we see a #, then the fragment is next
        if (*p == '#') {
            // skip over the hash mark
            p++;
            fragment_begin = p;

            // the fragment is delimited by EOS
            for (; *p != '\0'; p++) {
                if (!details::is_fragment_character(*p)) {
                    return false;
                }
            }
            fragment_end = p;
        }

        return true;
    }

    void write_to(uri<>::parameters &components) {

        if (scheme_begin) {
            // std::transform(scheme_begin, scheme_end, scheme_begin,
            // details::tolower_ascii);
            components.scheme =
                std::string_view(scheme_begin, scheme_end - scheme_begin);
        }

        if (uinfo_begin) {
            components.userinfo =
                std::string_view(uinfo_begin, uinfo_end - uinfo_begin);

            // components.m_user_info.assign(uinfo_begin, uinfo_end);
        }

        if (host_begin) {

            // std::transform(host_begin, host_end, host_begin,
            // details::tolower_ascii);
            components.host = std::string_view(host_begin, host_end - host_begin);
        }

        components.port = port;

        if (path_begin) {
            components.path = std::string_view(path_begin, path_end - path_begin);
        }

        else {
            // default path to begin with a slash for easy comparison
            components.path = "/";
        }

        if (query_begin) {
            components.query = std::string_view(query_begin, query_end - query_begin);
        }

        if (fragment_begin) {
            components.fragment =
                std::string_view(fragment_begin, fragment_end - fragment_begin);
        }
    }
};

template <class parameter_type> uri<parameter_type>::uri(std::string_view raw) {
    uri_parser parser;

    if (!parser.parse_from(raw.data()))

        throw uri_exception("provided uri is invalid");

    parser.write_to(parameters_);
}

namespace details {

// Encodes all characters not in given set determined by given function.
template <class F> std::string encode(const std::string &raw, F should_encode) {

    const char *const hex = "0123456789ABCDEF";

    std::string encoded;
    for (auto iter = raw.begin(); iter != raw.end(); ++iter) {
        // for utf8 encoded string, char ASCII can be greater than 127.
        int ch = static_cast<unsigned char>(*iter);
        // ch should be same under both utf8 and utf16.
        if (should_encode(ch)) {
            encoded.push_back('%');
            encoded.push_back(hex[(ch >> 4) & 0xF]);
            encoded.push_back(hex[ch & 0xF]);
        } else {
            // ASCII don't need to be encoded, which should be same on both utf8 and
            // utf16.
            encoded.push_back((char)ch);
        }
    }
    return encoded;
}

std::string encode_query(const std::string &raw) {
    return encode(raw, [](int ch) -> bool {
        switch (ch) {
            // Encode '&', ';', and '=' since they are used
            // as delimiters in query component.
        case '&':
        case ';':
        case '=':
        case '%':
        case '+':
            return true;
        default:
            return !details::is_query_character(ch);
        }
    });
}

/// </summary>
/// Encodes a string by converting all characters except for RFC 3986 unreserved
/// characters to their hexadecimal representation.
/// </summary>
std::string encode_data_string(const std::string &data) {

    return encode(data,
                  [](int ch) -> bool { return !details::is_unreserved(ch); });
}

} // namespace details

/// <summary>
/// Helper function to convert a hex character digit to a decimal character
/// value. Throws an exception if not a valid hex digit.
/// </summary>
static int hex_char_digit_to_decimal_char(int hex) {
    int decimal;
    if (hex >= '0' && hex <= '9') {
        decimal = hex - '0';
    } else if (hex >= 'A' && hex <= 'F') {
        decimal = 10 + (hex - 'A');
    } else if (hex >= 'a' && hex <= 'f') {
        decimal = 10 + (hex - 'a');
    } else {
        throw uri_exception("Invalid hexadecimal digit");
    }
    return decimal;
}

template <class String> static String decode_template(const String &encoded) {
    String raw;
    for (auto iter = encoded.begin(); iter != encoded.end(); ++iter) {
        if (*iter == '%') {
            if (++iter == encoded.end()) {
                throw uri_exception(
                    "Invalid URI string, two hexadecimal digits must follow '%'");
            }

            int decimal_value =
                hex_char_digit_to_decimal_char(static_cast<int>(*iter)) << 4;
            if (++iter == encoded.end()) {
                throw uri_exception(
                    "Invalid URI string, two hexadecimal digits must follow '%'");
            }
            decimal_value += hex_char_digit_to_decimal_char(static_cast<int>(*iter));

            raw.push_back(static_cast<char>(decimal_value));
        } else if (*iter > 127 || *iter < 0) {
            throw uri_exception("Invalid encoded URI string, must be entirely ascii");
        } else {
            // encoded string has to be ASCII.
            raw.push_back(static_cast<char>(*iter));
        }
    }
    return raw;
}

template <class T> std::string uri<T>::decode(const std::string &encoded) {
    return decode_template<std::string>(encoded);
}
} // namespace scymnus
