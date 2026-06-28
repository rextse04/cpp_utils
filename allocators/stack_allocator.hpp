#pragma once
#include <cstddef>
#include <memory_resource>
#include <memory>
#include "allocator_base.hpp"

namespace utils::pmr {
    namespace detail {
        struct stack_allocator_control {
            using void_pointer = void*;
            using size_type = std::size_t;
            stack_allocator_control* prev;
            size_type offset; // 0 if memory has been deallocated
        };
    }
    template <typename T>
    class stack_allocator : public allocator_base<T, detail::stack_allocator_control> {
        using control_type = detail::stack_allocator_control;
        using base = allocator_base<T, control_type>;
    public:
        using typename base::pointer;
        using typename base::void_pointer;
        using typename base::size_type;
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
    private:
        control_type* front_;

        template <typename U>
        friend class stack_allocator;
    public:
        stack_allocator(void_pointer buf, size_type space) {
            if (space > PTRDIFF_MAX || !std::align(alignof(control_type), sizeof(control_type), buf, space)) {
                throw std::bad_alloc();
            }
            front_ = new(buf) control_type;
            *front_ = {front_, space};
        }
        stack_allocator(std::pmr::memory_resource& mr, size_type space) {
            if (space > PTRDIFF_MAX || space < sizeof(control_type)) throw std::bad_alloc();
            front_ = new(mr.allocate(space, alignof(control_type))) control_type;
            *front_ = {front_, space};
        }
        template <typename U>
        stack_allocator(const stack_allocator<U>& other) noexcept : front_(other.front_) {}
        bool operator==(const stack_allocator& other) const noexcept { return front_ == other.front_; }

        void release(std::pmr::memory_resource& mr, size_type space) noexcept {
            mr.deallocate(front_, space, alignof(control_type));
        }

        pointer allocate(size_type n, std::align_val_t alignment = std::align_val_t(alignof(T))) const {
            control_type* const back = front_->prev;
            control_type sentinel = {back, back->offset - sizeof(control_type)};
            void_pointer obj_ptr = back + 1;
            const size_type bytes = sizeof(T) * n;
            if (!std::align(static_cast<std::size_t>(alignment), bytes, obj_ptr, sentinel.offset)) {
                throw std::bad_alloc();
            }
            void_pointer sentinel_ptr = obj_ptr + bytes;
            sentinel.offset -= bytes;
            if (!std::align(alignof(control_type), sizeof(control_type), sentinel_ptr, sentinel.offset)) {
                throw std::bad_alloc();
            }
            back->offset = static_cast<const std::byte*>(obj_ptr) - reinterpret_cast<const std::byte*>(back);
            front_->prev = new(sentinel_ptr) control_type(sentinel);
            return std::start_lifetime_as_array<T>(obj_ptr, n);
        }
        void deallocate(pointer p, size_type n, std::align_val_t = std::align_val_t(alignof(T))) const noexcept {
            void_pointer next = p + n;
            size_type max_space = SIZE_MAX;
            std::align(alignof(control_type), sizeof(control_type), next, max_space);
            if (next == front_->prev) {
                do { front_->prev = front_->prev->prev; }
                while (front_->prev != front_ && front_->prev->prev->offset == 0);
                front_->prev->offset = std::launder(reinterpret_cast<const control_type*>(next))->offset +
                    (reinterpret_cast<const std::byte*>(next) - reinterpret_cast<const std::byte*>(front_->prev));
            } else {
                std::launder(reinterpret_cast<control_type*>(next))->prev->offset = 0;
            }
        }
        constexpr static size_type max_size() noexcept { return PTRDIFF_MAX / sizeof(control_type); }
    };
}
