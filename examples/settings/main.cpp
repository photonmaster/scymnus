#include "server/app.hpp"


using namespace scymnus;

int main(){
    auto& app = scymnus::app::instance();
    app.route([](context& ctx)->response_for<http_method::GET, "/settings"> {

           return ctx.write(status<200>,settings<core>());
       }).summary("settings")
        .description("Retrieve application settings")
        .tag("settings");


    /// swagger is accessible here: http://127.0.0.1:8080/api-doc

    app.listen();
    app.run();
}

