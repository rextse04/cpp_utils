# Named Requirements: <i>IntegerBehaviorProfile</i> {#IntegerBehaviorProfile}
An <i>IntegerBehaviorProfile</i> encapsulates how an integer wrapper type (such as `utils::integer`)
ought to behave under a specified set of operations involving integer-like types.

## Requirements
An <i>IntegerBehaviorProfileTemplate</i> is a class template declared as such by the library in the following format:
```c++
template <op_functors_1 Funcs1, ..., op_functors_N FuncsN>
struct IntegerBehaviorProfileTemplate {
    static constexpr auto funcs_1 = Funcs1;
    ...
    static constexpr auto funcs_n = FuncsN;
};
```
where every `op_functors_I` is an [<i>OperatorFunctorSetTemplate</i>](OperatorFunctorSet.md).
The names of template parameters and member variables are only for illustration and may appear otherwise in implementation.

Let `IT` be a theoretical collection of wrapper types such that `IT<Int>` represents a unique wrapper around `Int` in `IT`
for any integer-like type `Int`.
In addition, every `IT<Int>` (denoted `I` below) satisfies the following properties:
1. `I` has an instance variable of type `Int` that represents the value of the instance.
   (The variable is called the <i>underlying object</i> of the instance.)
2. `I::underlying_type` is `Int`.
3. For every integer-like type `U`, let `u` be a (possibly cv-qualified) object of type `U`
and `w` be a (possibly cv-qualified) object of type `Int<U>`.
Denote by `R(obj)` a reference (of any value category) to an object `obj`.
Then, `I::to_underlying(R(u))` returns `R(u)` and `I::to_underlying(R(w))` returns `R(v)`,
where `v` is the underlying object of `w`.
`I::to_underlying` is not overloaded for any other types.
4. `I::template rebind<U>` gives `IT<U>` for any integer-like type `U`.
5. Given a (possibly cv-qualified) object `w` of type `I`, `+w` gives a copy of the underlying object.
6. `std::numeric_limits` is meaningfully specialized for `IT<Int>`.

Then, an instance of an <i>IntegerBehaviorProfileTemplate</i> is an <i>IntegerBehaviorProfile</i>
if `FuncsI` is an [<i>OperatorFunctorSet</i>](OperatorFunctorSet.md) for every type in `IT`, for every $I\in[N]$.