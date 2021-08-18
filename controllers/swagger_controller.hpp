#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include "core/named_tuple.hpp"
#include "mime/mime.hpp"
#include "server/api_manager.hpp"
#include "server/memory_resource_manager.hpp"
#include "server/router.hpp"
#include "server/server.hpp"

// TODO: secure access to files

namespace scymnus {

namespace fs = std::filesystem;

class get_swagger_description_controller {
public:
    response_for<http_method::GET, "/swagger"> operator()(context &ctx) const {
        return ctx.write_as<http_content_type::JSON>(
            status<200>, api_manager::instance().describe());
    }
};

class api_doc_controller {
public:
    response_for<http_method::GET, "/api-doc"> operator()(context &ctx) {
        std::pmr::string value{memory_resource_manager::instance().pool()};
        value.append("/swagger_ui/");
        value.append(api_manager::instance().swagger_path());
        value.append("/");
        value.append("index.html");
        
        ctx.add_response_header("Location", std::move(value));
        ctx.write(status<303>);
        
        return {};
    }
};

class swagger_controller_files {
public:
    response_for<http_method::GET, "/swagger_ui/:*"> operator()(context &ctx) {
        std::filesystem::path p{ctx.raw_url().substr(12)};
        
        ctx.write_file(p);
        return {};
    }
};

} // namespace scymnus
