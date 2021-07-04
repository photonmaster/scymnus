#include "server/app.hpp"
#include "server/settings.hpp"


using namespace scymnus;


///In this example a minimalistic example of a web service is given


using app_settings = model<
field<"a",std::string>
>;



int main(){


    init_settings<app_settings>();


      api_manager::instance().name("Your name");
      api_manager::instance().email("your@email.com");
      api_manager::instance().title("Calculator service");
      api_manager::instance().version(0,1);
      api_manager::instance().host("127.0.0.1:9090");
      api_manager::instance().add_consume_type("aplication/json");
      api_manager::instance().add_produce_type("aplication/json");
      api_manager::instance().add_scheme("http");
      api_manager::instance().swagger_path("/swagger_resources/index.html");


    /// scymnus::app is a singleton. we are taking a reference to each instance, named app
    /// that we will be using in the rest of the code
    auto& app = scymnus::app::instance();



    app.route([](context& ctx)
                  -> response_for<http_method::GET, "/settings">
           {

           return response{status<200>, settings<core>(), ctx};
              }).summary("retrieve settings")
            .description("retrieve settings");


    app.route([](context& ctx)
                  -> response_for<http_method::GET, "/plaintext">
           {
        return response{status<200>, std::string("hello world!"), ctx};
              }).summary("simple get")
            .description("simple get");



    app.route([](path_param<"x", int> x,path_param<"y", int> y, context& ctx)
                  -> response_for<http_method::GET, "/sum/{x}/{y}">
           {
               auto sum = x.get() + y.get();
               return response{status<200>, sum, ctx};
              })
        .summary("Integer addition")
        .description("Returns the sum of two integer numbers")
        .tag("calculator");

    /// Finally we are starting the webservice
    /// swagger is accessible here: http://10.0.2.15:9090/api-doc


    app.listen();
    app.run();
}

