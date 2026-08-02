#pragma once
#include <atomic>

/**
 * @file
 * @brief Utilities for atomics.
 */

namespace utils {
/// @brief A convenience macro to align a data member such that it is suitable for use of `std::atomic_ref`.
#define UTILS_ATOMIC_ALIGN(T) alignas(std::atomic_ref<T>::required_alignment) T
}