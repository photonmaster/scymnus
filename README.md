## Introduction
**Scymnous** is a web service framework for c++.
Swagger documentation is generated automatically without macros.
It tries to solve the lack of reflection in c++ by using c++20 feutures
and heavily relying on templates.

**This is a work in progress of pre-alpha quality**

## Usage
- Scymnous can be compiled with gcc 9.1
- Boost 1.70 is needed (Boos Asio, utils)
- cmake

## 3rd party tools included in source tree
- llhttp parser (https://github.com/nodejs/llhttp) 
- nlohmann/json (https://github.com/nlohmann/json)
- url parsing is taken from Microsoft/cpprestsdk (https://github.com/Microsoft/cpprestsdk)
- MimeTypeMap by Samuel Neff, (https://github.com/samuelneff/MimeTypeMap)

## useful resources
### github 


from https://github.com/envoyproxy (envoy proxy)
the implemetation of `inline std::string_view ltrim(std::string_view source)` 
was taken

## Examples and snippets
### hello world

first a using directive for makign things easier
```cpp
using namespace scymnous;
```

we are using the `api_manager::instance()` to setup some basic properties of our service.
in order to be able to browse swagger documentation a path to swagger resources
must be set first by calling ` api_manager::instance().swagger_path()`
the path must be relative to the executable.
E.g.: 
`api_manager::instance().swagger_path("scymnous/external/swagger/dist/index.html");`

all needed files can be found in `external/swagger/dist` directory.



```cpp
int main(){
      api_manager::instance().name("your name");
      api_manager::instance().title("Calculator service");
      api_manager::instance().version(0,1);
      api_manager::instance().email("your email");
      api_manager::instance().host("10.0.2.15:9090");
      api_manager::instance().add_consume_type("aplication/json");
      api_manager::instance().add_produce_type("aplication/json");
      api_manager::instance().add_scheme("http");
      api_manager::instance().swagger_path("scymnous/external/swagger/dist/index.html");
```

 `scymnous::app` is a singleton. We are taking a reference on it, named app,
 that we will be using in the rest of the code
 
 ```cpp
    auto& app = scymnous::app::instance();
```
our service will expose a single GET endpoint. Two path integer parameters named  `x` and `y` must be given by clients. The service will return the sum of the two integers 
 ```cpp
    app.route([](path_param<"x", int> x,path_param<"y", int> y, context& ctx)
                  -> response_for<http_method::GET, "/sum/{x}/{y}">
           {
               auto sum = x.get() + y.get();
               return response{status<200>, sum, ctx};
              })
        .summary("Integer addition")
        .description("Returns the sum of two integer numbers")
        .tag("calculator");
```

Finally we are starting the webservice.
Swagger is accessible here: http://10.0.2.15:9090/api-doc

 ```cpp
    app.listen("10.0.2.15", "9090");
    app.run();
```
}

complete code:
```cpp
#include "server/app.hpp"

using namespace scymnous;

int main(){
      api_manager::instance().name("your name");
      api_manager::instance().title("Calculator service");
      api_manager::instance().version(0,1);
      api_manager::instance().email("your email");
      api_manager::instance().host("10.0.2.15:9090");
      api_manager::instance().add_consume_type("aplication/json");
      api_manager::instance().add_produce_type("aplication/json");
      api_manager::instance().add_scheme("http");
      api_manager::instance().swagger_path("scymnous/external/swagger/dist/index.html");

    auto& app = scymnous::app::instance();

    app.route([](path_param<"x", int> x,path_param<"y", int> y, context& ctx)
                  -> response_for<http_method::GET, "/sum/{x}/{y}">
           {
               auto sum = x.get() + y.get();
               return response{status<200>, sum, ctx};
           })
        .summary("Integer addition")
        .description("Returns the sum of two integer numbers")
        .tag("calculator");

    app.listen("10.0.2.15", "9090");
    app.run();
}
```
### models
Let's say that a simple web service must be created for manipulating 3D points.
The following `struct` looks like a good candidate for presenting a 3D point:

```cpp
struct point
{
    int x;
    int y;
    int z;
};
```
Unfortunatelly the above `struct` is not reflectable.
Because of this serialization/deserialization to/from json, and swagger documentation cannot be automated.

Instead of using simple  `struct`s Scymnus uses templates to store meta data about types.

So for a 3D point the following using directive is used:

```cpp
using PointModel = model<
    field<"x", int>,
    field<"y", int>,
    field<"z", int>
    >;
```

Now it is easy to serialise to json:
```cpp
auto p = PointModel{1,2,3};  // a point with x == 1, y == 2, z == 3
json v;
to_json(v, p);
//prints:
//p: {"x":1,"y":2,"z":3}
std::cout << "p: " << v.dump() << '\n';
```

##### Adding meta-properties
For each field in a model meta-properties can be defined.
A model can have meta-properties on each own.
Two properties are avaliable at model level: name and description
Each field can have each own properties. The following field properties can be defined:
- description
- min
- max
- init
- ...

It is possible for the users of Scymnous to define user defined meta-properties 

Here is the updated 3d point model, where meta-properties are used:

```cpp
using PointModel = model<
    field<"id", std::optional<int>, description("Server side generated value")>, //server sets this
    field<"x", int, constraints::min(0), description("X coordinate of 3D point")>,
    field<"y", int, constraints::min(0), description("Y coordinate of 3D point")>,
    field<"z", std::optional<int>, init<int>([](){return 2;}),  description("Z coordinate of 3D point")>,
    properties<name("PointModel"), description("A 2d point model used in example")>
    >;
```
`init` meta property is used to assign a default value for a field that is not present in a request.

Of course models can be nested. Let's add an optional color property to the 3d point by first defining a ColorModel model:

```cpp
using ColorModel = model<
    field<"r", int, description("red")>,
    field<"g", int, description("green")>,
    field<"b", int, description("blue")>,
    properties<name("ColorModel"), description("Color coordinates")>
    >;
```

The updated PointModel is:
```cpp
using PointModel = model<
    field<"id", std::optional<int>, description("Server side generated value")>, //server sets this
    field<"x", int, constraints::min(0), description("X coordinate of 3D point")>,
    field<"y", int, constraints::min(0), description("Y coordinate of 3D point")>,
    field<"z", std::optional<int>, init<int>([](){return 2;}),  description("Z coordinate of 3D point")>,
    field<"c", std::optional<ColorModel>,init<ColorModel>([](){return ColorModel{127,127,127};}),  description("Color details of point")>,
    properties<name("PointModel"), description("A 2d point model used in example")>
    >;
```

now with the models in place, we are ready to define the controller for creating objects:
```cpp
std::map<int, PointModel> points{}; //map for holding created points

int main(){
    api_manager::instance().name("Your name");
    api_manager::instance().email("your@email.com");
    api_manager::instance().host("10.0.2.15:9090");
    api_manager::instance().add_consume_type("aplication/json");
    api_manager::instance().add_produce_type("aplication/json");
    api_manager::instance().add_scheme("http");
    api_manager::instance().swagger_path("scymnous/external/swagger/dist/index.html");

    auto& app = scymnous::app::instance();
    
    app.route([](body_param<"body", PointModel> body,
                 context& ctx) -> response_for<http_method::POST, "/points">
              {
                  auto p = body.get();
                  p.get<"id">() = points.size(); //code is not thread safe
                  return response{status<200>, p, ctx};
                  
              })
        .summary("create a point")
        .description("create a point")
        .tag("points");
```
when sending the request below:
```
curl -X POST "http://10.0.2.15:9090/points" -H  "accept: aplication/json" -H  "Content-Type: aplication/json" -d "{ \"x\": 0,  \"y\": 0  }"
```
the response would look like:
```javascript
{
  "c": {
    "b": 127,
    "g": 127,
    "r": 127
  },
  "id": 0,
  "x": 0,
  "y": 0,
  "z": 2
}
```
### aspects
Scymnous supports before and after aspects. An aspect is a piece of code that is executed by Scymnous before or after the main handler.

#### example
```cpp
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

///Defining aspects.
/// An aspect is a mechanism for separating cross cutting concerns from the rest of the code.
/// E.g. checking if a token in a request is valid, is a functionality that is ///shared between all of the controllers of our web service.
/// The code  that validates tokens can be seperated in an aspect and the the aspect /// to be enabled for the controllers needed

///Let's define an aspect that validates the meta-properties of PointModel
///In point model we define that the minimum value of all fields must be 0.
///We will write an aspect for checking the minimum values of PointModel fields.

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
            if constexpr(scymnous::has_type<typename constraints::min<int>, properties>::value){
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
///Let's introduce a new header, named operator of type int .
/// The operator header will be checked by an aspect.
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


/// Let's define a before aspect that will log the url requested:

struct log_request_aspect : aspect_base<"log_url", hook_type::before> {
    sink<"log_request"> operator()(context& ctx)
    {
        std::cout << "URL IS: " << ctx.raw_url << std::endl;
        return {};
    }
};

/// Let's define an after aspect that will log the response body:

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
    api_manager::instance().host("10.0.2.15:9090");
    api_manager::instance().add_consume_type("aplication/json");
    api_manager::instance().add_produce_type("aplication/json");
    api_manager::instance().add_scheme("http");
    api_manager::instance().swagger_path("scymnous/external/swagger/dist/index.html");

    auto& app = scymnous::app::instance();


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

    /// Finally we are starting the webservice
    /// swagger is accessible here: http://10.0.2.15:9090/api-doc
    app.listen("10.0.2.15", "9090");
    app.run();
}
```
### TODO
- better HHTP 1.1 support
- memory reclamation policies for buffer_pool (now memory is never released only reused)
- bug fixing
- better swagger support e.g. enums
- tests
- https support
- authorization, jwt tokens
- websockets
- documentation


