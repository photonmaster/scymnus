#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "utilities/utils.hpp"

namespace scymnus {

enum class hook_type : uint8_t { before, after };

template <meta::ct_string Name, hook_type Hook = hook_type::before,
         bool Mandatory = (Hook == hook_type::before)?false:true>
struct aspect_base {
    static constexpr const char *name = Name.str();
    static constexpr hook_type hook = Hook;
    static constexpr bool is_mandatory = Mandatory;
};

template <meta::ct_string AspectName, class T> class aspect_registrator {
public:
    aspect_registrator() {
        api_manager::instance().add_aspect_response(AspectName.str(), T::status,
                                                    traits<T>::describe());
    }
};

template <meta::ct_string AspectName, class Response> struct register_aspect {
    inline register_aspect() { &reg_; }

    static aspect_registrator<AspectName, Response> reg_;
};

template <meta::ct_string AspectName, class Response>
aspect_registrator<AspectName, Response>
    register_aspect<AspectName, Response>::reg_;

template <meta::ct_string AspectName> class sink {
public:
    static constexpr const char *aspect_name = AspectName.str();
    template <class T> inline constexpr sink(T &&t) {
        register_aspect<AspectName, T>();
    }
    sink() = default;
};

} // namespace scymnus
