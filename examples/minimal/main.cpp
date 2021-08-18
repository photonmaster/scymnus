#include "server/app.hpp"
#include "server/settings.hpp"


using namespace scymnus;


int main(){

    settings<core>()[CT_("idle_timeout")] = 0;
    settings<core>()[CT_("enable_swagger")] = false;


    auto& app = scymnus::app::instance();

    app.route([](context& ctx) -> response_for<http_method::GET, "/plaintext">
              {
                  return ctx.write_as<http_content_type::PLAIN_TEXT>(status<200>, "Hello, World!");
              });


    app.listen();
    app.run();
}

