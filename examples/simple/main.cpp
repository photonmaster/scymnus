#include "server/app.hpp"
#include "core/named_tuples_utils.hpp"

using namespace scymnus;
using namespace nlohmann;


using ColorModel = model<
    field<"r", int, description("red")>,
    field<"g", int, description("green")>,
    field<"b", int, description("blue")>,
    properties<name("ColorModel"), description("Color coordinates")>
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



template<class T>
using paginated_model = model<
    field<"contents",T>,
    field<"totalPages",uint32_t>,
    field<"totalItems",uint32_t>,
    field<"page",uint32_t>
    >;


int main(){
    auto& app = scymnus::app::instance();


    app.route([](body_param<"body", PointModel> body,
                 context& ctx) -> response_for<http_method::POST, "/points">
              {
                  auto p = body.get();
                  p.get<"id">() = points.size(); //code is not thread safe
                  points[points.size()] = p;
                  return ctx.write(status<204>);

              })
        .summary("create a point")
        .description("create a point")
        .tag("points");


    //TODO: check path params against endpoint
    app.route([](path_param<"id", int> id, context& ctx)
                  -> response_for<http_method::GET, "/points/{id}">
              {
                  if (points.count(id.get()))
                      return ctx.write_as<http_content_type::JSON>(status<200>, points[id.get()]);
                  else
                      return ctx.write(status<404>, error_model{1, "point id not found"});

              }).summary("get a point")
        .description("retrieve a point by id")
        .tag("points");


    app.route_internal([](path_param<"id", int> id, context& ctx)
                           -> response_for<http_method::GET, "/test/{id}">
                       {
                           if (points.count(id.get()))
                               return ctx.write(status<200>, points[id.get()]);
                           else
                               return ctx.write(status<404>, error_model{1, "point id not found"});

                       });






    app.route([](query_param<"page", int16_t> page,query_param<"pageSize", uint16_t> size, context& ctx)
                  -> response_for<http_method::GET, "/points">
              {

                  auto ps = size.get();
                  auto p = page.get();

                  if (ps == 0){
                      return ctx.write(status<400>, error_model{2, "pageSize must be greater than 0"});
                  }

                  std::size_t total_elements = points.size();
                  auto total_pages = total_elements % ps != 0?total_elements / ps + 1:total_elements / ps;


                  if (p > total_pages){
                      return ctx.write(status<400>,
                                       error_model{3, "requested page is greater than the total number of pages"});
                  }

                  std::vector<PointModel> data;
                  auto start = std::next(points.begin(), p * ps);
                  auto end = (p + 1) * ps > total_elements?points.end():std::next(points.begin(),(p + 1) * ps);


                  for (auto it = start; it != end; it++) {
                      data.push_back((*it).second);
                  }

                  auto model = paginated_model<std::vector<PointModel>>{data, total_pages,total_elements,p};
                  return ctx.write(status<200>,model);

              }).summary("get points")
        .description("retrieve a list of points")
        .tag("points");


    app.listen();
    app.run();
}
