#pragma once
#include <cstddef>
#include <memory_resource>
#include <memory>
#include "atomic.hpp"
#include "allocator_base.hpp"
#include "memory.hpp"

namespace utils::pmr {
    namespace detail {
        struct stack_allocator_control {
            using void_pointer = void*;
            using size_type = std::size_t;
            UTILS_ATOMIC_ALIGN(void_pointer) prev;
            // For the front node: pointer to end of buffer
            // For other nodes: pointer to corresponding object, or nullptr if deallocated
            UTILS_ATOMIC_ALIGN(void_pointer) obj;
        };
    }
    /// @brief An allocator which allocates memory from a pre-allocated buffer in a lazy-stack-like manner.
    ///
    /// A stack allocator allocates memory from a pre-allocated buffer in a lazy-stack-like manner.
    /// Specifically, a memory chunk is only recycled if it is the most recent allocation; otherwise, it is simply marked "removed".
    /// When deallocation is requested, the allocator recursively frees memory chunks in a LIFO manner until it finds
    /// a chunk that is still in use.
    ///
    /// Internally, an implementation-defined control structure is constructed at the beginning of the (aligned) buffer,
    /// and one is attached to every allocated memory chunk.
    /// The control at the beginning stores a pointer to the control of most recent chunk and a pointer to the end of the buffer.
    /// For others, each one stores a pointer to the structure attached to the previous chunk,
    /// and a pointer to the object itself (or nullptr if the chunk is marked "removed").
    /// Note that there may be gaps between a control structure and its associated object due to alignment.
    /// The following diagram is a simple diagram illustrating the allocation strategy:
    /// @image html stack_allocator_internal.svg
    ///
    /// All instances of this class satisfy [<i>BufferAllocator</i>](BufferAllocator.md) and @code resource_allocator@endcode.
    /// Instances with @code Sync@endcode equal to @code true@endcode also satisfy [<i>ConvertibleAllocator</i>](BufferAllocator.md).
    /// Consult their documentation for conditions and effects of member functions.
    /// @tparam T: @code value_type@endcode
    /// @tparam Sync: Whether allocator operations should be synchronized.
    template <typename T, bool Sync = false>
    class stack_allocator : public allocator_base<T, detail::stack_allocator_control> {
        using control_type = detail::stack_allocator_control;
        using base = allocator_base<T, control_type>;
    public:
        using typename base::pointer;
        using typename base::void_pointer;
        using typename base::size_type;
        template <typename U>
        struct rebind { using other = stack_allocator<U, Sync>; };
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
        static constexpr bool synchronized = Sync;
    private:
        control_type* front_;

        template <typename, bool>
        friend class stack_allocator;
    public:
        /// @defgroup pmr::stack_allocator<T, Sync>::stack_allocator
        /// @{
        /// @brief Takes ownership of the buffer and uses it for allocations.
        /// @throws std::bad_alloc: If the internal control structure does not fit in the given buffer.
        /// @note The construction can throw even if @code space@endcode is not less than the size of the control structure
        /// depending on the alignment of @code buf@endcode.
        stack_allocator(void_pointer buf, size_type space) {
            void_pointer back = buf;
            if (!std::align(alignof(control_type), sizeof(control_type), back, space)) {
                throw std::bad_alloc();
            }
            front_ = new(back) control_type;
            *front_ = {front_, static_cast<std::byte*>(buf) + space};
        }
        /// @brief Allocates a buffer of size @code space@endcode through @code mr@endcode and uses it for allocations.
        /// @throws std::bad_alloc: If @code space@endcode is insufficient to accommodate the internal control structure.
        stack_allocator(std::pmr::memory_resource& mr, size_type space) {
            if (space < sizeof(control_type)) throw std::bad_alloc();
            const void_pointer buf = mr.allocate(space, alignof(control_type));
            front_ = new(buf) control_type;
            *front_ = {front_, static_cast<std::byte*>(buf) + space};
        }
        /// @brief Conversion constructor.
        ///
        /// The constructor is explicit if @code synchronized != other.synchronized@endcode.
        /// This is to remind users to exercise caution when converting between synchronized and unsynchronized allocators,
        /// as mixing allocation operations on the same buffer with different synchronization schemes is UB.
        /// @remark Allocators of different types can share the same buffer due to the common control structure.
        template <typename U, bool S>
        explicit(S != Sync) stack_allocator(const stack_allocator<U, S>& other) noexcept : front_(other.front_) {}
        /// @}
        /// @brief Equality comparison operator.
        ///
        /// Two allocators are equal if and only if they share the same buffer.
        bool operator==(const stack_allocator& other) const noexcept { return front_ == other.front_; }
        /// @brief Deallocates the underlying buffer previously allocated through a memory resource.
        void release(std::pmr::memory_resource& mr, size_type space) noexcept {
            mr.deallocate(front_, space, alignof(control_type));
        }
        /// @brief Allocates storage for @code T[n]@endcode with specified alignment from the underlying buffer.
        ///
        /// The behavior is undefined if `alignment` is less than that of `T`.
        pointer allocate(size_type n, std::align_val_t alignment = std::align_val_t(alignof(T))) const {
            control_type sentinel;
            while (true) {
                void_pointer back = Sync
                    ? std::atomic_ref(front_->prev).load(std::memory_order::relaxed)
                    : front_->prev;
                sentinel.prev = back;
                sentinel.obj = static_cast<std::byte*>(back) + sizeof(control_type);
                const size_type bytes = sizeof(T) * n;
                sentinel.obj = align(static_cast<std::size_t>(alignment), bytes, sentinel.obj, front_->obj);
                if (!sentinel.obj) throw std::bad_alloc();
                void_pointer sentinel_ptr = align(alignof(control_type), sizeof(control_type),
                    static_cast<std::byte*>(sentinel.obj) + bytes, front_->obj);
                if (!sentinel_ptr) throw std::bad_alloc();
                if constexpr (Sync) {
                    if (std::atomic_ref(front_->prev).compare_exchange_weak(
                        back, sentinel_ptr, std::memory_order::relaxed)) {
                        new(sentinel_ptr) control_type(sentinel);
                        break;
                    }
                } else {
                    front_->prev = new(sentinel_ptr) control_type(sentinel);
                    break;
                }
            }
            return std::start_lifetime_as_array<T>(sentinel.obj, n);
        }
        /// @brief Deallocates previously allocated memory and recycles it in a LIFO manner.
        ///
        /// Formally, let @f$(M_1,M_2,...,M_k)@f$ be the memory chunks previously allocated by any allocator equal to `*this`
        /// which have not been <i>recycled</i>, in the order of allocation.
        /// A memory chunk is <i>recycled</i> if `allocate` can use its storage.
        /// As noted in the documentation of the class, deallocation in `stack_allocator` does not always recycle memory
        /// but may simply mark it as <i>removed</i>.
        /// Let $D_i$ indicate if $M_i$ has been removed for @f$i\in[k]@f$.
        /// A call to `deallocate` first finds @f$t@f$ such that `p` points to @f$M_t@f$, marks the chunk as removed,
        /// then recycles all chunks in the set @f$\{M_i\mid D_i,...,D_k\}@f$.
        /// @remark The alignment argument is unused.
        /// It is defined to deallocate a pointer obtained through aligned allocation without passing the required alignment.
        void deallocate(pointer p, size_type n, std::align_val_t = std::align_val_t(alignof(T))) const noexcept {
            control_type* const next = std::launder(static_cast<control_type*>(
                align(alignof(control_type), sizeof(control_type), p + n, front_->obj)));
            void_pointer back = Sync
                ? std::atomic_ref(front_->prev).load(std::memory_order::relaxed)
                : front_->prev;
            while (true) {
                if (next == back) {
                    control_type* new_back = std::launder(static_cast<control_type*>(back));
                    while (true) {
                        new_back = std::launder(static_cast<control_type*>(new_back->prev));
                        if (new_back == front_) break;
                        const void_pointer obj = Sync
                            ? std::atomic_ref(new_back->obj).load(std::memory_order::relaxed)
                            : new_back->obj;
                        if (obj) break;
                    }
                    if constexpr (Sync) {
                        if (std::atomic_ref(front_->prev).compare_exchange_weak(
                            back, new_back, std::memory_order::relaxed)) break;
                    } else {
                        front_->prev = new_back;
                        break;
                    }
                } else {
                    if constexpr (Sync) {
                        std::atomic_ref(next->obj).store(nullptr, std::memory_order::relaxed);
                    } else {
                        next->obj = nullptr;
                    }
                    break;
                }
            }
        }
    };
}