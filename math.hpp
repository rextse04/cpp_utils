#pragma once
#include <cstddef>

namespace utils {
    /// @brief Calculates @code base@endcode raised to the power of @code Exp@endcode. Supports zero and negative exponents.
    template <std::ptrdiff_t Exp, typename CastT>
    constexpr CastT pow(CastT base) {
        if constexpr (Exp == 0) return CastT(1);
        else if constexpr (Exp > 0) return static_cast<CastT>(base) * pow<Exp-1, CastT>(base);
        else return CastT(1)/ pow<-Exp, CastT>(base);
    }
}