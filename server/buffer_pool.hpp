#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <list>
#include <iostream>
#include <thread>

namespace scymnus {

class buffer_manager {
public:
    static buffer_manager& get(){
        thread_local buffer_manager manager;
        return  manager;
    }

    void *allocate();
    void release(void *);
    void reset();

    size_t free_size() const noexcept
    {
        return free.size();
    }
    size_t used_size() const noexcept
    {
        return used.size();
    }

    ~buffer_manager() {
        reset();

        for (auto it = free.begin(); it != free.end(); ++it)
            delete[] *it;
    }


private:
    static constexpr std::size_t chunk_size = 4096;
    buffer_manager() = default;
    buffer_manager( const buffer_manager & ) = delete;
    buffer_manager & operator =( const buffer_manager &) = delete;


    std::list<char*>  used;
    std::list<char*>  free;
};


inline void* buffer_manager::allocate()
{
#ifdef TEST_DEBUG
    std::cout << "Buffer pool allocation in thread:" << std::this_thread::get_id() << std::endl;
#endif
    if (free.empty()){
        used.push_front( new char[chunk_size]);

    }
    else {

        used.splice(used.begin(), free, free.begin());
    }
    return used.front();
}

inline  void buffer_manager::release(void *buffer)
{
#ifdef TEST_DEBUG
std::cout << "Buffer pool release in thread:" << std::this_thread::get_id() << std::endl;
#endif
    auto it = std::find(used.begin(), used.end(), buffer);
    if (it != used.end()){
      free.splice(free.begin(), used, it);
    }
        else {
        std::cout << "buffer pool unexpected error\n";
        }
}

inline void buffer_manager::reset()
{
    free.splice(free.begin(), used);
}


struct buffer_deleter{
    void operator()(char* p) {
        buffer_manager::get().release(p);
    }
};

} //namespace scymnus
