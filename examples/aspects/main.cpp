#include <exception>

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


///Defining aspects.
/// An aspect is a mechanism for separating cross cutting concerns from the rest of the code.
/// E.g. checking if a token in a request is valid, is a functionality that is shared between
/// all of the controllers of our web service.
/// The code  that validates tokens can be seperated in an aspect and the the aspect to be enabled
/// for the controllers needed

///Let's define an aspect that validates the meta-properties
///of the fiels
///
///In point model we define that the minimum value of all fields must be 0.
///We will write an aspect for checking the minimum alue of the incoming data.





/// Each aspect must have a name:


template<class T>
struct validate_aspect : aspect_base<"validate"> {

    template <typename F, typename V>
    void validate(F&& field, const V& value){
        if constexpr(is_model_v<F>){
            for_each(field,[](auto&& f, auto& v) {
                validate(f);

            });
        }
        else {
            using properties = std::remove_cvref_t<decltype(field.properties)>;
            if constexpr(scymnus::has_type<typename constraints::min<int>, properties>::value){

                 auto v = std::get<constraints::min<int>>(field.properties).value();

                if (value < v)
                    throw std::runtime_error{"field value is less than minimum value allowed"};
            }
        }
    }


    sink<"validate"> operator()(body_param<"body", T> body, context& ctx)
    {
        //validate
        try {
        for_each(body.get(),[this](auto&& f, auto& v) {
            validate(f,v);
        });
        }
        catch(std::runtime_error& exp) {
            return response{status<500>, std::string(exp.what()),  ctx};
        }

        return {};
    }
};



///Aspects can return a response.
///Also, it is possible for aspects to introduce a header or a query param.
///The swagger document is updated with the
///responses returned from aspects as well for the headers or query parameters
/// that aspects introduce.
/// (path parameters and body parameters are not updating the swagger doc)
///
/// Also aspects have a hook_type
///hook_type has one of two possible values:
///
/// hook_type:before (default)  and
/// hook_type::after
///
/// aspects with hook_type before, will be exectud before the main handler
/// aspects with hook_type after, will be executed after the main handler
///
///
///
///Let's introduce a new header, named operator of type int .
/// The operator header will be checked by the aspect.
/// If operator is different than the values 1 or 2 an 500 http status will be returned




struct check_operator_aspect : aspect_base<"check_operator", hook_type::before> {
    sink<"check_operator"> operator()(header_param<"operator", int> oper, context& ctx)
    {
        auto v = oper.get();
        if (v != 1 && v != 2){
            return response{status<500>, std::string{"Given operator is not supprted"},  ctx};
        }
        return {};
    }
};


/// Let's define a before aspect that will log the request:

struct log_request_aspect : aspect_base<"log_url", hook_type::before> {
    sink<"log_request"> operator()(context& ctx)
    {
        std::cout << "URL IS: " << ctx.raw_url << std::endl;
        return {};
    }
};

/// Let's define an after aspect that will log the response:

struct log_response_aspect : aspect_base<"log_url", hook_type::after> {
    sink<"log_response"> operator()(context& ctx)
    {
        std::cout << "RESPONSE IS: " << ctx.res.body << std::endl;
        return {};
    }
};



///lets now define our main function


int main(){
    /// let's add some
    api_manager::instance().name("Your name");
    api_manager::instance().email("your@email.com");
    api_manager::instance().host("127.0.0.1:9090");
    api_manager::instance().add_consume_type("aplication/json");
    api_manager::instance().add_produce_type("aplication/json");
    api_manager::instance().add_scheme("http");
    api_manager::instance().swagger_path("scymnus/external/swagger/dist/index.html");


    auto& app = scymnus::app::instance();


    app.route([](body_param<"body", PointModel> body, context& ctx) -> response_for<http_method::POST, "/points">
           {
               auto p = body.get();
               p.get<"id">() = points.size(); //code is not thread safe

               points[points.size()] = p;
               return response{status<200>, p, ctx};
           },
           log_request_aspect{},
           validate_aspect<PointModel>{},
           check_operator_aspect{},
           log_response_aspect{}
           )
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

              },
        log_request_aspect{},
        check_operator_aspect{},
        log_response_aspect{})

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

           },
           log_request_aspect{},
           check_operator_aspect{},
           log_response_aspect{})
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


    app.listen("127.0.0.1", "9090");
    app.run();
}

