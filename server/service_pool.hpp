#pragma once
#include <vector>
#include <memory>
#include <future>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/executor_work_guard.hpp>

#include "service_pool_manager.hpp"

namespace scymnus
{

class service_pool{
public:
    explicit  service_pool(std::size_t pool_size) : pool_size_(pool_size),
        next_io_service_(0)
    {

        if (pool_size_ == 0)
            pool_size_ = 1;

        for (std::size_t i = 0; i < pool_size_; ++i)
        {
            pool_.push_back(std::make_unique<boost::asio::io_context>());
        }
    }


    void run() {

        std::vector<std::future<void>> v;
        std::atomic<std::size_t> init_count(0);
        for(uint16_t i = 0; i < pool_size_; i ++){
            init_count++;
            v.push_back(
                std::async(std::launch::async, [this, i]{

                    io_info().set(pool_[i].get());

                    while(1)
                    {
                        try
                        {
                            auto w = boost::asio::executor_work_guard{pool_[i]->get_executor()};
                            if (pool_[i]->run() == 0)
                            {
                                break;
                            }
                        } catch(std::exception& e)
                        {
                            std::cout << "An uncaught exception occurred: " << e.what() << std::endl;
                        }
                    }

                }));

        }

        //        signals_.async_wait(
        //            [&](const boost::system::error_code& /*error*/, int /*signal_number*/){
        //                stop();
        //            });

        while(pool_size_ != init_count)
            std::this_thread::yield();

    }


    void stop() {

        for(uint16_t i = 0; i < pool_size_; i ++){
            //better call reset on work guard here
            pool_[i]->stop();
        }
    }

    boost::asio::io_context& next() {
        boost::asio::io_context& context = *pool_[next_io_service_];
        ++next_io_service_;
        if (next_io_service_ == pool_.size())
            next_io_service_ = 0;
        return context;
    }

private:
    boost::asio::io_context io_context_;
    std::size_t next_io_service_;
    std::size_t pool_size_;
    std::vector<std::unique_ptr< boost::asio::io_context>> pool_;
    //boost::asio::signal_set signals_;
};

} //namespace scymnus
