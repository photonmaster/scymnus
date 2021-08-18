#pragma once

#include "service_pool_manager.hpp"
#include <chrono>
#include <string>

#include "server/service_pool_manager.hpp"

namespace scymnus {

class date_manager {
public:
    static date_manager &instance() {
        static thread_local date_manager instance;
        return instance;
    }

    date_manager(date_manager const &) = delete;
    date_manager &operator=(date_manager const &) = delete;

    const std::string &get_http_time() { return entry_; }

    void append_http_time(std::pmr::string &response) { response.append(entry_); }

    const std::string &calculate_http_time() {

        using namespace std::chrono_literals;
        auto now = std::chrono::system_clock::now();

        if (now - last_update_ > 1s) {
            entry_.clear();
            char buff[32];

            std::time_t time = std::chrono::system_clock::to_time_t(now);
            tm tm = *gmtime(&time);
            std::strftime(buff, sizeof(buff), "%a, %d %b %Y %T", &tm);
            last_update_ = now;
            date_ = buff;

            entry_.append("Date:");
            entry_.append(date_);
            entry_.append(" GMT\r\n\r\n");
        }

        return entry_;
    }

private:
    date_manager() : timer_{io_info().get(), std::chrono::seconds(1)} {
        prepare_date();
        tick();
    }

    void prepare_date() {

        using namespace std::chrono_literals;
        auto now = std::chrono::system_clock::now();

        entry_.clear();
        char buff[32];

        std::time_t time = std::chrono::system_clock::to_time_t(now);
        tm tm = *gmtime(&time);
        std::strftime(buff, sizeof(buff), "%a, %d %b %Y %T", &tm);
        last_update_ = now;
        date_ = buff;

        entry_.append("Date:");
        entry_.append(date_);
        entry_.append(" GMT\r\n\r\n");
    }

    void tick() {
        prepare_date();
        timer_.expires_at(timer_.expires_at() + std::chrono::seconds(1));
        timer_.async_wait(
            [this](const boost::system::error_code & /*ec*/) { this->tick(); });
    }

    std::chrono::system_clock::time_point last_update_{};

    boost::asio::steady_timer timer_;

    std::string date_{29, '\0'};
    std::string entry_;
};

} // namespace scymnus
