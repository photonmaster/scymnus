#pragma once

#include "api_manager.hpp"
#include "router.hpp"
#include "server.hpp"
#include "controllers/swagger_controller.hpp"


namespace scymnous {

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



    void match(context& ctx){
        router_.match(ctx);
    }


private:

    app(const app&) = delete;
    app(app&&) = delete;
    app() {
        route_internal(scymnous::get_swagger_description_controller{});
        route_internal(scymnous::api_doc_controller{});
        route_internal(scymnous::swagger_controller_files{});
    }

    scymnous::http_server<> server_{std::thread::hardware_concurrency()};
    router router_;
};


} //nameapace scymnous
