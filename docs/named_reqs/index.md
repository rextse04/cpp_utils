# Named Requirements {#named_reqs}
Similar to that in the standard library,
a named requirement is a set of expectations on a given entity (template, type or value).
Named requirements are usually included in the specification of a library-provided template,
and so act as contracts between the library author and the user on delivering correct and expected behavior.
The program is ill-formed, no diagnostic required (IFNDR), if the user passes a entity that violates
a named requirement as specified by the library.
On the other hand, templates, types or values in the library can be declared to satisfy certain named requirements
to aid with documentation and clarity.

Whiles C++20 concepts allow static checking of most type requirements,
named requirements often attach semantic meanings to expressions which are impossible to check statically or even dynamically.
Nevertheless, they are useful at refining overload resolution and eliminating verbose type errors.
As such, in instances where appropriate, a concept may be associated with a named requirement.
A type models the concept if and only if it satisfies the named requirement.
In other words, it may fulfill all semantics specified in the named requirement, apart from the type constraints.

## Key Terms
A named requirement consists of a list of expressions that a candidate entity has to support,
with extra semantic requirements as specified.
Semantic requirements are usually divided into two parts:
- <b>Conditions</b>

  Conditions are requirements the user of the expression has to fulfill.
  Otherwise, the behavior may be unspecified or undefined, or the program may be ill-formed (with or without diagnostics).
  In other words, the author of the entity may assume all conditions to hold if the expression is used.

  Conditions may be further classified into pre-conditions and post-conditions.
  <b>Pre-conditions</b> are conditions imposed on behavior before the expression is used,
  while <b>post-conditions</b> are that imposed on after.
- <b>Effects</b>
  
  Effects are requirements the provider of the entity has to fulfill.
  They can be understood as the guarantees given by the provider to the user
  regarding the program state during and after the execution of the expression.
  If there is any scenario where such requirements do not hold while all conditions are met,
  the candidate entity does not satisfy the named requirement.

  Unless otherwise specified, an expression is allowed to throw,
  in which case the expected effects by the normal flow of program may be unfulfilled.

## Named Requirements in the Library
### Allocators
- @subpage BufferAllocator
- @subpage ResourceAllocator