#pragma once
#include <memory>
#include <utility>
#include <cstddef>
#include "functional.hpp"
#include "type.hpp"
#include "meta.hpp"
#include "self.hpp"
#include "integer.hpp"
#include "swap.hpp"

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
 * - <b>Strongly</b> prefer composition over inheritance.
 * Dynamic pointers (```dptr```s) only care about what interfaces a class implements,
 * but not the inheritance structure between interfaces. As an example:
 * @code
 * struct Animal { // interface
 *     dyn_method<void(obj_ptr)> eats;
 * };
 * struct SeaAnimal : Animal { // bad interface!
 *     dyn_method<void(obj_ptr)> swims;
 * };
 * class Fish : public implements<SeaAnimal> {...};
 * unique_dptr<SeaAnimal> animal1 = new Fish; // ok
 * unique_dptr<Animal> animal2 = new Fish; // ill-formed: Fish does not implement Animal!
 * @endcode
 * Instead, you should do this:
 * @code
 * struct Animal { // interface
 *     dyn_method<void(obj_ptr)> eats;
 * };
 * struct SeaAnimal { // good interface
 *     dyn_method<void(obj_ptr)> swims;
 * };
 * class Fish : public implements<Animal, SeaAnimal> {...};
 * unique_dptr<SeaAnimal> animal1 = new Fish; // ok
 * unique_dptr<Animal, SeaAnimal> animal2 = new Fish; // still ok
 * unique_dptr<Animal animal3 = new Fish; // still ok
 * @endcode
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
        /// @copydoc detail::bind_dyn
        template <typename I, typename U>
        requires (implemented<T, I>)
        constexpr decltype(auto) operator[](U I::* mptr) noexcept {
            return detail::bind_dyn(obj_, vtable_->*mptr);
        }
        constexpr T& operator[](integer_alias::size_t idx) noexcept { return obj_[idx]; }
        constexpr T* operator+(integer_alias::ptrdiff_t offset) noexcept { return obj_ + offset; }
        constexpr T* operator-(integer_alias::ptrdiff_t offset) noexcept { return obj_ - offset; }
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
        void (* const dtor)(void*);
#ifndef UTILS_DYN_NO_ARR
        /// ```size == sizeof(T)``` must be true.
        const integer_alias::size_t size;
        /// Let ```arr``` be a dynamically allocated array of ```T```,
        /// and ```DA``` be ```ivtable<I>(t.vtable()).arr_deleter```.
        /// Then ```DA((void*)arr)``` must perform the equivalent of ```delete[] arr```.
        void (* const arr_deleter)(void*);
#endif
    };
    template <interface... Is>
    struct vtable : ivtable<Is>... {
        constexpr vtable(Is... interfaces, void (*dtor)(void*)
#ifdef UTILS_DYN_NO_ARR
            ) noexcept : ivtable<Is>(interfaces, dtor)... {}
#else
            , integer_alias::size_t size, void (*arr_deleter)(void*)) noexcept :
            ivtable<Is>(interfaces, dtor, size, arr_deleter)... {}
#endif
        constexpr vtable(const vtable& other, void (*dtor)(void*)
#ifdef UTILS_DYN_NO_ARR
            ) noexcept : ivtable<Is>(other, dtor)... {}
#else
            , integer_alias::size_t size, void (*arr_deleter)(void*)) noexcept :
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
            [](void* ptr) { std::destroy_at(static_cast<UTILS_FIND_SELF_TYPE*>(ptr)); }\
        );\
        return vtable;\
    }
#else
#define UTILS_DYN_END ,\
            [](void* ptr) { std::destroy_at(static_cast<UTILS_FIND_SELF_TYPE*>(ptr)); },\
            sizeof(UTILS_FIND_SELF_TYPE),\
            [](void* arr) { delete[] static_cast<UTILS_FIND_SELF_TYPE*>(arr);}\
        );\
        return vtable;\
    }
#endif

    /// Denotes a deleter as opposed to an interface.
    /// Used in the template parameters of smart pointers.
    ///
    /// Semantic requirements for ```T```:\n
    /// Let ```t``` be an instance of ```T```, ``ptr``` be a void pointer to ```t```,
    /// ```ivtables``` be a tuple of const pointers to ivtables associated with ```t```.
    /// Then ```t(ptr, vtable)``` must be well-formed and well-defined, and perform the following:
    /// 1. If ```T::destroying_delete``` is true, it must first destroy ```t```;
    /// otherwise, it must not use ```t``` in any way.
    /// 2. After that, it must deallocate ```ptr```.
    /// @note The requirements are different from the deleter in ```std::unique_ptr```,
    /// where it also has to destroy the underlying object, which is optional here.
    template <typename T>
    requires requires(T t, void* ptr) {
        requires std::is_destructible_v<T>;
        t(ptr);
    }
    struct deleter {
        using tag = struct deleter_tag;
        using type = T;
    };
    struct default_deleter {
        using tag = deleter_tag;
        using type = default_deleter;
        static constexpr void operator()(void* ptr, [[maybe_unused]] const auto& ivtables) noexcept {
            operator delete(ptr);
        }
    };
    struct default_array_deleter {
        using tag = deleter_tag;
        using type = default_array_deleter;
        static constexpr bool destroying_delete = true;
        static constexpr void operator()(void* ptr, const auto& ivtables) noexcept {
#ifdef UTILS_DYN_NO_ARR
            static_assert(false, UTILS_DYN_NO_ARR_ERR_MSG);
#else
            std::get<0>(ivtables)->arr_deleter(ptr);
#endif
        }
    };
    struct disabled_deleter {
        using tag = deleter_tag;
        using type = disabled_deleter;
    };

    namespace detail {
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
    }
    /// A non-owning, type-erased pointer that allows dynamic dispatch on interfaces.
    /// @tparam I: First interface.
    /// 1. The cv-qualification of ```I``` determines that of ```base_type```.
    /// 2. The value-category of ```I``` determines what categories of ```dptr``` the class will accept
    /// (in its copy constructor and copy assignment operator):
    /// if ```I``` is an l-value reference, the class is "unique" and will only accept rvalue-references,
    /// otherwise there are no restrictions.
    /// 3. If ```I``` is an array type, ```ptr_``` is assumed to point to an array.
    /// The addition operator is enabled, and the default deleter becomes ```default_array_deleter```.
    /// @tparam Is: (Optional) additional interfaces. Unlike ```I```, all types in ```Is``` must be plain.
    /// If the last type ```D``` of ```Is``` is an instantiation of ```deleter```,
    /// ```D::type``` replaces the default deleter;
    /// if ```D``` is ```disabled_deleter```, ```destroy()``` is disabled.
    /// @remark If ```I``` is an array type, the deleter must be a "destroying delete".
    /// This is because there is no way for the class to know how many objects to destroy in the array.
    template <typename I, interface... Is>
    requires (interface<std::remove_all_extents_t<std::remove_cvref_t<I>>>)
    class dptr {
    public:
        using default_interface = std::remove_all_extents_t<std::remove_cvref_t<I>>;
    private:
        using LastI = meta::at_t<std::tuple<void, Is...>, -1>; // the void is for when ```Is``` is empty
        using AllIs = std::tuple<default_interface, Is...>;
    public:
        using tag = struct dptr_tag;
        using element_type = void; /// type erased
        using base_type = follow_t<I, void*>;
        static constexpr bool is_unique = std::is_lvalue_reference_v<I>,
        is_array = std::rank_v<std::remove_cvref_t<I>> > 0,
        custom_deleter = tagged<LastI, deleter_tag>;
        using interfaces = std::conditional_t<custom_deleter, meta::slice_t<AllIs, 0, -1>, AllIs>;
        using ivtable_pointer_types = detail::ivtable_pointers<interfaces>::type;
        using deleter_type = std::conditional_t<custom_deleter, LastI,
            std::conditional_t<is_array, default_array_deleter, default_deleter>>::type;
    protected:
        base_type ptr_ = nullptr;
        ivtable_pointer_types ivtables_;
        [[no_unique_address]] deleter_type deleter_;

        constexpr dptr(base_type ptr, ivtable_pointer_types ivtables) noexcept :
            ptr_(ptr), ivtables_(ivtables) {}
        template <std::size_t Idx = 0>
        constexpr void fill_ivtables(auto* ptr) noexcept {
            if constexpr (Idx < std::tuple_size_v<interfaces>) {
                std::get<Idx>(ivtables_) = ptr;
                fill_ivtables<Idx + 1>(ptr);
            }
        }
        template <std::size_t Idx = 0>
        constexpr void copy_ivtables(const auto& ivtables) noexcept {
            if constexpr (Idx < std::tuple_size_v<interfaces>) {
                std::get<Idx>(ivtables_) = std::get<std::tuple_element_t<Idx, ivtable_pointer_types>>(ivtables);
                copy_ivtables<Idx + 1>(ivtables);
            }
        }
        template <typename V, interface... Vs>
        requires (interface<std::remove_all_extents_t<std::remove_cvref_t<V>>>)
        friend class dptr;
    public:
        /** \defgroup ```dptr``` constructors
         * @param args: arguments forwarded to ```deleter_```
         * @{
         */
        /// Initializes ```ptr_``` with ```nullptr```.
        template <typename... Args>
        requires (std::is_constructible_v<deleter_type, Args...>)
        constexpr dptr(Args&&... args) noexcept : deleter_(std::forward<Args>(args)...) {}
        /// Initializes ```ptr_``` with ```nullptr```.
        template <typename... Args>
        requires (std::is_constructible_v<deleter_type, Args...>)
        constexpr dptr(std::nullptr_t, Args&&... args) noexcept : deleter_(std::forward<Args>(args)...) {}
        /// Initializes ```ptr_``` with ```ptr```.
        /// Then fills ```ivtables_``` with pointers to the required ```ivtable```s from ```ptr->vtable()```.
        template <detail::dptr_compatible<dptr> T, typename... Args>
        requires (std::is_constructible_v<deleter_type, Args...>)
        explicit constexpr dptr(T* ptr, Args&&... args) : ptr_(ptr), deleter_(std::forward<Args>(args)...) {
            fill_ivtables(std::addressof(ptr->vtable()));
        }
        /// Copies ```ptr_``` and ```ivtables_``` from ```other```.
        constexpr dptr(const dptr&) noexcept requires(!is_unique) = default;
        /// Same as the copy constructor, but also sets ```other.ptr_``` to ```nullptr```.
        constexpr dptr(dptr&& other) noexcept : dptr(other) { // call copy constructor
            other.ptr_ = nullptr;
        }
        /// Construction from a ```dptr``` with a stricter interface.
        /// Copies ```ptr_``` and the required subset of ```ivtables_``` from ```other```.
        /// If ```other``` is moved from, ```other.ptr_``` is set to ```nullptr```.
        template <tagged<dptr_tag> T, typename... Args>
        requires (
            (!is_unique || !std::is_lvalue_reference_v<T>) &&
            meta::subset_of_v<interfaces, typename std::remove_cvref_t<T>::interfaces> &&
            std::is_convertible_v<typename std::remove_cvref_t<T>::base_type, base_type> &&
            std::is_constructible_v<deleter_type, Args...>)
        constexpr dptr(T&& other, Args&&... args) noexcept : ptr_(other.ptr_), deleter_(std::forward<Args>(args)...) {
            copy_ivtables(other.ivtables_);
            if constexpr (!std::is_lvalue_reference_v<T>) other.ptr_ = nullptr;
        }
        /**@}*/

        /// Copies ```ptr_``` and ```ivtables_``` from ```other```.
        constexpr dptr& operator=(const dptr&) noexcept = default;
        /// Same as the copy assignment operator, but also sets ```other.ptr_``` to ```nullptr```.
        constexpr dptr& operator=(dptr&& other) noexcept {
            operator=(other); // call copy assignment operator
            other.ptr_ = nullptr;
            return *this;
        }
        /// Initializes ```ptr_``` with ```ptr```.
        /// Then fills ```ivtables_``` with pointers to the required ```ivtable```s from ```ptr->vtable()```.
        constexpr dptr& operator=(detail::dptr_compatible<dptr> auto* ptr) {
            ptr_ = ptr;
            fill_ivtables(std::addressof(ptr->vtable()));
            return *this;
        }
        /// Copies ```ptr_``` and the required subset of ```ivtables_``` from ```other```.
        /// If ```other``` is moved from, ```other.ptr_``` is set to ```nullptr```.
        template <tagged<dptr_tag> T>
        requires (
            (!is_unique || !std::is_lvalue_reference_v<T>) &&
            meta::subset_of_v<interfaces, typename std::remove_cvref_t<T>::interfaces> &&
            std::is_convertible_v<typename std::remove_cvref_t<T>::base_type, base_type> &&
            std::is_same_v<deleter_type, typename T::deleter_type> &&
            std::is_move_assignable_v<deleter_type>)
        constexpr dptr& operator=(T&& other) noexcept {
            ptr_ = other.ptr_;
            copy_ivtables(other.ivtables_);
            deleter_ = std::forward<T>(other).deleter_;
            if constexpr (!std::is_lvalue_reference_v<T>) other.ptr_ = nullptr;
            return *this;
        }

        constexpr bool operator==(const dptr& other) const noexcept { return ptr_ == other.ptr_; }
        constexpr auto operator<=>(const dptr& other) const noexcept { return ptr_ <=> other.ptr_; }
        constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
        /// Returns the pointer.
        constexpr base_type get() const noexcept { return ptr_; }
        /// Returns a reference to the deleter.
        constexpr deleter_type& get_deleter() noexcept { return deleter_; }
        constexpr const deleter_type& get_deleter() const noexcept { return deleter_; }
        /// @copydoc detail::bind_dyn
        template <interface V, typename T>
        requires (meta::contained_in_v<interfaces, V>)
        constexpr decltype(auto) operator[](T V::* mptr) const noexcept {
            return detail::bind_dyn(ptr_, std::get<const ivtable<V>*>(ivtables_)->*mptr);
        }
        /// Pointer arithmetic. Only enabled if the underlying object is an array (i.e. if ```is_array``` is true).
        /// @note Let ```T``` be the type of the underlying object.
        /// UB occurs if performing ```(T*)ptr_ + idx``` leads to UB.
        constexpr auto operator+(integer_alias::ptrdiff_t idx) const noexcept
        requires (is_array) {
#ifdef UTILS_DYN_NO_ARR
            static_assert(false, UTILS_DYN_NO_ARR_ERR_MSG);
#else
            using Vs_ = std::tuple<std::remove_reference_t<I>, Is...>;
            using Vs = std::conditional_t<custom_deleter,
                meta::replace_t<Vs_, -1, disabled_deleter>,
                meta::insert_t<Vs_, std::tuple_size_v<Vs_>, std::tuple<disabled_deleter>>>;
            using byte_t = follow_t<base_type, std::byte*>;
            const auto offset = integer_alias::ptrdiff_t(std::get<0>(ivtables_)->size) * idx;
            return meta::apply_t<dptr, Vs>(static_cast<byte_t>(ptr_) + offset, ivtables_);
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
        requires (!is_array && !std::is_same_v<deleter_type, disabled_deleter>) {
            destroy();
            deleter_(const_cast<void*>(ptr_), ivtables_);
            ptr_ = nullptr;
        }
        constexpr void destroy_and_delete() noexcept
        requires (deleter_type::destroying_delete) {
            deleter_(const_cast<void*>(ptr_), ivtables_);
            ptr_ = nullptr;
        }
        /// @}
    };

    /// An owning (RAII) version of ```dptr```.
    template <typename I, interface... Is>
    requires (interface<std::remove_all_extents_t<std::remove_cvref_t<I>>>)
    class unique_dptr : public dptr<I&, Is...> {
    private:
        using parent = dptr<I&, Is...>;
    public:
        using dptr<I&, Is...>::dptr;
        /// Destroyer simply calls ```destroy()```.
        constexpr ~unique_dptr() noexcept {
            if (parent::ptr_ != nullptr) parent::destroy_and_delete();
        }

        /// Releases ownership of ```ptr_``` and returns it.
        /// @note This only sets ```ptr_``` to ```nullptr``` and does not call ```destroy()```.
        constexpr parent::base_type release() noexcept {
            typename parent::base_type ptr = parent::ptr_;
            parent::ptr_ = nullptr;
            return ptr;
        }
        /// Equivalent to calling ```destroy()```, then setting ```ptr_``` to ```ptr```.
        template <detail::dptr_compatible<unique_dptr> T>
        constexpr void reset(T* ptr) noexcept {
            if (parent::ptr_ != nullptr) parent::destroy_and_delete();
            parent::operator=(ptr);
        }
        /// Equivalent to ```destroy()```.
        constexpr void reset(std::nullptr_t = nullptr) noexcept {
            if (parent::ptr_ != nullptr) parent::destroy_and_delete();
        }
    };
}