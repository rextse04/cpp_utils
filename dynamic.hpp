#pragma once
#include "type.hpp"
#include "meta.hpp"

namespace utils {
    struct interface {

    };

    /// This is a thin wrapper around a pointer to a dynamic method.
    /// This is preferred over raw pointers because it forces initialization.
    template <typename F>
    class dyn_method;
    template <typename R, typename... Args>
    class dyn_method<R(Args...)> {
    public:
        using function_type = R(Args...);
    private:
        function_type* func_;
    public:
        constexpr dyn_method(function_type* func) noexcept : func_(func) {}
        constexpr R operator()(Args... args) const noexcept { return func_(static_cast<Args>(args)...); }
    };
    template <typename F>
    requires (std::is_function_v<F>)
    dyn_method(F*) -> dyn_method<F>;

    struct implements_tag;
    /// Fat pointer. Allows dynamic dispatch on (implementation) classes.
    /// In other words, it makes sure the correct dynamic methods are called
    /// even if the stored object is of a derived class of ```T```.
    template <tagged<implements_tag> T>
    class fptr {
    public:
        using value_type = T;
        using vtable_type = std::remove_cvref_t<T>::vtable_type;
    private:
        T* obj_;
        const vtable_type* vtable_;
        constexpr fptr(T* obj, const vtable_type* vtable) noexcept : obj_(obj), vtable_(vtable) {}
        template <tagged<implements_tag> U>
        friend class fptr;
        template <typename... Interfaces>
        friend struct implements;
    public:
        constexpr fptr() noexcept = default;
        constexpr fptr(std::nullptr_t) noexcept : obj_(nullptr), vtable_(nullptr) {}
        template <std::derived_from<T> U>
        constexpr fptr(const fptr<U>& other) noexcept : obj_(other.obj_), vtable_(other.vtable_) {}
        constexpr const vtable_type* operator->() const noexcept { return vtable_; }
        constexpr T& operator*() const noexcept { return *obj_; }
        constexpr explicit operator bool() const noexcept { return obj_ != nullptr; }
        template <typename U>
        requires (std::is_convertible_v<T*, U*>)
        constexpr operator U*() const noexcept { return obj_; }
    };

    /// The child class ```T``` is valid iff for a (possibly cv-qualified) instance ```t``` of ```T```,
    /// ```t.vtable()``` returns an l-value reference to ```const T::vtable_type```.
    /// The reference must remain valid for the entirety of the program,
    /// although modification of the underlying vtable is allowed.
    template <typename... Interfaces>
    struct implements {
        using tag = implements_tag;
        using vtable_type = join<Interfaces...>;
        using interfaces = std::tuple<Interfaces...>;
        template <typename Self>
        constexpr fptr<std::remove_reference_t<Self>> operator&(this Self&& self) noexcept {
            return {std::addressof(self), std::addressof(self.vtable())};
        }
    };
    template <typename Interface, typename T>
    struct has_implemented : std::false_type {};
    template <typename Interface, tagged<implements_tag> T>
    struct has_implemented<Interface, T> : std::conjunction<
        meta::contained_in<typename T::interfaces, Interface>,
        std::bool_constant<requires(T t) { {t.vtable()} -> std::same_as<const typename T::vtable_type&>; }>> {};
    template <typename Interface, typename T>
    constexpr bool has_implemented_v = has_implemented<Interface, T>::value;
    template <typename T, typename Interface>
    concept implemented = has_implemented<Interface, T>::value;
#define UTILS_DYN constexpr static const vtable_type& vtable() {\
    static constexpr vtable_type vtable(
#define UTILS_DYN_END );\
    return vtable;\
    }

    /// An owning (RAII) pointer that allows dynamic dispatch on interfaces.
    template <typename... Interfaces>
    class unique_dptr {
    public:
        using interfaces = std::tuple<Interfaces...>;
        using default_interface = std::tuple_element_t<0, interfaces>;
        using vtable_pointer_types = std::tuple<const Interfaces*...>;
    private:
        void* ptr_ = nullptr;
        vtable_pointer_types vtables_;
        void (*deleter_)(void*) = [](void*) {};
        template <std::size_t Idx = 0>
        constexpr void fill_vtable(auto* ptr) noexcept {
            if constexpr (Idx < sizeof...(Interfaces)) {
                std::get<Idx>(vtables_) = ptr;
                fill_vtable<Idx + 1>(ptr);
            }
        }
    public:
        constexpr unique_dptr() noexcept = default;
        explicit constexpr unique_dptr(std::nullptr_t) noexcept {}
        /// UB if ```ptr->vtable()``` causes UB.
        template <typename T>
        requires ((has_implemented_v<Interfaces, std::remove_cv_t<T>> && ...))
        explicit constexpr unique_dptr(T* ptr) : ptr_(ptr), deleter_([](void* ptr) { delete static_cast<T*>(ptr); }) {
            fill_vtable(std::addressof(ptr->vtable()));
        }
        unique_dptr(const unique_dptr&) = delete;
        constexpr unique_dptr(unique_dptr&&) noexcept = default;
        unique_dptr& operator=(const unique_dptr&) = delete;
        constexpr unique_dptr& operator=(unique_dptr&&) noexcept = default;
        ~unique_dptr() { deleter_(ptr_); }

        template <typename Interface>
        constexpr auto as() const noexcept { return std::get<const Interface*>(vtables_); }
        constexpr auto operator->() const noexcept { return as<default_interface>(); }
        constexpr void* get() const noexcept { return ptr_; }
        constexpr operator void*() const noexcept { return get(); }
        explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

        constexpr void* release() noexcept {
            void* ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }
        /// UB if ```ptr->vtable()``` causes UB.
        template <typename T>
        requires ((has_implemented_v<Interfaces, std::remove_cv_t<T>> && ...))
        constexpr void reset(T* ptr) noexcept {
            [[assume(ptr == nullptr || ptr != ptr_)]];
            deleter_(ptr_);
            ptr_ = ptr;
            fill_vtable(std::addressof(ptr->vtable()));
        }
        constexpr void reset(std::nullptr_t = nullptr) noexcept {
            deleter_(ptr_);
            ptr_ = nullptr;
        }
        constexpr void swap(unique_dptr& other) noexcept {
            std::swap(ptr_, other.ptr_);
            std::swap(vtables_, other.vtables_);
            std::swap(deleter_, other.deleter_);
        }
    };
}