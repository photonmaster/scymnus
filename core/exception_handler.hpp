#pragma once

#include <functional>

namespace scymnous {

///exception  dispacher for handling intenal exception thrown


class context;

class exception_handler
{
public:
    template<class Callable>
    exception_handler(Callable&& handler) : handler_{std::forward<Callable>(handler)}
    {
    //    static_assert(std::is_void_v<decltype(handler())>, "Callable parameter must be of type: void()");
    }

    auto operator() (context& ctx) {
        return handler_(ctx);
    }

    template<class Callable>
    void reset(Callable&& handler) {
        handler_ = handler;
    }

private:
    std::function<void(context&)> handler_;
};

} //namespace scymnous
