#pragma once
#include "../common.hpp"
#include <span>
#include <cstddef>
#include <ranges>

inline void poison(std::span<std::byte> buf) {
    constexpr unsigned char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
    for (auto chunk : buf | std::views::chunk(4)) {
        for (std::size_t i = 0; i < chunk.size(); ++i) {
            if (i >= chunk.size()) break;
            chunk[i] = static_cast<std::byte>(pattern[i]);
        }
    }
}