#pragma once
#include <atomic>

namespace utils {
#define UTILS_ATOMIC_ALIGN(T) alignas(std::atomic_ref<T>::required_alignment) T
}