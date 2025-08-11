#pragma once
#include <type_traits>
#include "stateful_meta.hpp"

namespace utils::meta {
/// This macro stores the innermost class ```Self``` enclosing the current context into
/// ```meta::const_var<utils_find_self_result_>``` (the type of the value being ```std::type_identity<Self>```).
///
/// The program is ill-formed if the macro is not used inside a class declaration,
/// or if a method named ```utils_find_self_``` or a member type named ```utils_find_self_result_``` is already declared.
#define UTILS_FIND_SELF\
    struct utils_find_self_result_;\
    consteval auto utils_find_self()\
    -> decltype(utils::meta::define<utils_find_self_result_, std::type_identity<std::remove_cvref_t<decltype(*this)>>{}>{}) {\
        throw "You are not supposed to call this method. See the documentation for UTILS_FIND_SELF.";\
    }
/// Retrieves the type found by ```UTILS_FIND_SELF```.
#define UTILS_FIND_SELF_TYPE typename decltype(get(utils::meta::const_var<utils_find_self_result_>{}))::type
}