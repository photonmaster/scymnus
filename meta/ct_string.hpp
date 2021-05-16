#pragma once

namespace scymnous {
namespace meta {



int constexpr length(const char* str)
{
    return *str ? 1 + length(str + 1) : 0;
}



template<unsigned N>
struct ct_string {

    char buf[N + 1] = {};
    constexpr unsigned size()  const {
        return N;
    }

    constexpr ct_string(char const*  s) {
        for (unsigned i = 0; i != N; ++i) buf[i] = s[i];
    }




    constexpr operator char const*() const {
        return buf;
    }
    constexpr char const* str() const{
        return buf;
    }
};

template<unsigned N> ct_string(char const (&)[N]) -> ct_string<N - 1>;
template<class A> ct_string(A a) -> ct_string<length(A::str())>;





} //namespace meta
} //namespace scymnous
