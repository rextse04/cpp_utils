#pragma once
#include <memory>
#include "functional.hpp"
#include "type.hpp"
#include "meta.hpp"
#include "self.hpp"

namespace utils {
    /// Thin wrapper around a void pointer that acts like a ```this``` pointer,
    /// which allows the ```dyn_method``` template to specialize for non-static dynamic methods.
    /// Must be used in non-static dynamic methods and as the first parameter.
    template <type_qualifiers Q>
    class basic_obj_ptr {
    public:
        using tag = struct basic_obj_ptr_tag;
        using base_type = apply_qualifiers_t<void*, Q>;
    private:
        base_type ptr_;
    public:
        explicit constexpr basic_obj_ptr(base_type ptr) noexcept : ptr_(ptr) {}
        template <typename T>
        requires ((qualifiers_of_v<T> & Q) == Q)
        explicit constexpr operator T*() const noexcept { return static_cast<T*>(ptr_); }
    };
    using obj_ptr = basic_obj_ptr<type_qualifiers::none>;
    using const_obj_ptr = basic_obj_ptr<type_qualifiers::c>;
    using volatile_obj_ptr = basic_obj_ptr<type_qualifiers::v>;
    using const_volatile_obj_ptr = basic_obj_ptr<type_qualifiers::c | type_qualifiers::v>;

    namespace detail {
        template <typename R, typename... Args>
        class dyn_method_base {
        public:
            using tag = struct dyn_method_tag;
            using function_type = R(Args...);
            static constexpr bool non_static = false;
        private:
            function_type* func_;
        public:
            /// Made a template to allow copy-initialization in interface construction.
            constexpr dyn_method_base(std::convertible_to<function_type*> auto func) noexcept : func_(func) {}
            constexpr R operator()(Args... args) const noexcept { return func_(std::forward<Args>(args)...); }
            constexpr operator function_type*() const noexcept { return func_; }
        };
    }
    /// This is a thin wrapper around a pointer to a dynamic method.
    /// This is preferred over raw pointers because it forces initialization,
    /// and is the only way to declare a non-static dynamic method.
    template <typename F>
    class dyn_method;
    template <typename R, typename... Args>
    class dyn_method<R(Args...)> : public detail::dyn_method_base<R, Args...> {
    public:
        using detail::dyn_method_base<R, Args...>::dyn_method_base;
    };
    template <typename R, tagged<basic_obj_ptr_tag> ObjPtr, typename... Args>
    class dyn_method<R(ObjPtr, Args...)> : public detail::dyn_method_base<R, ObjPtr, Args...> {
    public:
        static constexpr bool non_static = true;
        using obj_ptr_type = ObjPtr;
        using detail::dyn_method_base<R, ObjPtr, Args...>::dyn_method_base;
    };
    template <typename T, typename F = lambda_decay_t<T>>
    dyn_method(T) -> dyn_method<F>;

    template <typename I>
    concept interface = std::is_class_v<I>;
    struct implements_tag;
    template <typename T, typename I>
    concept implemented = requires(T t) {
        requires interface<I>;
        requires tagged<T, implements_tag>;
        requires meta::contained_in_v<typename T::interfaces, I>;
        { t.vtable() } -> std::same_as<const typename T::vtable_type&>;
    };
    template <typename I, typename T>
    struct has_implemented : std::bool_constant<implemented<T, I>> {};
    template <typename I, typename T>
    constexpr bool has_implemented_v = has_implemented<I, T>::value;

    namespace detail {
        template <typename R, typename ObjPtr, typename... Args>
        struct bound_dyn_method {
            ObjPtr obj_ptr;
            R(*func)(ObjPtr, Args...);
            constexpr decltype(auto) operator()(Args... args) const
            noexcept(noexcept(func(obj_ptr, std::declval<Args>()...))){
                return func(obj_ptr, static_cast<Args>(args)...);
            }
        };
        template <typename Dyn>
        constexpr decltype(auto) bind_dyn(auto optr, const Dyn& dyn) noexcept {
            if constexpr (tagged<Dyn, dyn_method_tag>) {
                if constexpr (Dyn::non_static) {
                    return bound_dyn_method(typename Dyn::obj_ptr_type(optr), +dyn);
                } else {
                    return dyn;
                }
            } else {
                return dyn;
            }
        }
    }

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
        template <interface... Is>
        friend struct implements;
    public:
        constexpr fptr() noexcept = default;
        constexpr fptr(std::nullptr_t) noexcept : obj_(nullptr), vtable_(nullptr) {}
        template <std::derived_from<T> U>
        constexpr fptr(const fptr<U>& other) noexcept : obj_(other.obj_), vtable_(other.vtable_) {}
        constexpr T* operator->() const noexcept { return obj_; }
        constexpr T& operator*() const noexcept { return *obj_; }
        template <typename I, typename U>
        requires (implemented<T, I>)
        constexpr decltype(auto) operator[](U I::* mptr) noexcept {
            return detail::bind_dyn(obj_, vtable_->*mptr);
        }
        constexpr explicit operator bool() const noexcept { return obj_ != nullptr; }
        template <typename U>
        requires (std::is_convertible_v<T*, U>)
        constexpr operator U() const noexcept { return obj_; }
    };

    template <interface I>
    struct ivtable : I {
        void (*dtor)(void*);
    };
    template <interface... Is>
    struct vtable : ivtable<Is>... {
        explicit constexpr vtable(Is... interfaces, void (*dtor)(void*)) noexcept :
            ivtable<Is>(interfaces, dtor)... {}
        constexpr vtable(const vtable& other, void (*dtor)(void*)) noexcept :
            ivtable<Is>(other, dtor)... {}
    };

    /// The child class ```T``` is valid iff for a (possibly cv-qualified) instance ```t``` of ```T```,
    /// ```t.vtable()``` returns an l-value reference to ```const T::vtable_type```.
    /// The reference must remain valid for the entirety of the program,
    /// although modification of the underlying vtable is allowed.
    template <interface... Is>
    struct implements {
        using tag = implements_tag;
        using vtable_type = vtable<Is...>;
        using interfaces = std::tuple<Is...>;
        template <typename Self>
        constexpr fptr<std::remove_reference_t<Self>> operator&(this Self&& self) noexcept {
            return {std::addressof(self), std::addressof(self.vtable())};
        }
    };

#define UTILS_DYN UTILS_FIND_SELF\
    constexpr static const vtable_type& vtable() {\
        static constexpr vtable_type vtable(
#define UTILS_DYN_END , [](void* ptr) { std::destroy_at(static_cast<UTILS_FIND_SELF_TYPE*>(ptr)); }\
        );\
        return vtable;\
    }

    namespace detail {
        template <typename T, typename U>
        concept unique_dptr_compatible = requires {
            requires std::convertible_to<T*, typename U::base_type>;
            requires meta::reduce_v<std::conjunction, meta::map_t<
                meta::bind_back<has_implemented, std::remove_cv_t<T>>::template trait, typename U::interfaces>>;
        };
    }
    /// An owning (RAII), type-erased pointer that allows dynamic dispatch on interfaces.
    /// @note The cv-qualification of the first type in ```Is``` determines that of ```base_type```.
    template <interface... Is>
    class unique_dptr {
    public:
        using tag = struct unique_dptr_tag;
        using base_type = follow_t<std::tuple_element_t<0, std::tuple<Is...>>, void*>;
        using interfaces = std::tuple<std::remove_cv_t<Is>...>;
        using default_interface = std::tuple_element_t<0, interfaces>;
        using ivtable_pointer_types = std::tuple<const ivtable<std::remove_cv_t<Is>>*...>;
    private:
        base_type ptr_ = nullptr;
        ivtable_pointer_types ivtables_;
        template <std::size_t Idx = 0>
        constexpr void fill_ivtables(auto* ptr) noexcept {
            if constexpr (Idx < sizeof...(Is)) {
                std::get<Idx>(ivtables_) = ptr;
                fill_ivtables<Idx + 1>(ptr);
            }
        }
        template <std::size_t Idx = 0>
        constexpr void copy_ivtables(auto ivtables) noexcept {
            if constexpr (Idx < sizeof...(Is)) {
                std::get<Idx>(ivtables_) = std::get<std::tuple_element_t<Idx, ivtable_pointer_types>>(ivtables);
                copy_ivtables<Idx + 1>(ivtables);
            }
        }
        constexpr void delete_ptr() {
            if (ptr_ == nullptr) return;
            std::get<0>(ivtables_)->dtor(const_cast<void*>(ptr_));
            operator delete(const_cast<void*>(ptr_));
        }
        template <interface... Vs>
        friend class unique_dptr;
    public:
        constexpr unique_dptr() noexcept = default;
        explicit constexpr unique_dptr(std::nullptr_t) noexcept {}
        /// UB if ```ptr->vtable()``` causes UB.
        template <detail::unique_dptr_compatible<unique_dptr> T>
        explicit constexpr unique_dptr(T* ptr) : ptr_(ptr) { fill_ivtables(std::addressof(ptr->vtable())); }
        unique_dptr(const unique_dptr&) = delete;
        constexpr unique_dptr(unique_dptr&& other) noexcept : ptr_(other.release()), ivtables_(other.ivtables_) {}
        template <tagged<unique_dptr_tag> T>
        requires (meta::strict_subset_of_v<interfaces, typename T::interfaces>)
        constexpr unique_dptr(T&& other) noexcept : ptr_(other.release()) { copy_ivtables(other.ivtables_); }
        unique_dptr& operator=(const unique_dptr&) = delete;
        constexpr unique_dptr& operator=(unique_dptr&& other) noexcept {
            ptr_ = other.ptr_;
            ivtables_ = other.ivtables_;
            other.ptr_ = nullptr;
            return *this;
        }
        template <tagged<unique_dptr_tag> T>
        requires (meta::strict_subset_of_v<interfaces, typename T::interfaces>)
        constexpr unique_dptr& operator=(T&& other) noexcept {
            ptr_ = other.ptr_;
            copy_ivtables(other.ivtables_);
            other.ptr_ = nullptr;
            return *this;
        }
        constexpr ~unique_dptr() { delete_ptr(); }

        template <interface I, typename T>
        requires (meta::contained_in_v<interfaces, I>)
        constexpr decltype(auto) operator[](T I::* mptr) noexcept {
            return detail::bind_dyn(ptr_, std::get<const ivtable<I>*>(ivtables_)->*mptr);
        }
        constexpr base_type get() const noexcept { return ptr_; }
        constexpr operator base_type() const noexcept { return get(); }
        explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

        constexpr base_type release() noexcept {
            base_type ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }
        /// UB if ```ptr``` is not a null pointer and ``ptr == ptr_```.
        template <detail::unique_dptr_compatible<unique_dptr> T>
        constexpr void reset(T* ptr) noexcept {
            [[assume(ptr == nullptr || ptr != ptr_)]];
            delete_ptr();
            ptr_ = ptr;
            fill_ivtables(std::addressof(ptr->vtable()));
        }
        constexpr void reset(std::nullptr_t = nullptr) noexcept {
            delete_ptr();
            ptr_ = nullptr;
        }
        constexpr void swap(unique_dptr& other) noexcept {
            std::swap(ptr_, other.ptr_);
            std::swap(ivtables_, other.ivtables_);
        }
    };
}