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
    /// @tparam Sync: adds an internal mutex such that modifications via permit is synchronized
    /// (this should not be set to true if ```T``` is inherently synchronized, such as atomics)
    /// @tparam Semaphore: a type that follows the interface of ```std::counting_semaphore```
    /// @tparam SharedMutex: a type that satisfies <i>SharedMutex</i> (can be any type if ```Sync``` is false)
    template <
        typename T, bool Sync = false,
        typename Semaphore = std::counting_semaphore<>, typename SharedMutex = std::shared_mutex>
    class unique_resource : public detail::unique_resource_typedefs<Sync, SharedMutex> {
    public:
        using value_type = T;
        using semaphore_type = Semaphore;
        static constexpr bool sync = Sync;
    private:
        static constexpr struct key_t {} key{};
        template <typename Lock, type_qualifiers Q = type_qualifiers::none>
        class permit_ptr : public stale_class {
        private:
            apply_qualifiers_t<value_type&, Q> base_;
            Lock lock_;
        public:
            explicit constexpr permit_ptr(unique_resource& owner) : base_(owner.base_), lock_(owner.mutex_) {}
            constexpr auto operator->() const noexcept { return &base_; }
            constexpr auto& operator*() const noexcept { return base_; }
        };
        class permit : public stale_class {
        private:
            unique_resource& owner_;
        public:
            constexpr permit(key_t, unique_resource* owner) : owner_(*owner) {}
            constexpr auto read() const noexcept {
                if constexpr (sync) return permit_ptr<std::shared_lock<SharedMutex>, type_qualifiers::c>(owner_);
                else return &std::as_const(owner_.base_);
            }
            constexpr auto write() const noexcept {
                if constexpr (sync) return permit_ptr<std::scoped_lock<SharedMutex>>(owner_);
                else return &owner_.base_;
            }
            constexpr auto operator->() const noexcept {
                return read();
            }
            constexpr const value_type& operator*() const noexcept requires(!sync) {
                return owner_.base_;
            }
            constexpr ~permit() { owner_.sem_.release(); }
        };

        value_type base_;
        semaphore_type sem_;
        [[no_unique_address]] std::conditional_t<sync, std::shared_mutex, std::monostate> mutex_;

        using permit_opt = std::optional<permit>;
        constexpr permit_opt issue(bool success) noexcept {
            return success ? permit_opt(std::in_place, key, this) : permit_opt(std::nullopt);
        }
    public:
        template <typename U>
        constexpr unique_resource(integer_alias::ptrdiff_t quota, U&& base) :
            base_(std::forward<U>(base)), sem_(+quota) {}
        template <typename U>
        constexpr unique_resource(unique_resource_sync_t, integer_alias::ptrdiff_t quota, U&& base) :
            base_(std::forward<U>(base)), sem_(+quota) {}
        template <typename... Args>
        requires (std::is_constructible_v<T, Args...>)
        explicit constexpr unique_resource(integer_alias::ptrdiff_t quota, Args&&... args) :
            base_(std::forward<Args>(args)...), sem_(+quota) {}
        constexpr const T& operator*() const noexcept requires(!sync) { return base_; }
        permit acquire() {
            sem_.acquire();
            return {key, this};
        }
        permit_opt try_acquire() noexcept {
            return issue(sem_.try_acquire());
        }
        template <typename Rep, typename Period>
        permit_opt try_acquire_for(const std::chrono::duration<Rep, Period>& timeout) {
            return issue(sem_.try_acquire_for(timeout));
        }
        template <typename Clock, typename Duration>
        permit_opt try_acquire_until(const std::chrono::time_point<Clock, Duration>& ddl) {
            return issue(sem_.try_acquire_until(ddl));
        }
    };
    template <typename U>
    unique_resource(integer_alias::ptrdiff_t, U&&) -> unique_resource<std::remove_cvref_t<U>>;
    template <typename U>
    unique_resource(unique_resource_sync_t, integer_alias::ptrdiff_t, U&&)
    -> unique_resource<std::remove_cvref_t<U>, true>;
}