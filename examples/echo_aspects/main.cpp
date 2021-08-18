#include <exception>

#include "server/app.hpp"

using namespace scymnus;



/// Let's define a before aspect that will log the request:

struct log_request_aspect : aspect_base<"log_request", hook_type::before> {
    sink<"log_request"> operator()(context& ctx)
    {
        std::cout << "Request:" << '\n';

        std::cout << ctx.raw_url() << '\n';
        for(auto& [key, value] :ctx.request().headers()){
            std::cout << key << ": " << value << '\n';
        }
        std::cout << ctx.request().body() << '\n';
        std::cout.flush();

        return {};
    }
};

/// Let's define an after aspect that will log the response:

struct log_response_aspect : aspect_base<"log_response", hook_type::after> {
    sink<"log_response"> operator()(const context& ctx)
    {
        std::cout << "Response:" << '\n';

         std::cout << ctx.response().status_code() << '\n';
        for(auto& [key, value] :ctx.response().headers()){
            std::cout << key << ": " << value << '\n';
        }
        std::cout << '\n' << ctx.response().body() << '\n';
        std::cout.flush();

        return {};
    }
};




int main(){

    auto& app = scymnus::app::instance();
    app.route([](body_param<"body", json> body,context& ctx)->response_for<http_method::POST, "/echo"> {

        return ctx.write_as<http_content_type::JSON>(status<200>,ctx.request_body());
    }, log_request_aspect{},
       log_response_aspect{}
    ).summary("echo back the json payload in the request")
     .tag("echo");


    /// swagger is accessible here: http://127.0.0.1:8080/api-doc

    app.listen();
    app.run();
}

