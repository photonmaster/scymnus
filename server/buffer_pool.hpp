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

    void chunk_size(std::size_t size) {
        chunk_size_ = size;
    }

    size_t chunk_size() {
        return chunk_size_;
    }

    char *allocate();
    void release(char *);
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
    }


private:
    std::size_t chunk_size_{2 * 10 * 1000};
    buffer_manager() = default;
    buffer_manager( const buffer_manager & ) = delete;
    buffer_manager & operator =( const buffer_manager &) = delete;


    using buffer_t = std::vector<char>;

    std::list<buffer_t>  used;
    std::list<buffer_t>  free;
};


inline char* buffer_manager::allocate()
{
#ifdef TEST_DEBUG
    std::cout << "Buffer pool allocation in thread:" << std::this_thread::get_id() << std::endl;
#endif
    if (free.empty()){
        used.emplace_front(chunk_size_);

    }
    else {

        used.splice(used.begin(), free, free.begin());
    }
    return used.front().data();
}

inline  void buffer_manager::release(char *buffer)
{
#ifdef TEST_DEBUG
std::cout << "Buffer pool release in thread:" << std::this_thread::get_id() << std::endl;
#endif
    auto it = std::find_if(used.begin(), used.end(), [buffer](const std::vector<char>& v){return v.data() == buffer;});
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
