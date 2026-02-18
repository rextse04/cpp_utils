#pragma once
#include <tuple>
#include <type_traits>

namespace utils {
    template <typename... Fs>
    class visitor {
    private:
        std::tuple<Fs...> fs_;

        template <std::size_t Idx = 0, typename... Ts>
        static constexpr std::size_t match() noexcept {
            if constexpr (Idx == sizeof...(Fs)) {
                return -1;
            } else {
                if constexpr (std::is_invocable_v<std::tuple_element_t<Idx, std::tuple<Fs...>>, Ts...>) {
                    return Idx;
                } else {
                    return match<Idx + 1, Ts...>();
                }
            }
        }
    public:
        template <typename... Gs>
        requires (std::is_constructible_v<Fs, Gs> && ...)
        constexpr visitor(Gs&&... gs)
        noexcept((std::is_nothrow_constructible_v<Fs, Gs> && ...)):
            fs_(std::forward<Gs>(gs)...) {}

        template <typename... Ts, std::size_t Match = match<Ts...>()>
        requires (Match != -1)
        constexpr auto operator()(Ts&&... ts) const
        noexcept(noexcept(std::get<Match>(fs_)(std::forward<Ts>(ts)...))) {
            return std::get<Match>(fs_)(std::forward<Ts>(ts)...);
        }
    };

    template <typename... Gs>
    visitor(Gs&&...) -> visitor<std::decay_t<Gs>...>;
}