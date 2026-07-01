#pragma once
#include <type_traits>
#include <cstddef>
#include <memory_resource>
#include <memory>
#include <utility>
#include "memory.hpp"

namespace utils::pmr {
    /// @brief Determines if @code T@endcode can be an underlying allocator for @code allocator_resource@endcode.
    /// @note Semantic guarantees and requirements are outlined in the concept body.
    template <typename T>
    concept resource_allocator = (
        simple_allocator<T> && std::is_same_v<typename T::value_type, std::byte> &&
        requires (T a, std::pmr::memory_resource& mr, std::size_t space, std::size_t bytes, std::align_val_t alignment, std::byte* p) {
            /**
             * Precondition: @code a@endcode is equal to some @code T(mr, space, ...)@endcode.
             * Effect: @code a@endcode deallocates @code space@endcode through @code mr@endcode.
             * Postcondition: @code a@endcode and all equal allocators release management of and
             * do not use the previously allocated upstream memory after release.
             */
            { a.release(mr, space) } noexcept;
            /**
             * Precondition: @code alignment@endcode is a power of 2 and not less than @code alignof(T::value_type)@endcode.
             * Effect: Returns a pointer to a block of memory of size @code bytes@endcode and alignment @code alignment@endcode.
             */
            { a.allocate(bytes, alignment) } -> std::same_as<std::byte*>;
            /**
             * Precondition: @code p@endcode is provided by @code a.allocate(bytes, alignment)@endcode.
             * Effect: @code a@endcode deallocates the memory block pointed to by @code p@endcode.
             */
            { a.deallocate(p, bytes, alignment) };
        });
    /// @brief A @code std::pmr::memory_resource@endcode which reserves specified amount of memory and
    /// allocates it using a given @code resource_allocator@endcode.
    template <resource_allocator Allocator>
    class allocator_resource : public std::pmr::memory_resource, private Allocator {
    public:
        using allocator_type = Allocator;
    private:
        memory_resource* upstream_;
        std::size_t space_;
    public:
        /// @brief Constructs a @code allocator_resource@endcode with a given upstream memory resource and space to reserve.
        ///
        /// Semantic requirement:
        /// @code Allocator@endcode should reserve @code space@endcode amount of memory from @code upstream@endcode.
        template <typename... Args>
        allocator_resource(memory_resource* upstream, std::size_t space, Args&&... args)
        requires (std::is_constructible_v<Allocator, memory_resource&, std::size_t, Args&&...>) :
            Allocator(*upstream, space, std::forward<Args>(args)...), upstream_(upstream), space_(space) {}
        /// @brief Copy constructor is deleted because
        /// @code allocator_resource@endcode has unique and non-transferable ownership of the reserved memory.
        allocator_resource(const allocator_resource&) = delete;
        /// @brief Copy assignment operator is deleted because
        /// @code allocator_resource@endcode has unique and non-transferable ownership of the reserved memory.
        allocator_resource& operator=(const allocator_resource&) = delete;
        /// @brief Destructor. Releases ownership of reserved memory.
        ~allocator_resource() noexcept override { Allocator::release(*upstream_, space_); }
    private:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override {
            return Allocator::allocate(bytes, std::align_val_t(alignment));
        }
        void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
            Allocator::deallocate(static_cast<std::byte*>(ptr), bytes, std::align_val_t(alignment));
        }
        bool do_is_equal(const memory_resource& other) const noexcept override {
            const auto other_ = dynamic_cast<const Allocator*>(&other);
            if (other_ == nullptr) return false;
            return static_cast<const Allocator&>(*this) == *other_;
        }
    };
}