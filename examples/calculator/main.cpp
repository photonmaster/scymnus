#include <exception>
#include <numeric>

#include "server/app.hpp"

using namespace scymnus;



int main(){
    auto& app = scymnus::app::instance();

    app.route([](path_param<"x", int> x,path_param<"y", int> y, context& ctx)
                  -> response_for<http_method::GET, "/sum/{x}/{y}">
              {
                  auto sum = x.get() + y.get();
                  return ctx.write(status<200>, sum);
              })
        .summary("Integer addition")
        .description("Returns the sum of two integer numbers. The two numbers are given as path parameters")
        .tag("calculator");


    //mean value of a set of double numbers

    using mean_request_model = model<
        field<"input", std::vector<double>, description("input numbers")>
        >;

    using mean_response_model = model<
        field<"mean", double, description("mean value of input numbers")>
        >;



    app.route([](body_param<"body",mean_request_model> body, context& ctx)
                  -> response_for<http_method::PUT, "/mean">
              {
//                  if (body.get().get<"input">().empty())
//                      return ctx.write(status<400>, "input must not be empty");

                  double sum = std::accumulate(body.get().get<"input">().begin(), body.get().get<"input">().end(), 0.0);
                  double mean = sum / body.get().get<"input">().size();

                  return ctx.write(status<200>, mean);
              })
        .summary("mean value")
        .description("mean value of a set of decimal numbers. The numbers are given in body of the request")
        .tag("calculator");



    app.route([](body_param<"body",std::vector<int>> body, context& ctx)
                  -> response_for<http_method::PUT, "/minimum">
              {
                  if (body.get().empty())
                      return ctx.write(status<400>, "input must not be empty");

                  auto it = std::min_element(body.get().begin(), body.get().end());

                  return ctx.write_as<http_content_type::JSON>(status<200>, *it);
              })
        .summary("Minimum value")
        .description("Minimum value of a set of integer numbers. The numbers are given in the body of the request")
        .tag("calculator");


//TODO: array as path parameter is not supported yet
//    app.route([](path_param<"numbers",std::vector<int>> input, context& ctx)
//                  -> response_for<http_method::GET, "/maximum/{numbers}">
//              {

//                  if (input.get().empty())
//                      return ctx.write(status<400>, "input must not be empty");


//                  auto it = std::max_element(input.get().begin(), input.get().end());


//                  return ctx.write(status<200>, *it);
//              })
//        .summary("Maximum value")
//        .description("Maximum value of a set of integer numbers. The numbers are given in the path of the request")
//        .tag("calculator");



    app.route([](query_param<"a", int> a,query_param<"b", int> b, context& ctx)
                  -> response_for<http_method::GET, "/midpoint">
              {
                  auto mid = std::midpoint(a.get() , b.get());
                  return ctx.write(status<200>, mid);
              })

        .summary("midpont value")
        .description("Returns the midpoint of two integer numbers. The two numbers are given as query parameters")
        .tag("calculator");


    app.route([](header_param<"value", int> value, context& ctx)
                  -> response_for<http_method::GET, "/abs">
              {
                  auto abs = std::abs(value.get());
                  return ctx.write(status<200>, abs);
              })
        .summary("Absolute value")
        .description("Returns the absolute value of an integer number. The number is given as a header parameter")
        .tag("calculator");




    /// swagger is accessible here: http://127.0.0.1:8080/api-doc

    app.listen();
    app.run();
}

