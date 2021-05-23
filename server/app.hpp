#pragma once

#include "api_manager.hpp"
#include "router.hpp"
#include "server.hpp"
#include "controllers/swagger_controller.hpp"


namespace scymnus {

class app{

public:
    static app& instance(){
        static app instance;

        return instance;
    }


    void run() {

        server_.run();
    }


    bool listen(std::string_view address, std::string_view port) {
        return server_.listen(address,port);
    }


    template <class F,  typename... T>
    router_parameters route(F &&f, T&&... t){
          return router_.route(f, t...);
    }

    template <class F,  typename... T>
    void route_internal(F &&f, T&&... t){
        return router_.route_internal(f, t...);
    }



    template<class Callable>
    void set_excpetion_handler(Callable&& callable){
        router_.exception_handler_.reset(std::forward<Callable>(callable));
    }


private:

    app(const app&) = delete;
    app(app&&) = delete;
    app() {
        route_internal(scymnus::get_swagger_description_controller{});
        route_internal(scymnus::api_doc_controller{});
        route_internal(scymnus::swagger_controller_files{});
    }

    scymnus::http_server<> server_{std::thread::hardware_concurrency()};
    router router_;
};


} //nameapace scymnus
