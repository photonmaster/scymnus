#pragma once

#include<chrono>
#include <string>


namespace scymnus {

class date_manager
{
public:
    static date_manager& instance()
    {
        static thread_local date_manager instance;
        return instance;
    }

    date_manager(date_manager const &) = delete;
    date_manager& operator=(date_manager const&) = delete;


    std::string get_date()
    {
        using namespace std::chrono_literals;
        auto now = std::chrono::system_clock::now();

        if (now - last_update_ > 1s) {
            char buff[32];

            std::time_t time = std::chrono::system_clock::to_time_t(now);
            tm tm = *gmtime(&time);
            std::strftime(buff, sizeof(buff), "%a, %d %b %Y %T", &tm);
            last_update_ = now;
            date_ = buff;
        }
        return date_;
    }

private:
    date_manager() = default;

    std::chrono::system_clock::time_point last_update_{};
    std::string date_ = std::string(29,'\0');
};

} //namespace scymnus
