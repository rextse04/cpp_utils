#include "self.hpp"

namespace random {
    struct test {
        UTILS_FIND_SELF;
        static_assert(std::is_same_v<UTILS_FIND_SELF_TYPE, test>);
    };
}