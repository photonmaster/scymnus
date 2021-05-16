# pragma once

#include <map>


namespace scymnous {

class query_string
{
public:
    ///get single value
    std::optional<std::string_view> get(std::string_view key) const
    {
        auto v = parameters_.find(key);


        if (v != parameters_.end())
            return (*v).second;
        return {};
    }

    std::optional<std::string_view> get_list(std::string_view key) const
    {
        auto v = parameters_.find(key);


        if (v != parameters_.end())
            return (*v).second;
        return {};
    }



private:
    void parse(std::string_view query){
        
        const char* begin = query.begin();
        const char* it = query.begin();

        
        const char* end = query.end();
        
        std::string_view key;
        std::optional<std::string_view> value;
        
        while(it != query.end()){
            switch (*it)
            {
            case '=':
                key = std::string_view{begin, static_cast<std::string_view::size_type>(it - begin)};
                it++;
                begin = it;
                break;
                
            case '&':
                if (key.size())
                    value = std::string_view{begin, static_cast<std::string_view::size_type>(it - begin)};
                else
                    key = std::string_view{begin, static_cast<std::string_view::size_type>(it - begin)};
                
                it++;
                begin = it;
                
                //store values;
                parameters_.emplace(key, value);
                break;
            default:
                it++;
            }
            
            
            //last parameter
            if (key.size())
                value = std::string_view{begin, static_cast<std::string_view::size_type>(it - begin)};
            else
                key = std::string_view{begin, static_cast<std::string_view::size_type>(it - begin)};


            parameters_.emplace(std::piecewise_construct,
                                std::forward_as_tuple(key),
                                std::forward_as_tuple(value));

        }
        
    }

    std::multimap<std::string_view,std::optional<std::string_view>> parameters_;
    friend class context;
};


} //namespace scymnous
