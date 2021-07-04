#include "server/app.hpp"



using namespace scymnus;




///When defining a model it is possible to define meta properties to the field of a model.
///It is also possible to define meta properties to the model itself
///E.g. we can refine the model of the "points" example and set minimum values to each of the coordinated
/// of the point. Also we define a name and a description for our model
/// using the properties type

using PointModel = model<
    field<"id", std::optional<int>>, //server sets this
    field<"x", int, constraints::min(0)>,
    field<"y", int,constraints::min(0)>,
    field<"z", int, constraints::min(0)>,
    properties<name("3D point"), description("A 3d point model")>
    >;


///the follwing map is our data source:

std::map<int, PointModel> points{};

/// The endpoints that will be exposed by our service,  will operate
///on the above map.
///(We are using a map bacause each point will have a unique id, that is the key of our map)




///lets now define our main function


int main(){
    /// let's add some
    api_manager::instance().name("Your name");
    api_manager::instance().email("your@email.com");
    api_manager::instance().host("127.0.0.1:9090");
    api_manager::instance().add_consume_type("aplication/json");
    api_manager::instance().add_produce_type("aplication/json");
    api_manager::instance().add_scheme("http");
    api_manager::instance().swagger_path("/swagger_resources/index.html");


    auto& app = scymnus::app::instance();


    ///for creating 3D point we will define first a POST endpoint.
    /// clients will be calling the follwing API
    /// POST api/v1.0/points
    /// for creating points.
    /// Whenever a point is created it will be added in our map of points


    app.route([](body_param<"body", PointModel> body, context& ctx) -> response_for<http_method::POST, "/points">
              {
                  auto p = body.get();
                  p.get<"id">() = points.size(); //code is not thread safe

                  points[points.size()] = p;
                  return response{status<200>, p, ctx};
              })
        .summary("Create a point")
        .description("Create a point resource. The id will be returned in the response")
        .tag("points");



    /// we are using the route method of our app instance to route our endpoint
    /// route method takes a callable as argument (a lambda).
    /// a context parameter is always needed a argument of our callable.
    /// a context is our handle to the http request and http response
    /// Clients that are calling the POST /points API must send a point model (in JSON format) in the body of their request.
    /// For this reason we are adding the parameter: body_param<"body", PointModel> body
    /// as the first argument of our callable
    ///
    /// the class body_param<> is used only to support swagger documentation for a web service
    ///
    /// the object that is returned by our callable is of type:
    ///
    /// response_for<http_method::POST, "/points">
    ///
    /// the template parameters of the response_for class type
    /// are used for specifying the http method of our endpoint and the endpoint itself
    ///
    ///
    /// Finally we are using the summary(), description() and tag() methods for better documenting our webservice:

    ///.summary("Create a point")
    ///.description("Create a point resource. The id will be returned in the response")
    ///.tag("points");



    ///Let's now add an endpoint for retrieving points given their id:

    app.route([](path_param<"id", int> id, context& ctx)
                  -> response_for<http_method::GET, "/points/{id}">
              {
                  if (points.count(id.get()))
                      return response{status<200>, points[id.get()], ctx};
                  else
                      return response{status<404>, std::string("Point not found"), ctx};

              })
        .summary("get a point")
        .description("retrieve a point by id")
        .tag("points");

    /// For our get endpoint we are using a path_param<> argument.
    /// the name of the path param: "id" must be present in the endpoint of the response_for return type:
    /// response_for<http_method::GET, "/points/{id}">


    ///We will add now an endpoint for deleting a point given point's id
    ///
    ///

    app.route([](path_param<"id", int> id, context& ctx)
                  -> response_for<http_method::DELETE, "/points/{id}">
              {
                  auto count = points.erase(id.get());

                  if (count)
                      return response{status<204>, ctx};
                  else
                      return response{status<404>, std::string("Point not found"), ctx};

              })
        .summary("remove a point")
        .description("remove a point by id")
        .tag("points");

    ///The last endpoint for this endpoint will return all the points in our map
    ///
    app.route([](context& ctx)
                  -> response_for<http_method::GET, "/points">
              {

                  std::vector<PointModel> data;

                  for(auto element : points)
                      data.push_back(element.second);

                  return response(status<200>,data, ctx);

              })
        .summary("get all points")
        .description("get a list of avaliable points")
        .tag("points");



    /// Finally we are starting the webservice
    /// swagger is accessible here: http://10.0.2.15:9090/api-doc


    app.listen("127.0.0.1", 9090);
    app.run();
}

