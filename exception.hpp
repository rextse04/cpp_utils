#pragma once
#include <exception>
#include <string_view>

namespace utils {
    /// @brief Represents an error at compilation due to incorrect use of library constructs.
    class compile_error : public std::exception {
    private:
        std::string_view msg_;
    public:
        explicit consteval compile_error(std::string_view msg) : msg_(msg) {}
        constexpr const char *what() const noexcept override { return msg_.data(); }
    };
}