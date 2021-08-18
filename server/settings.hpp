#pragma once

#include <thread>

#include "core/named_tuple.hpp"
#include "properties/constraints.hpp"
#include "properties/defaulted.hpp"

namespace scymnus {

enum settings_type { core, application };

template <settings_type = application> auto &settings();

using doc_licence_model = model<
    field<"name", std::string, description("The license name used for the API")>,
    field<"url", std::optional<std::string>, description("A URL to the license used for the API. MUST be in the format of a URL")>
    >;

using doc_contact_model = model<
    field<"name", std::optional<std::string>, description("The identifying name of the contact person/organization")>,
    field<"url", std::optional<std::string>, description("The URL pointing to the contact information. MUST be in the format of a URL")>,
    field<"email", std::optional<std::string>, description("The email address of the contact person/organization. MUST be in the format of an email address")>
    >;

using doc_model = model<
    field<"title", std::optional<std::string>, init<[]() { return "Scymnus Based Webservice"; }>{}, description("The title of the API")>,
    field<"description", std::optional<std::string>, description("A short description of the API")>,
    field<"termsOfService", std::optional<std::string>, description("A URL to the Terms of Service for the API. MUST be in the format of a URL")>,
    field<"contact", std::optional<doc_contact_model>, description("The contact information for the exposed API.")>,
    field<"licence", std::optional<doc_licence_model>, description("The license information for the exposed API")>,
    field<"version", std::optional<std::string>, init<[]() { return ""; }>{},  description("The version of the OpenAPI document (which is distinct from the OpenAPI Specification version or the API implementation version).")>,
    field<"host", std::optional<std::string>, description("the domain name or IP address (IPv4) of the host that serves the API. It may include the port number")>,
    field<"swagger_directory", std::optional<std::string>,init<[]() { return "swagger_resources"; }>{}, description("relative path to the executable, where the swagger distribution is")>

    >;

using core_settings_model = model<
    field<"idle_timeout", std::optional<uint32_t>, init<[]() { return 60; }>{}, description("After idle_timeout seconds idle connections will be closed")>,
    field<"max_header_size", std::optional<uint16_t>, init<[]() { return 8 * 1024; }>{}, description("Maiximum accepted size of headers in a request")>,
    field<"max_url_size", std::optional<uint16_t>, init<[]() { return 2 * 1024; }>{}, description("Maiximum accepted size of url in a request")>,
    field<"max_body_size", std::optional<uint16_t>, init<[]() { return 8 * 1024; }>{}, description("Maiximum accepted size of request body")>,
    field<"port", std::optional<uint16_t>, init<[]() { return 8080; }>{}, description("Server's listening port")>,
    field<"ip", std::optional<std::string>, init<[]() { return "0.0.0.0"; }>{}, description("Server's ip")>,
    field<"workers", std::optional<uint16_t>, init<[]() { return std::thread::hardware_concurrency(); }>{}, description("Number of working threads")>,
    field<"enable_swagger", std::optional<bool>, init<[]() { return true; }>{}, description("enable swagger. Default value is false")>,
    field<"swagger", std::optional<doc_model>, description("swagger details")>
    >;

class core_settings {
public:
    static inline core_settings_model settings_{};
};

template <class T> class init_settings {
public:
    friend auto settings() { return settings_; }

    static inline T settings_{};
};

template <> auto &settings<core>() { return core_settings::settings_; }

} // namespace scymnus
