#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <memory_resource>
#include "allocator_base.hpp"

namespace utils::pmr {
    namespace detail {
        struct arena_allocator_control {
            using void_pointer = void*;
            using size_type = std::size_t;
            void_pointer data;
            size_type space;
        };
    }
    template <typename T>
    class arena_allocator : public allocator_base<T, detail::arena_allocator_control> {
        using control_type = detail::arena_allocator_control;
        using base = allocator_base<T, control_type>;
    public:
        using typename base::pointer;
        using typename base::void_pointer;
        using typename base::size_type;
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
    private:
        control_type* control_;

        template <typename U>
        friend class arena_allocator;
    public:
        arena_allocator(void_pointer buf, size_type space) {
            if (!std::align(alignof(control_type), sizeof(control_type), buf, space)) {
                throw std::bad_alloc();
            }
            control_ = new(buf) control_type;
            *control_ = {control_ + 1, space - sizeof(control_type)};
        }
        arena_allocator(std::pmr::memory_resource& mr, size_type space) {
            if (space < sizeof(control_type)) throw std::bad_alloc();
            control_ = new(mr.allocate(space, alignof(control_type))) control_type;
            *control_ = {control_ + 1, space - sizeof(control_type)};
        }
        template <typename U>
        arena_allocator(const arena_allocator<U>& other) noexcept : control_(other.control_) {}

        void release(std::pmr::memory_resource& mr, size_type space) noexcept {
            mr.deallocate(control_, space, alignof(control_type));
        }

        pointer allocate(size_type n, std::align_val_t alignment = std::align_val_t(alignof(T))) const {
            const size_type bytes = sizeof(T) * n;
            void_pointer out = std::align(std::size_t(alignment), bytes, control_->data, control_->space);
            if (out == nullptr) throw std::bad_alloc();
            control_->data = static_cast<std::byte*>(control_->data) + bytes;
            control_->space -= bytes;
            return std::start_lifetime_as_array<T>(out, n);
        }
        static void deallocate(pointer, size_type, std::align_val_t = std::align_val_t(alignof(T))) noexcept {}
        template <typename U>
        bool operator==(const arena_allocator<U>& other) const noexcept { return control_ == other.control_; }
    };
}