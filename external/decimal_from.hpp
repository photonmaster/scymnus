#pragma once

#include <limits>       // std::numeric_limits
#include <algorithm>    // std::reverse

//decimal_from function by Alf P. Steinbach  (https://ideone.com/nrQfA8)

namespace scymnus {


inline void unsigned_to_decimal( unsigned long number, char* buffer )
{
    if( number == 0 )
    {
        *buffer++ = '0';
    }
    else
    {
        char* p_first = buffer;
        while( number != 0 )
        {
            *buffer++ = '0' + number % 10;
            number /= 10;
        }
        std::reverse( p_first, buffer );
    }
    *buffer = '\0';
}

inline auto decimal_from_unsigned( unsigned long number, char* buffer )
    -> char const*
{
    unsigned_to_decimal( number, buffer );
    return buffer;
}

inline void to_decimal( long number, char* buffer )
{
    if( number < 0 )
    {
        buffer[0] = '-';
        unsigned_to_decimal( -number, buffer + 1 );
    }
    else
    {
        unsigned_to_decimal( number, buffer );
    }
}

inline auto decimal_from( long number, char* buffer )
    -> char const*
{
    to_decimal( number, buffer );
    return buffer;
}
}  // namespace scymnus
