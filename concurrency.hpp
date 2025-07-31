#pragma once
#include <semaphore>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <variant>
#include <chrono>
#include "integer.hpp"
#include "type.hpp"

namespace utils {
    constexpr struct unique_resource_sync_t {} unique_resource_sync{};
    namespace detail {
        template <bool Sync, typename SharedMutex>
        struct unique_resource_typedefs { using mutex_type = SharedMutex; };
        template <typename SharedMutex>
        struct unique_resource_typedefs<false, SharedMutex> {};
    }
    /// This class provides a manager of a user-provided resource that is capable of limiting and synchronizing access to it.
    /// Access to the underlying resource is done via "permits" that are issued by the class.
    /// @tparam Sync: adds an internal mutex such that modifications via permit is synchronized
    /// (this should not be set to true if ```T``` is inherently synchronized, such as atomics)
    /// @tparam Semaphore: a type that follows the interface of ```std::counting_semaphore```
    /// @tparam SharedMutex: a type that satisfies <i>SharedMutex</i> (can be any type if ```Sync``` is false)
    /// @remark In the context of this class, ```read-only access``` is synonymous with const reference,
    /// while ```read/write access``` is synonymous with non-const reference (to the underlying resource).
    template <
        typename T, bool Sync = false,
        typename Semaphore = std::counting_semaphore<>, typename SharedMutex = std::shared_mutex>
    class unique_resource : public detail::unique_resource_typedefs<Sync, SharedMutex> {
    public:
        using value_type = T;
        using semaphore_type = Semaphore;
        static constexpr bool sync = Sync;
    private:
        static constexpr struct permit_key : key<unique_resource> {} key{};
    public:
        class permit;
        /// A fancy pointer that holds a lock throughout its lifetime.
        template <typename Lock, type_qualifiers Q = type_qualifiers::none>
        class permit_ptr : public stale_class {
        private:
            apply_qualifiers_t<value_type&, Q> base_;
            Lock lock_;
            explicit constexpr permit_ptr(unique_resource& owner) : base_(owner.base_), lock_(owner.mutex_) {}
            friend permit;
        public:
            constexpr auto operator->() const noexcept { return &base_; }
            constexpr auto& operator*() const noexcept { return base_; }
        };
        class permit : public stale_class {
        private:
            unique_resource& owner_;
        public:
            // a private constructor with friend doesn't work because it is also constructed through std::optional
            constexpr permit(permit_key, unique_resource* owner) : owner_(*owner) {}
            /// Provides read-only access to the resource.
            /// @remark If ```sync``` is true, a fancy pointer (```permit_ptr```) is returned.
            constexpr auto read() const noexcept {
                if constexpr (sync) return permit_ptr<std::shared_lock<SharedMutex>, type_qualifiers::c>(owner_);
                else return &std::as_const(owner_.base_);
            }
            /// Provides read-write access to the resource.
            /// @remark If ```sync``` is true, a fancy pointer (```permit_ptr```) is returned.
            constexpr auto write() const noexcept {
                if constexpr (sync) return permit_ptr<std::scoped_lock<SharedMutex>>(owner_);
                else return &owner_.base_;
            }
            /// Equivalent to ```read```.
            constexpr auto operator->() const noexcept {
                return read();
            }
            /// Returns a const reference to the resource. Only available if ```sync`` is false.
            constexpr const value_type& operator*() const noexcept requires(!sync) {
                return owner_.base_;
            }
            constexpr ~permit() { owner_.sem_.release(); }
        };
    private:
        value_type base_;
        [[no_unique_address]] semaphore_type sem_;
        [[no_unique_address]] std::conditional_t<sync, std::shared_mutex, std::monostate> mutex_;

        using permit_opt = std::optional<permit>;
        constexpr permit_opt issue(bool success) noexcept {
            return success ? permit_opt(std::in_place, key, this) : permit_opt(std::nullopt);
        }
    public:
        constexpr unique_resource(unique_resource_sync_t, integer_alias::ptrdiff_t quota, T&& base)
        requires (std::is_move_constructible_v<T>) :
            base_(std::forward<T>(base)), sem_(+quota) {}
        template <typename... Args>
        requires (std::is_constructible_v<T, Args...>)
        explicit(sizeof...(Args) == 0)
        constexpr unique_resource(integer_alias::ptrdiff_t quota, Args&&... args) :
            base_(std::forward<Args>(args)...), sem_(+quota) {}
        /// Get read access to the resource. Only available if ```sync``` is false.
        constexpr const T& operator*() const noexcept requires(!sync) { return base_; }
        /// Get read/write access to the resource, bypassing semaphore and mutex.
        /// @warning Dangerous operation.
        constexpr T& raw_access() noexcept { return base_; }
        /// Blocks until a permit is issued.
        permit acquire() {
            sem_.acquire();
            return {key, this};
        }
        /// Check if a permit is instantly available.
        /// If yes, returns the issued permit, otherwise returns a ```std::nullopt```.
        /// No blocking occurs nevertheless.
        permit_opt try_acquire() noexcept {
            return issue(sem_.try_acquire());
        }
        /// Wait for ```timeout``` for a permit, returns a permit once it is issued.
        /// If a permit is still not available, returns a ```std::nullopt```.
        template <typename Rep, typename Period>
        permit_opt try_acquire_for(const std::chrono::duration<Rep, Period>& timeout) {
            return issue(sem_.try_acquire_for(timeout));
        }
        /// Wait until ```ddl``` for a permit, returns a permit once it is issued.
        /// If a permit is still not available, returns a ```std::nullopt```.
        template <typename Clock, typename Duration>
        permit_opt try_acquire_until(const std::chrono::time_point<Clock, Duration>& ddl) {
            return issue(sem_.try_acquire_until(ddl));
        }
    };
    template <typename T>
    unique_resource(unique_resource_sync_t, integer_alias::ptrdiff_t, T&&)
    -> unique_resource<std::remove_cvref_t<T>, true>;

    /// A semaphore that does not do any counting. Used for disabling the semaphore in ```unique_resource```.
    struct trivial_semaphore : stale_class {
        explicit constexpr trivial_semaphore(integer_alias::ptrdiff_t) noexcept {}
        static constexpr void release() noexcept {}
        static constexpr void release(integer_alias::ptrdiff_t) noexcept {}
        static constexpr void acquire() noexcept {}
        static constexpr bool try_acquire() noexcept { return true; }
        template <typename Rep, typename Period>
        static constexpr bool try_acquire_for(const std::chrono::duration<Rep, Period>&) noexcept { return true; }
        template <typename Clock, typename Duration>
        static constexpr bool try_acquire_until(const std::chrono::time_point<Clock, Duration>&) noexcept { return true; }
        static constexpr integer_alias::ptrdiff_t max() noexcept { return PTRDIFF_MAX; }
    };
}