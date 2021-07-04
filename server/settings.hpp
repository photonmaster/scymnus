#pragma once

#include <thread>

#include "core/named_tuple.hpp"
#include "properties/constraints.hpp"
#include "properties/defaulted.hpp"

namespace scymnus {


enum settings_type
{
    core,
    application
};


template<settings_type = application>
auto settings();




using core_settings_model = model<
field<"idle_timeout", std::optional<uint32_t>, init<[](){return 60;}>{}, description("Afte idle_timeout seconds idle connections will be closed")>,
field<"max_header_size", std::optional<uint16_t>, init<[](){return 8 * 1024;}>{}, description("Maiximum accepted size of headers in a request")>,
field<"max_url_size", std::optional<uint16_t>, init<[](){return 2 * 1024;}>{}, description("Maiximum accepted size of url in a request")>,
field<"max_body_size", std::optional<uint16_t>, init<[](){return 8 * 1024;}>{}, description("Maiximum accepted size of request body")>,
field<"port", std::optional<uint16_t>, init<[](){return 8080;}>{}, description("Server's listening port")>,
field<"ip", std::optional<std::string>, init<[](){return "0.0.0.0";}>{}, description("Server's ip")>,
field<"workers", std::optional<uint16_t>, init<[](){return std::thread::hardware_concurrency();}>{}, description("Number of working threads")>
>;






class core_settings {
public:

    static inline core_settings_model settings_{};
};


template <class T>
class init_settings {
public:

    friend auto settings() {
       return settings_;
    }

    static inline T settings_{};
};




template<>
auto settings<core>()
{
    return core_settings::settings_;
}

}
