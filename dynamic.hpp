#pragma once
#include <memory>
#include <utility>
#include <cstddef>
#include <atomic>
#include <cstring>
#include <functional>
#include "functional.hpp"
#include "type.hpp"
#include "meta.hpp"
#include "self.hpp"
#include "integer.hpp"
#include "swap.hpp"
#include "memory.hpp"

/**
 * @file
 *
 * This file provides utilities for Rust-like dynamic dispatch,
 * as a partial replacement for virtual methods in standard C++.
 * The major difference between this system and that in the language is that
 * rather than embedding a pointer to vtable in each object, it is stored in each pointer to an object.
 * The advantages of this approach are that it reduces the footprint of "dynamic objects"
 * and one level of indirection on each dynamic dispatch.
 *
 * Important concepts and remarks:
 * - An <b>Interface</b> is any class type that defines what its <b>implementers</b> must implement.
 * Under the hood, the interface is directly embedded in its associated vtables,
 * and so there are few limits on what the class type can be.
 * However, practically, you would probably want an interface to be an aggregate for the provided macros to be usable.
 *
 * - When a class ```T``` implements one or more interfaces (by using the UTILS_DYN_BEGIN and UTILS_DYN_END macros),
 * you are defining a static method ```vtable()``` that returns a ```vtable``` object stored in static duration storage.
 * The ```vtable``` struct inherits from ```ivtable<I>``` for each ```I``` in ```Is```,
 * which in turn is a struct that inherits from ```I``` and contains an extra ```dtor```.
 * The macro and the constructor of ```vtable``` automatically initializes
 * the ```dtor``` fields with the destructor of ```T```,
 * the ```size``` fields with ```sizeof(T)```,
 * and the ```arr_deleter``` fields with ```delete T[]```.
 * An ```fptr``` or ```dptr``` stores a type-erased pointer and extra pointer(s) to appropriate ```ivtable```s,
 * which is how they know which methods and destructors to call.
 *
 * Functionality toggle macros:
 * - UTILS_DYN_NO_ARR: Disables array support in ```dptr``` if present.
 * Reduces the size of each ```ivtable``` by ```sizeof(size_t) + sizeof(void*)```.
 */
#define UTILS_DYN_NO_ARR_ERR_MSG "array support for dptr disabled [UTILS_DYN_NO_ARR]"

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

    namespace detail {
        template <typename I, typename... Is>
        struct implemented : std::disjunction<std::is_convertible<const Is*, const I*>...> {};
        template <typename I, typename... Is>
        struct implemented<I, std::tuple<Is...>> : implemented<I, Is...> {};
    }
    template <typename I>
    concept interface = std::is_class_v<I>;
    template <typename T, typename I>
    concept implemented = requires(T t) {
        requires interface<I>;
        requires tagged<T, struct implements_tag>;
        requires detail::implemented<I, typename T::interfaces>::value;
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
        /// Returns an l-value reference to the corresponding vtable entry,
        /// unless when the entry is a non-static ```dyn_method```,
        /// in which case it returns a proxy with the first argument substituted by the pointed-to object.
        template <typename Dyn>
        constexpr decltype(auto) bind_dyn(auto optr, const Dyn& dyn) noexcept {
            if constexpr (tagged<Dyn, dyn_method_tag>) {
                if constexpr (Dyn::non_static) return bound_dyn_method(typename Dyn::obj_ptr_type(optr), +dyn);
                else return dyn;
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
        using tag = struct fptr_tag;
        using element_type = T;
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
        /// @copydoc detail::bind_dyn
        template <typename I, typename U>
        requires (implemented<T, I>)
        constexpr decltype(auto) operator[](U I::* mptr) noexcept {
            return detail::bind_dyn(obj_, vtable_->*mptr);
        }
        constexpr T& operator[](integer_alias::size_t idx) noexcept { return obj_[idx]; }
        constexpr T* operator+(integer_like auto offset) noexcept { return obj_ + to_fundamental(offset); }
        constexpr T* operator-(integer_like auto offset) noexcept { return obj_ - to_fundamental(offset); }
        constexpr explicit operator bool() const noexcept { return obj_ != nullptr; }
        template <typename U>
        requires (std::is_convertible_v<T*, U>)
        constexpr operator U() const noexcept { return obj_; }
    };

    template <interface I>
    struct ivtable : I {
        /// For any implementer ```T``` of ```I``` and any (possibly cv-qualified) instance ```t``` of ```T```,
        /// let ```D``` be ```ivtable<I>(t.vtable()).dtor```.
        /// Then ```D((void*)&t)``` must perform the equivalent of ```t.~T()```.
        void (* const dtor)(void*) noexcept;
#ifndef UTILS_DYN_NO_ARR
        /// ```size == sizeof(T)``` must be true.
        const integer_alias::size_t size;
        /// Let ```arr``` be a dynamically allocated array of ```T```,
        /// and ```DA``` be ```ivtable<I>(t.vtable()).arr_deleter```.
        /// Then ```DA((void*)arr)``` must perform the equivalent of ```delete[] arr```.
        void (* const arr_deleter)(void*) noexcept;
#endif
    };
    template <interface... Is>
    struct vtable : ivtable<Is>... {
        constexpr vtable(Is... interfaces, void (*dtor)(void*) noexcept
#ifdef UTILS_DYN_NO_ARR
            ) noexcept : ivtable<Is>(interfaces, dtor)... {}
#else
            , integer_alias::size_t size, void (*arr_deleter)(void*) noexcept) noexcept :
            ivtable<Is>(interfaces, dtor, size, arr_deleter)... {}
#endif
        constexpr vtable(const vtable& other, void (*dtor)(void*) noexcept
#ifdef UTILS_DYN_NO_ARR
            ) noexcept : ivtable<Is>(other, dtor)... {}
#else
            , integer_alias::size_t size, void (*arr_deleter)(void*) noexcept) noexcept :
            ivtable<Is>(other, dtor, size, arr_deleter)... {}
#endif
    };

    /// Denotes an implementer of each of ```Is```.
    /// A child class ```T``` (of ```implements```) is valid iff:
    /// 1. The inheritance is public.
    /// 2. For a (possibly cv-qualified) instance ```t``` of ```T```,
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

    /// Hides boilerplate for setting up the ```vtable()``` method as specified in ```implements```.
    /// It finds the current class by using the UTILS_FIND_SELF macro, so the rules of the macro apply.
    /// Pairs with ```UTILS_DYN_END```.
#define UTILS_DYN UTILS_FIND_SELF\
    constexpr static const vtable_type& vtable() {\
        static constexpr vtable_type vtable(
    /// Pairs with ```UTILS_DYN```. Automatically sets up ```dtor```.
#ifdef UTILS_DYN_NO_ARR
#define UTILS_DYN_END ,\
            [](void* ptr) noexcept { std::destroy_at(static_cast<UTILS_FIND_SELF_TYPE*>(ptr)); }\
        );\
        return vtable;\
    }
#else
#define UTILS_DYN_END ,\
            [](void* ptr) noexcept { std::destroy_at(static_cast<UTILS_FIND_SELF_TYPE*>(ptr)); },\
            sizeof(UTILS_FIND_SELF_TYPE),\
            [](void* arr) noexcept { delete[] static_cast<UTILS_FIND_SELF_TYPE*>(arr);}\
        );\
        return vtable;\
    }
#endif

    /// @brief Denotes a deleter as opposed to an interface.
    /// Used in the template parameters of smart pointers.
    struct deleter_tag;
    /// Semantic requirements for ```T```:\n
    /// Let ```t``` be an instance of ```T``` and ```dptr``` be a valid dynamic pointer to ```t```.
    /// Then ```t(dptr)``` must be well-formed and well-defined, and perform the following:
    /// 1. If ```T::destroying_delete``` is true, it must first destroy ```t```;
    /// otherwise, it must not use ```t``` in any way.
    /// 2. After that, it must deallocate ```ptr```.
    /// It is guaranteed that std::tuple_size<decltype(ivtables)> is at least 1.
    /// @note The requirements are different from the deleter in ```std::unique_ptr```,
    /// where it also has to destroy the underlying object, which is optional here.
    template <typename T>
    concept deleter = is_tagged_v<deleter_tag, T> && std::is_destructible_v<T>;
    /// A convenience wrapper class that
    /// 1. provides ```deleter_tag```,
    /// 2. generates constructors that accepts a reference to an incomplete ```dptr``` in the first argument from
    /// existing constructors that do not require it, and
    /// 3. generates ```assign``` functions that accepts a reference to the subject ```dptr``` from
    /// existing assignment operators.
    template <typename D>
    struct deleter_wrap : D {
        using tag = deleter_tag;
        using D::D;

        template <tagged<struct dptr_tag> DPtr, typename... Args>
        requires (std::is_constructible_v<D, Args...>)
        constexpr deleter_wrap([[maybe_unused]] const DPtr& dptr, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<D, Args...>) : D(std::forward<Args>(args)...) {}

        template <tagged<dptr_tag> DPtr, typename Other>
        requires (std::is_assignable_v<D, Other>)
        constexpr void assign([[maybe_unused]] const DPtr& dptr, Other&& other)
        noexcept(std::is_nothrow_assignable_v<D, Other>) {
            D::operator=(std::forward<Other>(other));
        }
    };
    namespace detail {
        struct default_deleter_base {
            static constexpr bool destroying_delete = true;
            static constexpr void operator()(const tagged<dptr_tag> auto& dptr) noexcept {
                const auto ptr = const_cast<void*>(dptr.get());
                [[assume(ptr != nullptr)]];
                std::get<0>(dptr.get_ivtables())->dtor(ptr);
                operator delete(ptr);
            }
        };
        struct default_array_deleter_base {
            static constexpr bool destroying_delete = true;
            static constexpr void operator()(const tagged<dptr_tag> auto& dptr) noexcept {
#ifdef UTILS_DYN_NO_ARR
                static_assert(false, UTILS_DYN_NO_ARR_ERR_MSG);
#else
                const auto ptr = const_cast<void*>(dptr.get());
                [[assume(ptr != nullptr)]];
                std::get<0>(dptr.get_ivtables())->arr_deleter(ptr);
#endif
            }
        };
    }
    using default_deleter = deleter_wrap<detail::default_deleter_base>;
    using default_array_deleter = deleter_wrap<detail::default_array_deleter_base>;
    using disabled_deleter = deleter_wrap<std::monostate>;

    enum class ownership_category { borrowed, unique, shared };
    inline constexpr struct detach_deleter_t {} detach_deleter;
    namespace detail {
        template <typename I>
        concept dptr_first = interface<std::remove_all_extents_t<std::remove_cvref_t<I>>>;

        template <typename... Is>
        struct ivtable_pointers { using type = std::tuple<const ivtable<std::remove_cv_t<Is>>*...>; };
        template <typename... Is>
        struct ivtable_pointers<std::tuple<Is...>> : ivtable_pointers<Is...> {};

        template <typename T, typename U>
        concept dptr_compatible = requires {
            requires std::is_convertible_v<T*, typename U::base_type>;
            requires meta::reduce_v<std::conjunction, meta::map_t<
                meta::bind_back<has_implemented, std::remove_cv_t<T>>::template trait, typename U::interfaces>>;
        };

        template <typename Is, typename... Vs>
        struct superset_of : std::conjunction<implemented<Vs, Is>...> {};
        template <typename Is, typename... Vs>
        struct superset_of<Is, std::tuple<Vs...>> : superset_of<Is, Vs...> {};
    }
    /// @brief A non-owning, type-erased pointer that allows dynamic dispatch on interfaces.
    /// @tparam I: First interface.
    /// 1. The cv-qualification of ```I``` determines that of ```base_type```.
    /// 2. The value-category of ```I``` determines what categories of ```dptr``` the class will accept
    /// (in its copy constructor and copy assignment operator):
    /// if ```I``` is an l-value reference, the pointer is "unique", only accepts rvalue-references,
    /// and calls ```destroy_and_delete()``` upon move;
    /// if ```I``` is an r-value reference, the pointer is "shared" and calls ```destroy_and_delete()``` upon copy/move;
    /// otherwise there are no restrictions or special behavior.
    /// 3. If ```I``` is an array type, ```ptr_``` is assumed to point to an array.
    /// The addition operator is enabled, and the default deleter becomes ```default_array_deleter```.
    /// @tparam Is: (Optional) additional interfaces. Unlike ```I```, all types in ```Is``` must be plain.
    /// If the last type ```D``` of ```Is``` is an instantiation of ```deleter```,
    /// ```D::type``` replaces the default deleter;
    /// if ```D``` is ```disabled_deleter```, ```destroy()``` is disabled.
    /// @remark If ```I``` is an array type, the deleter must be a "destroying delete".
    /// This is because there is no way for the class to know how many objects to destroy in the array.
    template <detail::dptr_first I, interface... Is>
    class dptr {
    public:
        using default_interface = std::remove_all_extents_t<std::remove_cvref_t<I>>;
    private:
        using LastI = meta::at_t<std::tuple<void, Is...>, -1>; // the void is for when ```Is``` is empty
        using AllIs = std::tuple<default_interface, Is...>;
    public:
        using tag = dptr_tag;
        using element_type = void; /// type erased
        using base_type = follow_t<I, void*>;
        using enum ownership_category;
        static constexpr ownership_category ownership =
            std::is_lvalue_reference_v<I> ? unique :
            std::is_rvalue_reference_v<I> ? shared : borrowed;
        static constexpr bool is_array = std::rank_v<std::remove_cvref_t<I>> > 0,
        custom_deleter = deleter<LastI>;
        using interfaces = std::conditional_t<custom_deleter, meta::slice<AllIs, 0, -1>, std::type_identity<AllIs>>::type;
        using ivtable_pointer_types = detail::ivtable_pointers<interfaces>::type;
        using deleter_type = std::conditional_t<custom_deleter, LastI,
            std::conditional_t<is_array, default_array_deleter, default_deleter>>;
    private:
        using Args = std::tuple<I, Is...>;
        template <typename Deleter, bool Custom, typename T = void>
        struct rebind_deleter_tmpl;
        template <typename Deleter, typename T>
        struct rebind_deleter_tmpl<Deleter, true, T> {
            using type = meta::apply_t<dptr, meta::concat_t<meta::slice_t<Args, 0, -1>, std::tuple<Deleter>>>;
        };
        template <typename Deleter, typename T>
        struct rebind_deleter_tmpl<Deleter, false, T> {
            using type = dptr<I, Is..., Deleter>;
        };
        template <typename T>
        struct rebind_deleter_tmpl<void, true, T> {
            using type = meta::apply_t<dptr, meta::slice_t<Args, 0, -1>>;
        };
        template <typename T>
        struct rebind_deleter_tmpl<void, false, T> {
            using type = dptr;
        };
        template <ownership_category Ownership, typename T = void>
        struct change_ownership_tmpl;
        template <typename T>
        struct change_ownership_tmpl<borrowed, T> {
            using type = dptr<std::remove_reference_t<I>, Is...>;
        };
        template <typename T>
        struct change_ownership_tmpl<unique, T> {
            using type = dptr<std::remove_reference_t<I>&, Is...>;
        };
        template <typename T>
        struct change_ownership_tmpl<shared, T> {
            using type = dptr<std::remove_reference_t<I>&&, Is...>;
        };
    public:
        template <deleter Deleter>
        using rebind_deleter_type = rebind_deleter_tmpl<Deleter, custom_deleter>::type;
        using remove_deleter_type = rebind_deleter_tmpl<void, custom_deleter>::type;
        template <ownership_category Ownership>
        using change_ownership_type = change_ownership_tmpl<Ownership>::type;
    protected:
        base_type ptr_ = nullptr;
        ivtable_pointer_types ivtables_;
        [[no_unique_address]] deleter_type deleter_;

        template <typename... Args>
        constexpr dptr(base_type ptr, const ivtable_pointer_types& ivtables, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, Args...>) :
            ptr_(ptr), ivtables_(ivtables), deleter_(std::as_const(*this), std::forward<Args>(args)...) {}
        template <std::size_t Idx = 0>
        constexpr void fill_ivtables(ivtable_pointer_types& ivtables, auto* ptr) noexcept {
            if constexpr (Idx < std::tuple_size_v<interfaces>) {
                std::get<Idx>(ivtables) = ptr;
                fill_ivtables<Idx + 1>(ivtables, ptr);
            }
        }
        constexpr ivtable_pointer_types extract_ivtables(auto* ptr) noexcept {
            ivtable_pointer_types ivtables;
            fill_ivtables(ivtables, ptr);
            return ivtables;
        }
        template <std::size_t Idx = 0>
        constexpr void copy_ivtables_to(ivtable_pointer_types& ivtables, const auto& from) noexcept {
            if constexpr (Idx < std::tuple_size_v<interfaces>) {
                std::get<Idx>(ivtables) = std::get<std::tuple_element_t<Idx, ivtable_pointer_types>>(from);
                copy_ivtables_to<Idx + 1>(ivtables, from);
            }
        }
        constexpr ivtable_pointer_types copy_ivtables(const auto& from) noexcept {
            ivtable_pointer_types ivtables;
            copy_ivtables_to(ivtables, from);
            return ivtables;
        }
        constexpr void quick_reset() noexcept { ptr_ = nullptr; }

        template <detail::dptr_first V, interface... Vs>
        friend class dptr;
    public:
        /** \defgroup ```dptr``` constructors
         * @param args: arguments forwarded to ```deleter_``` succeeding a const reference to dptr
         * (i.e. deleter_ is constructed with (const dptr&, std::forward(args)...))
         * @{
         */
        /// Initializes ```ptr_``` with ```nullptr```.
        template <typename... Args>
        requires (std::is_constructible_v<deleter_type, const dptr&, Args...>)
        constexpr dptr(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, Args...>) :
            deleter_(std::as_const(*this), std::forward<Args>(args)...) {}
        /// Initializes ```ptr_``` with ```nullptr```.
        template <typename... Args>
        requires (std::is_constructible_v<deleter_type, const dptr&, Args...>)
        constexpr dptr(std::nullptr_t, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, Args...>) :
            deleter_(std::as_const(*this), std::forward<Args>(args)...) {}
        /// Initializes ```ptr_``` with ```ptr```.
        /// Then fills ```ivtables_``` with pointers to the required ```ivtable```s from ```ptr->vtable()```.
        template <detail::dptr_compatible<dptr> T, typename... Args>
        requires (std::is_constructible_v<deleter_type, const dptr&, Args...>)
        explicit constexpr dptr(T* ptr, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, Args...>) :
            ptr_(ptr), ivtables_(extract_ivtables(std::addressof(ptr->vtable()))),
            deleter_(std::as_const(*this), std::forward<Args>(args)...) {}
        /// Same as the default copy constructor.
        constexpr dptr(const dptr& other)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, const deleter_type&>)
        requires(ownership != unique && std::is_constructible_v<deleter_type, const dptr&, const deleter_type&>) :
            ptr_(other.ptr_), ivtables_(other.ivtables_), deleter_(std::as_const(*this), other.deleter_) {}
        /// Same as the default move constructor, but also sets ```other.ptr_``` to ```nullptr```.
        constexpr dptr(dptr&& other)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, deleter_type&&>)
        requires (std::is_constructible_v<deleter_type, const dptr&, deleter_type&&>) :
            ptr_(other.ptr_), ivtables_(other.ivtables_), deleter_(std::as_const(*this), std::move(other.deleter_)) {
            other.quick_reset();
        }
        /// Construction from a ```dptr``` with a stricter interface.
        /// Copies ```ptr_``` and the required subset of ```ivtables_``` from ```other```.
        /// After that, moves/copies deleter from ```other``` to ```*this```.
        /// If ```other``` is moved from, ```other.ptr_``` is set to ```nullptr```.
        template <
            tagged<dptr_tag> T, typename DT = std::remove_cvref_t<T>,
            typename D = follow_t<T, typename DT::deleter_type>>
        requires (
            (DT::ownership != unique || !std::is_lvalue_reference_v<T>) &&
            (DT::ownership == borrowed || !std::is_const_v<T>) &&
            detail::superset_of<typename DT::interfaces, interfaces>::value &&
            std::is_convertible_v<typename DT::base_type, base_type> &&
            std::is_constructible_v<deleter_type, const dptr&, D>)
        constexpr dptr(T&& other)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, D>) :
            ptr_(other.ptr_), ivtables_(copy_ivtables(other.ivtables_)),
            deleter_(std::as_const(*this), std::forward<D>(other.deleter_)) {
            if constexpr (DT::ownership != borrowed && !std::is_lvalue_reference_v<T>) other.quick_reset();
        }
        /// Construction from a ```dptr``` with a stricter interface, while constructing a separate deleter.
        /// Copies ```ptr_``` and the required subset of ```ivtables_``` from ```other```.
        /// If ```other``` is moved from, ```other.ptr_``` is set to ```nullptr```.
        template <tagged<dptr_tag> T, typename DT = std::remove_cvref_t<T>, typename... Args>
        requires (
            (DT::ownership != unique || !std::is_lvalue_reference_v<T>) &&
            (DT::ownership == borrowed || !std::is_const_v<T>) &&
            detail::superset_of<typename DT::interfaces, interfaces>::value &&
            std::is_convertible_v<typename DT::base_type, base_type> &&
            std::is_constructible_v<deleter_type, const dptr&, Args...>)
        constexpr dptr(detach_deleter_t, T&& other, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<deleter_type, const dptr&, Args...>) :
            ptr_(other.ptr_), ivtables_(copy_ivtables(other.ivtables_)),
            deleter_(std::as_const(*this), std::forward<Args>(args)...) {
            if constexpr (DT::ownership != borrowed && !std::is_lvalue_reference_v<T>) other.quick_reset();
        }
        /**@}*/

        /** \defgroup ```dptr``` destructors
         * Calls ```destroy_and_delete()``` if ```ownership``` is not ```borrowed``` and ```ptr_``` is not null.
         * @{
         */
        constexpr ~dptr() noexcept requires (ownership == borrowed) = default;
        constexpr ~dptr() noexcept requires (ownership != borrowed) {
            if (ptr_) destroy_and_delete();
        }
        /**@}*/

        /// Copy assignment operator. Not available if ```ownership``` is ```unique```.
        /// When ```ownership``` is not ```borrowed```,
        /// calls ```destroy_and_delete()``` beforehand if ```ptr_``` is not ```nullptr```.
        constexpr dptr& operator=(const dptr& other)
        noexcept(noexcept(deleter_.assign(std::as_const(*this), other.deleter_)))
        requires requires {
            requires ownership != unique;
            { deleter_.assign(std::as_const(*this), other.deleter_) };
        } {
            if (ownership != borrowed && ptr_ != nullptr) destroy_and_delete();
            ptr_ = other.ptr_;
            ivtables_ = other.ivtables_;
            deleter_.assign(std::as_const(*this), other.deleter_);
            return *this;
        }
        /// Move assignment operator.
        /// When ```ownership``` is not borrowed,
        /// calls ```destroy_and_delete()``` beforehand and sets ```other.ptr_``` to ```nullptr``` afterwards.
        constexpr dptr& operator=(dptr&& other)
        noexcept(noexcept(deleter_.assign(std::as_const(*this), std::move(other.deleter_))))
        requires requires {
            { deleter_.assign(std::as_const(*this), std::move(other.deleter_)) };
        } {
            if (ownership != borrowed && ptr_ != nullptr) destroy_and_delete();
            ptr_ = other.ptr_;
            ivtables_ = other.ivtables_;
            deleter_.assign(std::as_const(*this), std::move(other.deleter_));
            other.quick_reset();
            return *this;
        }
        /// If the pointer is not borrowed and ```ptr_``` is not ```nullptr```, calls ```destroy_and_delete()```.
        /// Then, sets ```ptr_``` to ```ptr```,
        /// and fills ```ivtables_``` with pointers to the required ```ivtable```s from ```ptr->vtable()```.
        constexpr dptr& operator=(detail::dptr_compatible<dptr> auto* ptr) noexcept {
            if (ownership != borrowed && ptr_ != nullptr) destroy_and_delete();
            ptr_ = ptr;
            fill_ivtables(ivtables_, std::addressof(ptr->vtable()));
            return *this;
        }
        /// If the pointer is not borrowed and ```ptr_``` is not ```nullptr```, calls ```destroy_and_delete()```.
        /// Then, copies ```ptr_``` and the required subset of ```ivtables_``` from ```other```,
        /// and copies/moves the deleter from ```other```.
        /// If the pointer is not borrowed and ```other``` is moved from, ```other.ptr_``` is set to ```nullptr```.
        template <tagged<dptr_tag> T, typename DT = std::remove_cvref_t<T>>
        constexpr dptr& operator=(T&& other)
        noexcept(noexcept(deleter_.assign(std::as_const(*this), std::forward_like<T>(other.deleter_))))
        requires requires {
            requires DT::ownership != unique || !std::is_lvalue_reference_v<T>;
            requires DT::ownership == borrowed || !std::is_const_v<T>;
            requires detail::superset_of<typename DT::interfaces, interfaces>::value;
            requires std::is_convertible_v<typename DT::base_type, base_type>;
            requires std::is_same_v<deleter_type, typename DT::deleter_type>;
            { deleter_.assign(std::as_const(*this), std::forward_like<T>(other.deleter_)) };
        } {
            if (ownership != borrowed && ptr_ != nullptr) destroy_and_delete();
            ptr_ = other.ptr_;
            copy_ivtables_to(ivtables_, other.ivtables_);
            deleter_.assign(std::as_const(*this), std::forward_like<T>(other.deleter_));
            if constexpr (DT::ownership != borrowed && !std::is_lvalue_reference_v<T>) other.quick_reset();
            return *this;
        }

        constexpr bool operator==(const dptr& other) const noexcept { return ptr_ == other.ptr_; }
        constexpr auto operator<=>(const dptr& other) const noexcept { return ptr_ <=> other.ptr_; }
        constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
        /// Returns the pointer.
        constexpr base_type get() const noexcept { return ptr_; }
        /// Returns a copy of ```ivtables_```.
        constexpr ivtable_pointer_types get_ivtables() const noexcept { return ivtables_; }
        /// Returns a reference to the deleter.
        /// @{
        constexpr deleter_type& get_deleter() noexcept { return deleter_; }
        constexpr const deleter_type& get_deleter() const noexcept { return deleter_; }
        /// @}
        /// @copydoc detail::bind_dyn
        template <interface V, typename T>
        requires (detail::implemented<V, interfaces>::value)
        constexpr decltype(auto) operator[](T V::* mptr) const noexcept {
            return detail::bind_dyn(ptr_, std::get<const ivtable<V>*>(ivtables_)->*mptr);
        }
        /// Pointer arithmetic. Only enabled if the underlying object is an array (i.e. if ```is_array``` is true).
        /// @note Let ```T``` be the type of the underlying object.
        /// UB occurs if performing ```(T*)ptr_ + idx``` leads to UB.
        constexpr auto operator+(integer_alias::size_t idx) const noexcept
        requires (is_array) {
#ifdef UTILS_DYN_NO_ARR
            static_assert(false, UTILS_DYN_NO_ARR_ERR_MSG);
#else
            using result = rebind_deleter_type<disabled_deleter>::template change_ownership_type<borrowed>;
            using byte_t = follow_t<base_type, std::byte*>;
            const integer_alias::size_t offset = std::get<0>(ivtables_)->size * idx;
            return result{static_cast<byte_t>(ptr_) + offset, ivtables_};
#endif
        }
        constexpr void swap(dptr& other)
        noexcept (std::is_nothrow_swappable_v<deleter_type>)
        requires (std::is_swappable_v<deleter_type>) {
            std::swap(ptr_, other.ptr_);
            std::swap(ivtables_, other.ivtables_);
            std::swap(deleter_, other.deleter_);
        }
        /// Destroy the underlying object (but does not call deleter on ```ptr_```).
        /// UB if ```ptr_ == nullptr```.
        /// Using the destroyed object (which occurs if you call a non-static ```dyn_method```) afterwards is UB.
        /// @remark Dangerous operation, as destroying an object twice is UB.
        /// If your ```dptr``` is owning (e.g. a ```unique_dptr```),
        /// you must construct another object (of the same type) in place before the lifetime of the ```dptr``` ends.
        /// @remark If ```is_array``` is true,
        /// as the class does not know how many elements there are in the underlying array, this method is disabled.
        /// If you wish to destroy the first element, get a non-array ```dptr``` by calling ```operator+(0)```.
        constexpr void destroy() const noexcept requires (!is_array) {
            std::get<0>(ivtables_)->dtor(const_cast<void*>(ptr_));
        }
        /// Destroy the underlying object and delete ```ptr_``` using ```deleter_```, before setting it to ```nullptr```.
        /// UB if ```ptr_ == nullptr```.
        /// Usual UB rules apply for deleted pointers.
        /// In particular, all pointers pointing at the original ```ptr_``` become dangling.
        /// @{
        constexpr void destroy_and_delete() noexcept
        requires (!is_array && !std::is_same_v<deleter_type, disabled_deleter> && !deleter_type::destroying_delete) {
            destroy();
            deleter_(*this);
            ptr_ = nullptr;
        }
        constexpr void destroy_and_delete() noexcept
        requires (deleter_type::destroying_delete) {
            deleter_(*this);
            ptr_ = nullptr;
        }
        /// @}
        /// Releases ownership of ```ptr_``` and returns it.
        /// @note This only sets ```ptr_``` to ```nullptr``` and does not call ```destroy()```,
        /// even if the pointer is owning.
        constexpr base_type release() noexcept {
            base_type ptr = ptr_;
            ptr_ = nullptr;
            return ptr;
        }
        /// Equivalent to ```operator=(ptr)```.
        template <detail::dptr_compatible<dptr> T>
        constexpr void reset(T* ptr) noexcept { operator=(ptr); }
        /// Equivalent to ```operator=(nullptr)```.
        constexpr void reset(std::nullptr_t = nullptr) noexcept { operator=(nullptr); }
        /// Returns an equivalent ```dptr``` with the deleter replaced by```deleter```.
        template <deleter Deleter>
        constexpr rebind_deleter_type<Deleter> rebind_deleter(auto&&... args) const
        noexcept(std::is_nothrow_constructible_v<Deleter, const rebind_deleter_type<Deleter>&, decltype(args)...>)
        requires (std::is_constructible_v<Deleter, const rebind_deleter_type<Deleter>&, decltype(args)...>) {
            return {ptr_, ivtables_, std::forward<decltype(args)>(args)...};
        }
        /// Returns an equivalent ```dptr``` with the default deleter.
        constexpr remove_deleter_type remove_deleter() const noexcept { return {ptr_, ivtables_}; }
    };

    /// @brief an owning (RAII) version of ```dptr```
    template <detail::dptr_first I, interface... Is>
    requires (!std::is_reference_v<I>)
    class unique_dptr : public dptr<I&, Is...> {
    private:
        using parent = dptr<I&, Is...>;
    public:
        using parent::parent;
        constexpr unique_dptr(unique_dptr&& other) = default;
        constexpr unique_dptr& operator=(unique_dptr&& other) = default;
    };

    inline constexpr struct alias_with_t{} alias_with{};
    namespace detail {
        class shared_dptr_control_base {
        public:
            std::atomic<long> shared_count = 1,
            weak_count = 1; // +1 for when there is an active shared_ptr
            virtual constexpr void destroy_and_delete() noexcept = 0;
            virtual constexpr void deallocate() noexcept = 0;
        };

        template <typename DPtr, typename Alloc>
        struct shared_dptr_control final : shared_dptr_control_base {
            using allocator_type = std::allocator_traits<Alloc>::template rebind_alloc<shared_dptr_control>;
            using allocator_traits = std::allocator_traits<allocator_type>;
            using allocator_pointer = allocator_traits::pointer;
            DPtr dptr;
            [[no_unique_address]] Alloc alloc;
            [[no_unique_address]] std::conditional_t<std::is_pointer_v<allocator_pointer>,
                sink, allocator_pointer> self_ptr;

            template <typename DPtrT, typename AllocT>
            constexpr shared_dptr_control(DPtrT&& dptr, AllocT&& alloc, allocator_pointer self_ptr) :
                dptr(std::forward<DPtrT>(dptr)), alloc(std::forward<AllocT>(alloc)), self_ptr(self_ptr) {}

            constexpr void destroy_and_delete() noexcept override {
                dptr.destroy_and_delete();
            }
            constexpr void deallocate() noexcept override {
                allocator_type alloc_ = std::move(alloc);
                allocator_traits::destroy(alloc_, this);
                if constexpr (std::is_pointer_v<allocator_pointer>) {
                    allocator_traits::deallocate(alloc_, this, 1);
                } else {
                    allocator_traits::deallocate(alloc_, self_ptr, 1);
                }
            }
        };

        struct shared_dptr_deleter {
            using tag = deleter_tag;
            using default_allocator = std::allocator<std::byte>;
            static constexpr bool destroying_delete = true;

            shared_dptr_control_base* control;

            template <
                tagged<dptr_tag> DPtr, typename D, simple_allocator Alloc = default_allocator,
                deleter BaseDeleter = std::remove_cvref_t<D>,
                typename Control = shared_dptr_control<typename DPtr::template rebind_deleter_type<BaseDeleter>, Alloc>,
                typename ReAlloc = std::allocator_traits<Alloc>::template rebind_alloc<Control>,
                typename ReAllocTraits = std::allocator_traits<ReAlloc>>
            requires (std::is_constructible_v<BaseDeleter, D>)
            constexpr shared_dptr_deleter(const DPtr& dptr, D&& deleter, const Alloc& alloc = Alloc()) {
                if (!dptr) {
                    control = nullptr;
                    return;
                }
                ReAlloc alloc_ = alloc;
                auto control_ptr = ReAllocTraits::allocate(alloc_, 1);
                control = std::to_address(control_ptr);
                ReAllocTraits::construct(alloc_, std::to_address(control_ptr),
                    dptr.template rebind_deleter<BaseDeleter>(std::forward<D>(deleter)), std::move(alloc_), control_ptr);
            }
            template <tagged<dptr_tag> DPtr, typename DefaultDeleter = DPtr::remove_deleter_type::deleter_type>
            constexpr shared_dptr_deleter(const DPtr& dptr) :
                shared_dptr_deleter(dptr, DefaultDeleter(dptr), default_allocator()) {}
            constexpr shared_dptr_deleter(shared_dptr_control_base* control, const auto& expired_handler) {
                if (control) {
                    long shared_count = control->shared_count.load(std::memory_order::acquire);
                    while (shared_count) {
                        if (control->shared_count.compare_exchange_weak(shared_count, shared_count + 1,
                            std::memory_order::acq_rel, std::memory_order::acquire)) {
                            this->control = control;
                            return;
                        }
                    }
                }
                expired_handler();
            }
            constexpr shared_dptr_deleter(const tagged<dptr_tag> auto&, shared_dptr_control_base* control) :
                shared_dptr_deleter(control, []() { throw std::bad_weak_ptr(); }) {}
            constexpr shared_dptr_deleter(const tagged<dptr_tag> auto&,
                alias_with_t, shared_dptr_control_base* control) noexcept :
                shared_dptr_deleter(control, [this]() { this->control = nullptr; }) {}
            constexpr shared_dptr_deleter(const tagged<dptr_tag> auto& dptr, const shared_dptr_deleter& other) noexcept :
                shared_dptr_deleter(dptr, other.control) {}
            constexpr shared_dptr_deleter(const tagged<dptr_tag> auto&, shared_dptr_deleter&& other) noexcept :
                control(other.control) {
                other.control = nullptr;
            }
            constexpr void assign(const tagged<dptr_tag> auto&, const shared_dptr_deleter& other) noexcept {
                if (other.control) other.control->shared_count.fetch_add(1, std::memory_order::relaxed);
                control = other.control;
            }
            constexpr void assign(const tagged<dptr_tag> auto&, shared_dptr_deleter&& other) noexcept {
                control = other.control;
                other.control = nullptr;
            }
            constexpr void operator()(const tagged<dptr_tag> auto&) noexcept {
                if (control && control->shared_count.fetch_sub(1, std::memory_order::acq_rel) == 1) {
                    control->destroy_and_delete();
                    if (control->weak_count.fetch_sub(1, std::memory_order::acq_rel) == 1) {
                        control->deallocate();
                    }
                }
            }
        };

        struct ext_control_dptr {
            constexpr long use_count(this const auto& self) noexcept {
                return self.get_control() ? self.get_control()->shared_count.load(std::memory_order::relaxed) : 0;
            }
            constexpr bool owner_before(this const auto& self,
                const std::derived_from<ext_control_dptr> auto& other) noexcept {
                // note that this is well-defined even if the two pointers are unrelated
                // see "pointer total order" in [defns.order.ptr] and [comparison.general]
                return std::less{}(self.get_control(), other.get_control());
            }
            constexpr integer_alias::size_t owner_hash(this const auto& self) noexcept {
                return std::hash<shared_dptr_control_base*>{}(self.get_control());
            }
            constexpr bool owner_equal(this const auto& self,
                const std::derived_from<ext_control_dptr> auto& other) noexcept {
                return self.get_control() == other.get_control();
            }
        };
    }

    template <detail::dptr_first I, interface... Is>
    requires (!std::is_reference_v<I>)
    class shared_dptr;

    /// @brief Analogous to ```std::weak_ptr```.
    template <typename... Is>
    requires requires { typename shared_dptr<Is...>; }
    class weak_dptr : public detail::ext_control_dptr {
    public:
        using tag = struct weak_dptr_tag;
        using element_type = void;
    private:
        dptr<Is...> dptr_ = nullptr;
        detail::shared_dptr_control_base* control_ = nullptr;

        template <detail::dptr_first V, interface... Vs>
        requires (!std::is_reference_v<V>)
        friend class shared_dptr;

        static constexpr void record_control(detail::shared_dptr_control_base* control) noexcept {
            if (control) control->weak_count.fetch_add(1, std::memory_order::relaxed);
        }
        static constexpr void release_control(detail::shared_dptr_control_base* control) noexcept {
            if (control) control->weak_count.fetch_sub(1, std::memory_order::acq_rel);
        }
        static constexpr struct internal_copy_t {} internal_copy{};
        static constexpr struct internal_move_t {} internal_move{};
        constexpr weak_dptr(internal_copy_t, const auto& other) noexcept :
            dptr_(other.dptr_), control_(other.control_) {
            record_control(control_);
        }
        constexpr weak_dptr(internal_move_t, auto&& other) noexcept :
            dptr_(std::move(other.dptr_)), control_(other.control_) {
            other.dptr_ = nullptr;
            other.control_ = nullptr;
        }
        constexpr weak_dptr& assign(internal_copy_t, const auto& other) noexcept {
            dptr_ = other.dptr_;
            record_control(other.control_);
            release_control(control_);
            control_ = other.control_;
            return *this;
        }
        constexpr weak_dptr& assign(internal_move_t, auto&& other) noexcept {
            release_control(control_);
            dptr_ = std::move(other.dptr_);
            control_ = other.control_;
            other.control_ = nullptr;
            return *this;
        }
    public:
        constexpr weak_dptr() noexcept = default;
        constexpr weak_dptr(const weak_dptr& other) noexcept : weak_dptr(internal_copy, other) {}
        template <typename... Vs>
        requires (std::is_constructible_v<dptr<Is...>, const dptr<Vs...>&>)
        constexpr weak_dptr(const weak_dptr<Vs...>& other) noexcept : weak_dptr(internal_copy, other) {}
        template <typename... Vs>
        requires (std::is_constructible_v<dptr<Is...>, const dptr<Vs...>&>)
        constexpr weak_dptr(const shared_dptr<Vs...>& other) noexcept :
            dptr_(detach_deleter, other), control_(other.get_control()) {
            record_control(control_);
        }
        constexpr weak_dptr(weak_dptr&& other) noexcept : weak_dptr(internal_move, std::move(other)) {}
        template <typename... Vs>
        requires (std::is_constructible_v<dptr<Is...>, dptr<Vs...>&&>)
        constexpr weak_dptr(weak_dptr<Vs...>&& other) noexcept : weak_dptr(internal_move, std::move(other)) {}
        constexpr weak_dptr& operator=(const weak_dptr& other) noexcept { return assign(internal_copy, other); }
        template <typename... Vs>
        requires (std::is_assignable_v<dptr<Is...>, const dptr<Vs...>&>)
        constexpr weak_dptr& operator=(const weak_dptr<Vs...>& other) noexcept { return assign(internal_copy, other); }
        template <typename... Vs>
        requires (std::is_assignable_v<dptr<Is...>, const dptr<Vs...>&>)
        constexpr weak_dptr& operator=(const shared_dptr<Vs...>& other) noexcept {
            dptr_ = other.dptr_.remove_deleter();
            record_control(other.get_control());
            release_control(control_);
            control_ = other.control_;
            return *this;
        }
        constexpr weak_dptr& operator=(weak_dptr&& other) noexcept {
            return assign(internal_move, std::move(other));
        }
        template <typename... Vs>
        requires (std::is_assignable_v<dptr<Is...>, dptr<Vs...>&&>)
        constexpr weak_dptr& operator=(weak_dptr<Vs...>&& other) noexcept {
            return assign(internal_move, std::move(other));
        }
        constexpr ~weak_dptr() noexcept {
            if (control_ && control_->weak_count.fetch_sub(1, std::memory_order::acq_rel) == 1) {
                control_->deallocate();
            }
        }

        constexpr detail::shared_dptr_control_base* get_control() const noexcept { return control_; }
        constexpr bool expired(this const auto& self) noexcept { return self.use_count() == 0; }
        constexpr void reset() noexcept {
            release_control(control_);
            control_ = nullptr;
        }
        constexpr void swap(weak_dptr& other) noexcept {
            std::swap(dptr_, other.dptr_);
            std::swap(control_, other.control_);
        }
        template <typename... Vs>
        requires (std::is_constructible_v<dptr<Vs...>, dptr<Is...>>)
        constexpr shared_dptr<Vs...> lock() const noexcept {
            return expired() ? shared_dptr<Vs...>() : shared_dptr<Vs...>(dptr_);
        }
        constexpr shared_dptr<Is...> lock() const noexcept { return lock<Is...>(); }
    };

    /// @brief An owning ```dptr``` with shared ownership.
    ///
    /// The underlying pointer is deleted when and only when the lifetime of the last owner of the pointer ends.
    template <detail::dptr_first I, interface... Is>
    requires (!std::is_reference_v<I>)
    class shared_dptr : public dptr<I&&, Is..., detail::shared_dptr_deleter>, public detail::ext_control_dptr {
    private:
        using parent = dptr<I&&, Is..., detail::shared_dptr_deleter>;
    public:
        using tag = std::tuple<dptr_tag, struct shared_dptr_tag>;
        using parent::parent;
        constexpr shared_dptr(const shared_dptr&) noexcept = default;
        constexpr shared_dptr(shared_dptr&&) noexcept = default;
        template <typename Other, typename... Args>
        requires (
            tagged<Other, shared_dptr_tag> && !std::is_const_v<Other> &&
            std::is_constructible_v<dptr<I, Is...>, Args...>)
        constexpr shared_dptr(alias_with_t, Other&& other, Args&&... args) noexcept :
            parent(std::forward<Args>(args)..., alias_with, other.get_control()) {
            if constexpr (!std::is_lvalue_reference_v<Other>) other.quick_reset();
        }
        template <typename... Vs>
        constexpr shared_dptr(const weak_dptr<Vs...>& other)
        requires (std::is_constructible_v<dptr<I, Is...>, dptr<Vs...>>) :
            parent(detach_deleter, other.dptr_, other.control_) {}
        constexpr shared_dptr& operator=(const shared_dptr&) noexcept = default;
        constexpr shared_dptr& operator=(shared_dptr&&) noexcept = default;

        constexpr detail::shared_dptr_control_base* get_control() const noexcept {
            return parent::get_deleter().control;
        }
    };
}