# Named Requirements: <i>Trait</i> {#Trait}
<i>Trait</i> is a generalization of <i>UnaryTypeTrait</i> and <i>BinaryTypeTrait</i> in the standard library.

## Requirements
A class template or alias template `Tmpl` satisfies <i>Trait</i> if every well-formed instantiation of `Tmpl` produces a
1. [<i>TypeResult</i>](Result.html), in which case it is a <i>TypeTrait</i>; or
2. [<i>ValueResult</i>](Result.html), in which case it is a <i>ValueTrait</i>; or
3. [<i>TraitResult</i>](Result.html), in which case it is a <i>MetaTrait</i>.

A <i>Trait</i> can belong to one or more of the above categories.