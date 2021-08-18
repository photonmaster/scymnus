#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <list>
#include <memory_resource>
#include <thread>

namespace scymnus {

class memory_resource_manager {

public:
    static memory_resource_manager &instance() {
        thread_local memory_resource_manager manager;
        return manager;
    }

    auto pool() {

        // return std::pmr::new_delete_resource();
        return &pool_;
    }

private:
    std::pmr::unsynchronized_pool_resource
        pool_{}; //{std::data(buffer_), std::size(buffer_)};

    memory_resource_manager() = default;
    memory_resource_manager(const memory_resource_manager &) = delete;
    memory_resource_manager &operator=(const memory_resource_manager &) = delete;
};

} // namespace scymnus
