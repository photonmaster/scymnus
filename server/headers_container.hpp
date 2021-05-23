#pragma once

#include <string_view>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <boost/functional/hash.hpp>
#include <iostream>


namespace scymnus
{

template<class... Ts> struct overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts> overload(Ts...) -> overload<Ts...>;



template<
    class CharT,
    class Traits = std::char_traits<CharT>
    >
class string_view_cluster
{

    friend std::ostream & operator << (std::ostream &out, const string_view_cluster &c);
    size_t length_{0};


private:
    template<typename ValueType>
    struct iterator : public std::iterator<std::input_iterator_tag, ValueType>
    {


        const string_view_cluster* cluster_;
        friend class string_view_cluster;

        ValueType const * ptr_{nullptr};
        size_t idx_{1};
        size_t v_idx_{0};




        iterator(const string_view_cluster* cluster) noexcept : cluster_(cluster)
        {
            if(cluster_->size())
                ptr_ = cluster_->fragments_.front().begin();
        }

        inline  iterator end(){
            ptr_ = cluster_->fragments_.back().end();
            return(*this);
        }


        //check operators
        inline friend  bool operator==(const iterator& lhs, const iterator& rhs) noexcept
        {
            return lhs.ptr_ == rhs.ptr_;
        }

        inline friend bool operator!=(const iterator& lhs, const iterator& rhs)  noexcept
        {
            return  lhs.ptr_ != rhs.ptr_;
        }

        //prefix
        inline const iterator& operator++() noexcept
        {

            if (idx_ == (cluster_->fragments_[v_idx_].size()))
            {
                ++v_idx_;
                idx_ = 1;

                if (v_idx_ == cluster_->fragments_.size()){
                    ptr_ = cluster_->fragments_.back().end();

                    bool equal = (*this == this->end());
                }
                else {

                    ptr_ = &(cluster_->fragments_[v_idx_][0]);

                }
            }
            else {
                ++ptr_;
                ++idx_;
            }

            return *this;

        }

        //post incr
        inline const iterator& operator++(int) noexcept
        {
            iterator t{*this};
            ++t;
            return t;
        }

        const ValueType& operator*() const noexcept
        {
            return *ptr_;
        }
        const ValueType* operator->() const noexcept
        {
            return ptr_;
        }

        //  friend void swap(iterator& lhs, iterator& rhs); //C++11 I think
    };

public:
    string_view_cluster() = default;
    string_view_cluster& operator=(string_view_cluster&&)  = default;
    string_view_cluster(string_view_cluster&&) = default;

    std::vector<std::string_view> fragments_;

    std::string to_string() const
    {
        //TODO: reserve space
        std::string str;
        for(const auto& sv: fragments_){
            str.append(sv);
        }
        return str;
    }

    void append(std::string_view sv){
        if (sv.empty())
            return;
        fragments_.push_back(sv);
        length_ += sv.length();
    }


    using traits_type	= Traits;
    using value_type	= CharT;
    using pointer		= value_type*;
    using const_pointer	= const value_type*;
    using reference		= value_type&;
    using const_reference	= const value_type&;
    using const_iterator	=    iterator<value_type>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator	= const_reverse_iterator;
    using size_type		= size_t;
    using difference_type	= ptrdiff_t;
    static constexpr size_type npos = size_type(-1);

    constexpr const_iterator
    begin() const noexcept
    {
        return iterator<value_type>(this);
    }

    constexpr const_iterator
    end() const noexcept
    {
        if (fragments_.size())
            return iterator<value_type>(this).end();
        return nullptr;
    }

    constexpr const_iterator
    cbegin() const noexcept
    {
        return begin();
    }

    constexpr const_iterator
    cend() const noexcept
    {
        return end();
    }

    constexpr const_reverse_iterator
    rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    constexpr const_reverse_iterator
    rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    constexpr const_reverse_iterator
    crbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    constexpr const_reverse_iterator
    crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }


    constexpr size_type
    size() const noexcept
    {
        return length_;
    }

    constexpr size_type
    length() const noexcept
    {
        return length_;
    }

    constexpr size_type
    max_size() const noexcept
    {
        return (npos - sizeof(size_type) - sizeof(void*))
               / sizeof(value_type) / 4;
    }

    [[nodiscard]] constexpr bool
    empty() const noexcept
    {
        return fragments_.size() == 0;
    }
};



inline std::ostream & operator << (std::ostream &out, const string_view_cluster<char> &c)
{
    for(auto f: c.fragments_)
        out << f;
    return out;
}



using message_data_t = std::variant<std::string_view, string_view_cluster<char>>;


inline  std::ostream & operator << (std::ostream &out, const message_data_t &h)
{

    std::visit([&out](auto&& l)
               {
                   out << l;
               }, h);
    return out;

}


//only ascii
char to_lower(char c) {
    if (c <= 'Z' && c >= 'A')
        return c - ('Z' - 'z');
    return c;
}


inline bool iless(char lhs, char rhs){
    return to_lower(lhs) < to_lower(rhs);
}


//'A': 65, 'a': 97
//comapre ascii chars
inline bool icompare(char lhs, char rhs){
    if (lhs == rhs) return true;
    return lhs > rhs? static_cast<unsigned>(rhs) + 32L == lhs: static_cast<unsigned>(lhs) + 32L == rhs;
}


inline char toupper(char c) {
    return ('a' <= c && c <= 'z') ? c^0x20 : c;
}


//TODO: refactor this
inline  bool insensitive_comp(const message_data_t& lhs, const message_data_t& rhs)
{

    return   std::visit(overload{
                          [](const std::string_view& l, const std::string_view& r )
                          {
                              return  std::lexicographical_compare(std::cbegin(l),std::cend(l),std::cbegin(r),std::cend(r),iless);

                          },
                          [](const std::string_view& l, const string_view_cluster<char>& r)
                          {
                              return  std::lexicographical_compare(std::cbegin(l),std::cend(l),std::cbegin(r),std::cend(r),iless);

                          },
                          [](const string_view_cluster<char>& l, const string_view_cluster<char>& r )
                          {

                              return  std::lexicographical_compare(std::cbegin(l),std::cend(l),std::cbegin(r),std::cend(r),iless);

                          },
                          [](const string_view_cluster<char>& l, const std::string_view& r )
                          {
                              return  std::lexicographical_compare(std::cbegin(l),std::cend(l),std::cbegin(r),std::cend(r),iless);

                                  },
                          }, lhs, rhs);
}




inline  bool iequals(const std::string& lhs, const std::string&  rhs)
{

    return (lhs.size() == rhs.size()) &&
           std::equal(std::begin(lhs),std::end(lhs),std::begin(rhs),icompare);
}


struct ihash{
    size_t operator()(const message_data_t& field) const
    {
        std::size_t seed = 0;
        std::visit(overload{
                       [&seed](const std::string_view& l)
                       {
                           for(auto c : l)
                           {
                               boost::hash_combine(seed, toupper(c));
                           }
                       },

                       [&seed](const string_view_cluster<char>& l)
                       {
                           for(auto c : l)
                           {
                               boost::hash_combine(seed, toupper(c));
                           }
                       }
                   },field);

        return seed;
    }

};


inline std::string to_string(const message_data_t& field)
{
    return std::visit<std::string>(overload{
                                       [](std::string_view l) ->std::string
                                       {
                                           return std::string(l);
                                       },

                                       [](const string_view_cluster<char>& l)->std::string
                                       {
                                           return l.to_string();
                                           //return std::string{};
                                       }
                                   },field);

}

inline std::size_t size(const message_data_t& v)
{

    [[likely]] if (std::holds_alternative<std::string_view>(v))
    {

        return std::get<std::string_view>(v).size();

    }
    else {
        return std::get<string_view_cluster<char>>(v).size();
    }
}



struct insensitive_compare
{
    bool operator()(const message_data_t& lhs, const message_data_t& rhs) const
    {
        return insensitive_comp(lhs, rhs);
    }

    //TODO: fix this
    bool operator()(const std::string& lhs, const std::string& rhs) const
    {
        return iequals(lhs, rhs);
    }

};


using headers_container = std::multimap<message_data_t,message_data_t,insensitive_compare>;


//headers for http response

struct ci_hash
{
    size_t operator()(const std::string& key) const
    {
        std::size_t seed = 0;
        std::locale locale;

        for(auto c : key)
        {
            boost::hash_combine(seed, std::toupper(c, locale));
        }
        return seed;
    }
};



using res_headers = std::unordered_multimap<std::string, std::string, ci_hash, insensitive_compare>;

} //namespace scymnus





