# Named Requirements: <i>Result</i> {#Result}
<i>Result</i> is a generalization of instances of templates in `<type_traits>`.

## Requirements
A class type `T` is a `Result` if it fits into one of following categories:

| Category           | Conditions                                                                                                                                                                                                                                  |
|--------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <i>TypeResult</i>  | `T::type` names a type.                                                                                                                                                                                                                     |
| <i>ValueResult</i> | `T` has a constexpr static data member named `value`.                                                                                                                                                                                       |
| <i>TraitResult</i> | `T::trait` names a class template which satisfies [<i>Trait</i>](Trait.html), such that a <i>TypeResult</i> or <i>ValueResult</i> can be obtained in a finite number of steps for any well-formed sequence of nested template instantiations. |

## Remarks
The final clause in the conditions for <i>TraitResult</i> is necessary to prevent circular definition,
as illustrated by the following contrived example:
```c++
template <typename T>
struct NotTraitResult {
    template <typename U>
    using trait = NotTraitResult<U>;
};
```