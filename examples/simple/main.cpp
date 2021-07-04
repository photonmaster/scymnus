#include "server/app.hpp"
#include "core/named_tuples_utils.hpp"

using namespace scymnus;
using namespace nlohmann;


using ColorModel = model<
    field<"r", int, description("red")>,
    field<"g", int, description("green")>,
    field<"b", int, description("blue")>//,
    //properties<name("ColorModel"), description("Color coordinates")>
    >;



using PointModel = model<
    field<"id", std::optional<int>, description("Server side generated value")>, //server sets this
    field<"x", int, constraints::min(0), description("X coordinate of 3D point")>,
    field<"y", int, constraints::min(0), description("Y coordinate of 3D point")>,
    field<"z", std::optional<int>, init<[](){return 1;}>{},  description("Z coordinate of 3D point")>,
    field<"c", std::optional<ColorModel>, init<[](){return ColorModel{127,127,127};}>{},  description("Color details of point")>,
    properties<name("PointModel"), description("A 2d point model used in example")>
    >;

using error_model = model<
    field<"code", int>,
    field<"message", std::string>,
    properties<name("ErrorModel"), description("Contains error details")>
    >;


std::map<int, PointModel> points{};


using app_settings = model<
field<"x", int>
>;



int main(){


   auto f = init_settings<app_settings>();

    api_manager::instance().name("Your name");
    api_manager::instance().email("your@email.com");
    api_manager::instance().host("127.0.0.1:9090");
    api_manager::instance().add_consume_type("aplication/json");
    api_manager::instance().add_produce_type("aplication/json");
    api_manager::instance().add_scheme("http");
    api_manager::instance().swagger_path("/swagger_resources/index.html");


    auto& app = scymnus::app::instance();

    app.set_excpetion_handler([](context& ctx){
        try{
            throw;
        }
        catch(std::exception& exp){

            error_model error {1423, exp.what()};

            json v = error;
            ctx.res.add_header("Content-Type", "application/json");
            ctx.res.status_code = 500;
            ctx.res.body = v.dump();
        }
    });


    //    app.route(scymnus::get_swagger_description_controller{});
    //    app.route(scymnus::api_doc_controller{});
    //    app.route(scymnus::swagger_controller_files{});


    app.route([](body_param<"body", PointModel> body,
                 context& ctx) -> response_for<http_method::POST, "/points">
              {
                  auto p = body.get();
                  p.get<"id">() = points.size(); //code is not thread safe
                  points[points.size()] = p;
                  return response{status<204>, ctx};

              })
        .summary("create a point")
        .description("create a point")
        .tag("points");


    //TODO: check path params aainst endpoint
    app.route([](path_param<"id", int> id, context& ctx)
                  -> response_for<http_method::GET, "/points/{id}">
              {
                  if (points.count(id.get()))
                      return response{status<200>, points[id.get()], ctx};
                  else
                      return response{status<404>, error_model{1, "point id not found"}, ctx};

              }).summary("get a point")
        .description("retrieve a point by id")
        .tag("points");


    app.route([](context& ctx)
                  -> response_for<http_method::GET, "/points">
              {
                  std::vector<PointModel> data;

                  for(auto element : points)
                      data.push_back(element.second);

                  return response(status<200>,data, ctx);

              }).summary("get points")
        .description("retrieve a list of points")
        .tag("points");


    app.listen("127.0.0.1", 9090);
    app.run();
}
