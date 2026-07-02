#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <memory_resource>
#include <atomic>
#include "atomic.hpp"
#include "allocator_base.hpp"
#include "memory.hpp"

namespace utils::pmr {
    namespace detail {
        struct arena_allocator_control {
            using void_pointer = void*;
            using size_type = std::size_t;
            UTILS_ATOMIC_ALIGN(void_pointer) data;
            const void_pointer end;
        };
    }
    /// @brief An allocator which does not recycle allocated memory.
    ///
    /// An arena allocator allocates memory from a pre-allocated buffer and does not recycle memory.
    /// (In other words, @code deallocate@endcode is a no-op.)
    /// When the buffer is depleted, it throws at every subsequent allocation request.
    /// It is useful for allocating many small objects with the same lifetime,
    /// as it can reduce the overhead of individual allocations.
    ///
    /// Internally, @code arena_allocator@endcode only stores a pointer to an implementation-defined control structure
    /// living on the buffer, which stores the current allocation position and the end of the buffer.
    /// At construction, the control structure is constructed at the beginning of the (aligned) buffer,
    /// which is updated on every allocation request.
    /// If @code Sync@endcode is true, updates are performed atomically.
    /// Note that there may be gaps in between allocated objects on the buffer due to alignment.
    /// The following is a simple diagram illustrating the allocation strategy:
    /// @image html arena_allocator_internal.svg
    ///
    /// All instances of this class satisfy [<i>BufferAllocator</i>](BufferAllocator.md) and @code resource_allocator@endcode.
    /// Instances with @code Sync@endcode equal to @code true@endcode also satisfy [<i>ConvertibleAllocator</i>](BufferAllocator.md).
    /// Consult their documentation for conditions and effects of member functions.
    /// @tparam T: @code value_type@endcode
    /// @tparam Sync: Whether allocator operations should be synchronized.
    template <typename T, bool Sync = false>
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
        template <typename U>
        struct rebind { using other = arena_allocator<U, Sync>; };
        static constexpr bool synchronized = Sync;
    private:
        control_type* control_;

        template <typename, bool>
        friend class arena_allocator;
    public:
        /// @defgroup pmr::arena_allocator::arena_allocator
        /// @{
        /// @brief Takes ownership of the buffer and uses it for allocations.
        /// @throws std::bad_alloc: If the internal control structure does not fit in the given buffer.
        /// @note The construction can throw even if @code space@endcode is not less than the size of the control structure
        /// depending on the alignment of @code buf@endcode.
        arena_allocator(void_pointer buf, size_type space) {
            if (!std::align(alignof(control_type), sizeof(control_type), buf, space)) {
                throw std::bad_alloc();
            }
            control_ = new(buf)
                control_type(static_cast<std::byte*>(buf) + sizeof(control_type), static_cast<std::byte*>(buf) + space);
        }
        /// @brief Allocates a buffer of size @code space@endcode through @code mr@endcode and uses it for allocations.
        /// @throws std::bad_alloc: If @code space@endcode is insufficient to accommodate the internal control structure.
        arena_allocator(std::pmr::memory_resource& mr, size_type space) {
            if (space < sizeof(control_type)) throw std::bad_alloc();
            const void_pointer buf = mr.allocate(space, alignof(control_type));
            control_ = new(buf)
                control_type(static_cast<std::byte*>(buf) + sizeof(control_type), static_cast<std::byte*>(buf) + space);
        }
        /// @brief Conversion constructor.
        ///
        /// The constructor is explicit if @code synchronized != other.synchronized@endcode.
        /// This is to remind users to exercise caution when converting between synchronized and unsynchronized allocators,
        /// as mixing allocation operations on the same buffer with different synchronization schemes is UB.
        /// @remark Allocators of different types can share the same buffer due to the common control structure.
        template <typename U, bool S>
        explicit(S != Sync) arena_allocator(const arena_allocator<U, S>& other) noexcept : control_(other.control_) {}
        /// @}
        /// @brief Equality comparison operator.
        ///
        /// Two allocators are equal if and only if they share the same buffer.
        bool operator==(const arena_allocator& other) const noexcept { return control_ == other.control_; }
        /// @brief Deallocates the underlying buffer previously allocated through a memory resource.
        void release(std::pmr::memory_resource& mr, size_type space) noexcept {
            mr.deallocate(control_, space, alignof(control_type));
        }
        /// @brief Allocates storage for @code T[n]@endcode with specified alignment from the underlying buffer.
        ///
        /// The behavior is undefined if `alignment` is less than that of `T`.
        pointer allocate(size_type n, std::align_val_t alignment = std::align_val_t(alignof(T))) const {
            const size_type bytes = sizeof(T) * n;
            void_pointer aligned;
            void_pointer data = Sync
                ? std::atomic_ref(control_->data).load(std::memory_order::relaxed)
                : control_->data;
            while (true) {
                aligned = align(static_cast<std::size_t>(alignment), bytes, data, control_->end);
                if (!aligned) throw std::bad_alloc();
                if constexpr (Sync) {
                    if (std::atomic_ref(control_->data).compare_exchange_weak(
                        data, aligned + bytes, std::memory_order::relaxed)) break;
                } else {
                    control_->data = aligned + bytes;
                    break;
                }
            }
            return std::start_lifetime_as_array<T>(aligned, n);
        }
        /// @brief Deallocation is a no-op.
        static void deallocate(pointer, size_type, std::align_val_t = std::align_val_t(alignof(T))) noexcept {}
    };
}