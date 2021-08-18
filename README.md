## Introduction
**Scymnus** is a web service framework for c++.
Swagger documentation is generated automatically without macros.
It tries to solve the lack of reflection in c++ by using c++20 feutures


**This is a work in progress**

> *Scymnus* is an ancient greek word for lion cub
## Building
For building *Scymnus* the following are needed:

- gcc 10.3 or greater
- Boost 1.70 or later is needed (Boost Asio, utils)
- cmake 

## Examples and snippets
### hello world

first a using directive for making things easier:
```cpp
using namespace scymnus;
```

In the beggining of the `main()` function, we are taking a reference to the instance of the singleton class `scymnus::app`, that we will be using in the rest of the code:

```cpp
int main() {
    auto& app = scymnus::app::instance();
```

our service will expose a single `GET` endpoint. Two path integer parameters named x and y must be given by clients. The service will return the sum of the two numbers

```cpp
    app.route([](path_param<"x", int> x,path_param<"y", int> y, context& ctx)
                  -> response_for<http_method::GET, "/sum/{x}/{y}">
              {
                  auto sum = x.get() + y.get();
                  return ctx.write(status<200>, sum);
              })
        .summary("Integer addition")
        .description("Returns the sum of two integer numbers. The two numbers are given as path parameters")
        .tag("calculator");
```
 
Finally we are starting the webservice:
 ```cpp
    app.listen();
    app.run();
```
}







complete code:
```cpp
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

    app.listen();
    app.run();
}
```
##### Enabling swagger
For serving the swagger documentation of our webservice a folder named
`swagger_resources` must be created first **at the same location with the executable**.

The `swagger_resources` folder should contain all the files in the `external/swagger/dist` directory of the source tree.  

Swagger is accessible here: http://127.0.0.1:8080/api-doc

##### Query and header parameters

In the above snippet arguments of type `path_param<>`  were used as arguments.
Similar to `path_param<>` arguments one can use `query_param<>`, `header_param<>` or
`body_param<>` arguments for introducing query, header of body parameters:


***Query parameters***
```cpp
    app.route([](query_param<"a", int> a,query_param<"b", int> b, context& ctx)
                  -> response_for<http_method::GET, "/midpoint">
              {
                  auto mid = std::midpoint(a.get() , b.get());
                  return ctx.write(status<200>, mid);
              })
        .summary("midpont value")
        .description("Returns the midpoint of two integer numbers. The two numbers are given as query parameters")
        .tag("calculator");
```
***Header parameters***
```cpp
    app.route([](header_param<"value", int> value, context& ctx)
                  -> response_for<http_method::GET, "/abs">
              {
                  auto abs = std::abs(value.get());
                  return ctx.write(status<200>, abs);
              })
        .summary("Absolute value")
        .description("Returns the absolute value of an integer number. The number is given as a header parameter")
        .tag("calculator");
```

***Body parameter***
```cpp
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
```
please check the `calculator` example for more information.


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
Because of this, serialization/deserialization to/from json as well as swagger documentation cannot be automated.

Instead of using simple  `struct`s Scymnus uses templates to store meta data about types.

So for a 3D point the following type alias is used:

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
json v = p;

//prints:
//p: {"x":1,"y":2,"z":3}
std::cout << "p: " << v.dump() << '\n';
```

##### Adding meta-properties
For each field in a model, meta-properties can be defined.
A model itself can also have meta-properties.
Two properties are avaliable at model level: `name` and `description`

Each field can have each own properties. The following field properties can be used:
- description
- min
- max
- init
- ...

It is possible for the users of Scymnus to define user defined meta-properties 

Here is the updated 3d point model, where meta-properties are used:

```cpp
using PointModel = model<
    field<"id", std::optional<int>, description("Server side generated value")>, //server sets this
    field<"x", int, constraints::min(0), description("X coordinate of 3D point")>,
    field<"y", int, constraints::min(0), description("Y coordinate of 3D point")>,
    field<"z", std::optional<int>, init<[](){return 2;}>{},  description("Z coordinate of 3D point")>,
    properties<name("PointModel"), description("A 2d point model used in example")>
    >;
```
`init` meta property is used for assigning a default value for a field that is not present in a request. the type of the field must be an optional in this case.


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
    field<"z", std::optional<int>, init<[](){return 2;}>{}),  description("Z coordinate of 3D point")>,
    field<"c", std::optional<ColorModel>,init<[](){return ColorModel{127,127,127};}>{},  description("Color details of point")>,
    properties<name("PointModel"), description("A 2d point model used in example")>
    >;
```
> std::optional<> is used whenever a type is not required 

now with the models in place, we are ready to define the controller for creating objects:
```cpp
std::map<int, PointModel> points{}; //map for holding created points

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
...
```
when sending the request below:
```
curl -X POST "http://127.0.0.1:8080/points" -H  "accept: aplication/json" -H  "Content-Type: aplication/json" -d "{ \"x\": 0,  \"y\": 0  }"
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
  "z": 1
}
```
### aspects
Scymnus supports before and after aspects. An aspect is a piece of code that is executed by Scymnus before or after the main handler.

##### Defining aspects
Aspects are callable objects inheriting from `aspect_base<>`. Each aspect must have a name and their call operator must return a `sink<>` object:

```cpp
struct log_request_aspect : aspect_base<"log_request", hook_type::before> {
    sink<"log_request"> operator()(context& ctx)
```
the "name" parameter in `sink<>`  type  **must** be the same to the  "name" parameter in the `aspect_base<>` base class.

##### Example
Below, two aspects are defined. The first one  is executed before the main handler(`hook_type::before`) and the second one is executed after the main handler (`hook_type::after`).  

```cpp
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

```

Aspects can be enabled for an andpoint like is shown below:

```cpp
    app.route([](body_param<"body", json> body,context& ctx)->response_for<http_method::POST, "/echo"> {

        return ctx.write_as<http_content_type::JSON>(status<200>,ctx.request_body());
    }, log_request_aspect{},
       log_response_aspect{}
    ).summary("echo back the json payload in the request")
     .tag("echo");
```
> check the `echo_aspects` and `aspects` examples for details

Aspects can return a response or introduce  header or query parameters as can been seen here:

```cpp
struct check_operator_aspect : aspect_base<"check_operator", hook_type::before> {
    sink<"check_operator"> operator()(header_param<"operator", int> oper, context& ctx)
    {
        auto v = oper.get();
        if (v != 1 && v != 2){
            return ctx.write_as<http_content_type::JSON>(status<500>, std::string{"Given operator is not supported"});
        }
        return {};
    }
};
```


The swagger document is updated with the responses returned from aspects as well as with the headers or query parameters used for calling an aspect
(path parameters and body parameters are not updating the swagger doc)

  


### TODO
- [x] HTTP pipelining
- [ ] HTTPS
- [ ] compression
- [ ] chunked transfer encoding
- [ ] enumerations in swagger
- [ ] jwt tokens
- [ ] websockets

## 3rd party tools included in source tree
- llhttp parser (https://github.com/nodejs/llhttp) 
- nlohmann/json (https://github.com/nlohmann/json)
- url parsing is taken from Microsoft/cpprestsdk (https://github.com/Microsoft/cpprestsdk)
- MimeTypeMap by Samuel Neff, (https://github.com/samuelneff/MimeTypeMap)
- decimal_from function by Alf P. Steinbach  (https://ideone.com/nrQfA8)

