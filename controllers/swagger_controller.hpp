#pragma once

#include <string>
#include <filesystem>
#include <fstream>

#include "server/server.hpp"
#include "server/router.hpp"
#include "core/named_tuple.hpp"
#include "server/api_manager.hpp"
#include "mime/mime.hpp"



//TODO: secure access to files


namespace scymnus {

namespace fs = std::filesystem;



class get_swagger_description_controller
{
public:
    response_for<http_method::GET, "/swagger"> operator() (context& ctx) const
    {
        return response(status<200>, json::parse(api_manager::instance().describe()), ctx);
    }

};




class api_doc_controller
{
public:

    response_for<http_method::GET, "/api-doc"> operator() (context& ctx){

        ctx.res.status_code = 303;
        ctx.res.add_header("Location", "/swagger_ui" + api_manager::instance().swagger_path());
        return {};
    }
};


class swagger_controller_files
{
public:
    response_for<http_method::GET, "/swagger_ui/:*">
    operator() (context& ctx) {
        std::filesystem::path p{ctx.raw_url.substr(11)};

        if (!p.has_filename()){
            ctx.res.status_code = 404;
            return {};
        }

         std::ifstream stream(p.c_str(), std::ios::binary);

        std::string body{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};

        auto ct = mime_map::content_type(p.extension().string());
        if (ct){
            ctx.res.add_header("Content-Type",*ct);
        }

        ctx.res.status_code = 200;
        ctx.res.body = std::move(body);
        return {};

    }

};

} //namespace scymnus

