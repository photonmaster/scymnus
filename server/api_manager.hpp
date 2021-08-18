#pragma once

#include <set>
#include <typeindex>
#include <unordered_map>

#include "doc_traits.hpp"
#include "external/json.hpp"
#include "settings.hpp"
#include "version.hpp"

namespace scymnus {
using json = nlohmann::json;

class api_manager {

    std::string host_;
    std::string base_path_{"/"};
    std::vector<std::string> schemes_;
    std::vector<std::string> consumes_;
    std::vector<std::string> produces_;

    // TODO: tags can have a description
    std::set<std::string> tags_;

public:
    static api_manager &instance();

    void add_aspect_response(const std::string &aspect_name, int status,
                             const json &description) {

        json v;
        v["status"] = std::to_string(status);
        v["details"] = description;

        aspect_responses_[aspect_name] += v;
    }

    auto aspect_responses() const { return aspect_responses_; }

    std::string title() const {
        return settings<core>()[CT_("swagger")][CT_("title")];
    }

    void title(std::string title) {
        settings<core>()[CT_("swagger")][CT_("title")] = std::move(title);
    }

    std::string description() const {
        return settings<core>()[CT_("swagger")][CT_("description")];
    }

    void description(std::string description) {
        settings<core>()[CT_("swagger")][CT_("description")] =
            std::move(description);
    }

    std::string version() const {
        return settings<core>()[CT_("swagger")][CT_("version")];
    }

    void version(unsigned short major, unsigned short minor) {
        std::string ver = std::to_string(major) + '.' + std::to_string(minor);
        settings<core>()[CT_("swagger")][CT_("version")] = std::move(ver);
    }

    void version(unsigned short major, unsigned short minor,
                 unsigned short patch) {
        std::string ver = std::to_string(major) + '.' + std::to_string(minor) +
                          '.' + std::to_string(patch);
        settings<core>()[CT_("swagger")][CT_("version")] = std::move(ver);
    }

    // term of service
    std::string tos() const {
        return settings<core>()[CT_("swagger")][CT_("termsOfService")];
    }

    void tos(std::string tos) {
        settings<core>()[CT_("swagger")][CT_("termsOfService")] = std::move(tos);
    }

    std::string contact_name() const {
        return settings<core>()[CT_("swagger")][CT_("contact")][CT_("name")];
    }

    void contact_name(std::string name) {
        settings<core>()[CT_("swagger")][CT_("contact")][CT_("name")] =
            std::move(name);
    }

    std::string contact_url() const {
        return settings<core>()[CT_("swagger")][CT_("contact")][CT_("url")];
    }

    void contact_url(std::string url) {
        settings<core>()[CT_("swagger")][CT_("contact")][CT_("url")] =
            std::move(url);
    }

    std::string contact_email() const {
        return settings<core>()[CT_("swagger")][CT_("contact")][CT_("email")];
    }

    void contact_email(std::string email) {
        settings<core>()[CT_("swagger")][CT_("contact")][CT_("email")] =
            std::move(email);
    }

    std::string license_name() const {
        return settings<core>()[CT_("swagger")][CT_("licence")][CT_("name")];
    }

    void license_name(std::string license_name) {
        settings<core>()[CT_("swagger")][CT_("licence")][CT_("name")] =
            std::move(license_name);
    }

    std::string license_url() const {
        return settings<core>()[CT_("swagger")][CT_("licence")][CT_("url")];
    }

    void license_url(std::string license_url) {
        settings<core>()[CT_("swagger")][CT_("licence")][CT_("url")] =
            std::move(license_url);
    }

    // TODO: remove this, by automating tag handling based on groups
    void add_tag(std::string name) { tags_.insert(std::move(name)); }

    std::string host() const { return host_; }

    void host(std::string host) { host_ = std::move(host); }

    std::string base_path() const { return base_path_; }

    void base_path(std::string base_path) { base_path_ = std::move(base_path); }

    void add_consume_type(std::string consume) {
        consumes_.push_back(std::move(consume));
    }

    void add_produce_type(std::string produce) {
        produces_.push_back(std::move(produce));
    }

    void add_scheme(std::string scheme) { schemes_.push_back(std::move(scheme)); }

    // path to swagger-resources
    std::string swagger_path() const {
        return  settings<core>()[CT_("swagger")][CT_("swagger_directory")];
    }

    void swagger_path(std::string path) {
        settings<core>()[CT_("swagger")][CT_("swagger_directory")] = std::move(path);
    }

    inline std::string describe() { return swagger_description_; }

    void prepare_description() {
        swagger_["swagger"] = "2.0";
        if (host_.empty()) {
            std::string ip = settings<core>()[CT_("ip")] == "0.0.0.0"
                                 ? "127.0.0.1"
                                 : settings<core>()[CT_("ip")];
            host_ = ip + ":" + std::to_string(settings<core>()[CT_("port")]);
        }
        swagger_["host"] = host();
        swagger_["info"]["title"] = title();

        //set scymnus version if no version is avaliable
        if (version().empty()){
            version(SCYMNUS_PROJECT_VER_MAJOR,SCYMNUS_PROJECT_VER_MINOR,SCYMNUS_PROJECT_VER_PATCH);
        }
        swagger_["info"]["version"] = version();

        swagger_["info"]["description"] = description();
        if (!license_name().empty()) {
            swagger_["info"]["license"]["name"] = license_name();
            if (!license_url().empty()) {
                swagger_["info"]["license"]["url"] = license_url();
            }
        }

        if (!contact_name().empty())
            swagger_["info"]["contact"]["name"] = contact_name();
        if (!contact_email().empty())
            swagger_["info"]["contact"]["email"] = contact_email();
        if (!contact_url().empty())
            swagger_["info"]["contact"]["url"] = contact_url();

        swagger_["basePath"] = base_path();

        if (consumes_.empty())
            consumes_.push_back("application/json");

        swagger_["consumes"] = consumes_;

        if (produces_.empty())
            produces_.push_back("application/json");

        swagger_["produces"] = produces_;

        if (schemes_.empty())
            schemes_.push_back("http");

        swagger_["schemes"] = schemes_;

        for (const auto &tag : tags_) {
            json v;
            v["name"] = tag;
            swagger_["tags"] += v;
        }

        if (doc_types::get_definitions().size()) {
            swagger_["definitions"] = doc_types::get_definitions();
        }

        swagger_description_ = swagger_.dump(3);
    }

    json aspect_responses_;
    json endpoint_responses_;
    json endpoint_produce_types_;
    json swagger_;

private:
    friend class app;
    std::string swagger_description_;

    api_manager(const api_manager &) = delete;
    api_manager(api_manager &&) = delete;
    api_manager() = default;
};

inline api_manager &api_manager::instance() {
    static api_manager instance;
    return instance;
}

} // namespace scymnus
