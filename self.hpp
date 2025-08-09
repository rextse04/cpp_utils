#pragma once
#include <type_traits>
#include "stateful_meta.hpp"

namespace utils::meta {
    struct find_self_result;

/// This macro stores the innermost class ```Self``` enclosing the current context into
/// ```meta::var<meta::find_self_result_var>``` (the type of the value being ```std::type_identity<Self>```).
///
/// The program is ill-formed if the macro is not used inside a class declaration,
/// or if a method named ```utils_find_self_``` is already declared.
#define UTILS_FIND_SELF\
    consteval void utils_find_self() {\
        utils::meta::set<utils::meta::find_self_result, std::type_identity<std::remove_cvref_t<decltype(*this)>>{}> _;\
        throw "you are not supposed to call this method";\
    }
/// Retrieves the type found by ```UTILS_FIND_SELF```.
#define UTILS_FIND_SELF_TYPE typename decltype(get(meta::var<meta::find_self_result>{}))::type
}