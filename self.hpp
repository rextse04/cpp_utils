#pragma once
#include <type_traits>
#include "stateful_meta.hpp"
#include "exception.hpp"

namespace utils::meta {
/// This macro stores the innermost class @code Self@endcode enclosing the current context into
/// @code meta::const_var<utils_find_self_result_>@endcode (the type of the value being @code std::type_identity<Self>@endcode).
///
/// The program is ill-formed if the macro is not used inside a class declaration,
/// or if a method named @code utils_find_self_@endcode or a member type named @code utils_find_self_result_@endcode is already declared.
#define UTILS_FIND_SELF\
    struct utils_find_self_result_;\
    consteval auto utils_find_self_()\
    -> decltype(utils::meta::define<utils_find_self_result_, std::type_identity<std::remove_cvref_t<decltype(*this)>>{}>{}) {\
        throw utils::compile_error(\
            "You are not supposed to call this method. See the documentation for UTILS_FIND_SELF.");\
    }
/// Retrieves the type found by @code UTILS_FIND_SELF@endcode.
#define UTILS_FIND_SELF_TYPE typename decltype(get(utils::meta::const_var<utils_find_self_result_>{}))::type
}