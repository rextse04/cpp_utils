# Named Requirements: <i>OperatorFunctorSet</i> {#OperatorFunctorSet}
An <i>OperatorFunctorSet</i> is a structural type that encapsulates and unifies type constraints and behavior
for overloaded operators of a user-provided class type.
`utils::*_op` accepts <i>OperatorFunctorSet</i>(s) as template parameter and translates them into operator overloads,
which is then expected to be inherited as part of the mixin pattern.
Its use is most appropriate in a scenario where the child type is a wrapper around an underlying type
for which such operations are well-defined.

## Requirements
An <i>OperatorFunctorSetTemplate</i> is a class template declared as such by the library in the following format:
```c++
template <typename Op1, ..., typename OpN, typename Traits1, ..., typename TraitsM>
struct OperatorFunctorSetTemplate;
```
Each `OpI` corresponds to an operator supported by the template, denoted by `@_I`;
and a <i>trait set</i> (from `Traits1` to `TraitsM`), denoted by `Traits_I`.
All overloadable operators are classified into <i>types</i>, each corresponding to a trait set given by a fixed template parameter name,
as specified below:

| Case | Type                | Trait Parameter Name | Remarks                             |
|------|---------------------|----------------------|-------------------------------------|
| 1    | Binary operator     | `BinaryTraits`       | This excludes assignment operators. |
| 2    | Assignment operator | `AsgTraits`          | -                                   |
| 3    | Unary operator      | `UnaryTraits`        | This excludes prefix operators.     |
| 4    | Prefix operator     | `PrefixTraits`       | -                                   |

An instance of such a template is an <i>OperatorFunctorSet</i> for a class type `T`
if it satisfies all requirements below, for each @f$I\in[N]@f$.
### Notations
| Expression | Definition                                                                            |
|------------|---------------------------------------------------------------------------------------|
| `op_i`     | A const-qualified object of `OpI`                                                     |
| `a`, `b`   | An expression of any value category and any (possibly cv-qualified) referencable type |
| `t`        | An expression of any value category and type `T` (possibly cv-qualified)              |

| Type | Definition      |
|------|-----------------|
| `A`  | `decltype(a)&&` |
| `B`  | `decltype(b)&&` |
| `TR` | `decltype(t)&&` |

| Template | Definition             |
|----------|------------------------|
| `R`      | `Traits_I::result`     |
| `C`      | `Traits_I::constraint` |
### Type Requirements
- `OpI` and `Traits` must be structural types.
### Supported Operations
<table>
<tr>
    <th>Expression (<code>exp</code>)</th>
    <th>Semantics</th>
</tr>
<tr>
    <td>
        <span style="color:green">(for Case 1)</span><br>
        <code>op_i(T::to_underlying(a), T::to_underlying(b))</code>
    </td>
    <td>
        <b>Conditions</b>
        <ul>
            <li><code>OpI</code> is not <code>utils::disable_op</code>.</li>
            <li>
                At least one of <code>a</code> and <code>b</code> is of type <code>T</code>, discarding cv-qualifiers.
                Let <code>t</code> be the first expression (in <code>a</code> and <code>b</code>) that meets this requirement.
            </li>
            <li><code>C&lt;TR, A, B&gt;::value</code> is <code>true</code>.</li>
            <li><code>exp</code> is well-formed.</li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>
                If <code>R&lt;A, B&gt;&gt;</code> is <code>utils::deduce_t</code>, <code>a @_I b</code> is equivalent to <code>exp</code>;
                otherwise, it is equivalent to <code>static_cast&lt;R&lt;TR, A, B&gt;&gt;(exp)</code>.
            </li>
        </ul>
    </td>
</tr>
<tr>
    <td>
        <span style="color:green">(for Case 2)</span><br>
        <code>op_i(T::to_underlying(a), T::to_underlying(b))</code>
    </td>
    <td>
        <b>Conditions</b>
        <ul>
            <li><code>OpI</code> is not <code>utils::disable_op</code>.</li>
            <li><code>a</code> is or is a reference to an object of type <code>T</code>.</li>
            <li><code>C&lt;A, A, B&gt;::value</code> is <code>true</code>.</li>
            <li><code>exp</code> is well-formed.</li>
        </ul>
        <b>Effects</b>
        <ul>
            <li><code>a @_I b</code> is equivalent to <code>exp, a</code> (disregarding overloading of the comma operator).</li>
        </ul>
    </td>
</tr>
<tr>
    <td>
        <span style="color:green">(for Case 3)</span><br>
        <code>op_i(T::to_underlying(t))</code>
    </td>
    <td>
        <b>Conditions</b>
        <ul>
            <li><code>OpI</code> is not <code>utils::disable_op</code>.</li>
            <li><code>C&lt;TR&gt;::value</code> is <code>true</code>.</li>
            <li><code>exp</code> is well-formed.</li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>
                <code>@_I a</code> or <code>a @_I</code>, whichever is applicable,
                is equivalent to <code>exp</code> if <code>R&lt;TR&gt;</code> is <code>utils::deduce_t</code>,
                or <code>static_cast&lt;R&lt;TR&gt;&gt;(exp)</code> otherwise.
            </li>
        </ul>
    </td>
</tr>
<tr>
    <td>
        <span style="color:green">(for Case 4)</span><br>
        <code>op_i(T::to_underlying(t))</code>
    </td>
    <td>
        <b>Conditions</b>
        <ul>
            <li><code>OpI</code> is not <code>utils::disable_op</code>.</li>
            <li><code>C&lt;TR&gt;::value</code> is <code>true</code>.</li>
            <li><code>exp</code> is well-formed.</li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>
                <code>@_I a</code> is equivalent to <code>exp, a</code> (disregarding overloading for the comma operator).
            </li>
        </ul>
    </td>
</tr>
</table>

## Remarks
For brevity, in library documentation, when a template parameter `Traits` is required to be an `OperatorFunctorSet` for `utils::*_op`,
the meaning changes slightly.
In this case, `Traits` is required to be an `OperatorFunctorSet` for each descendant of `utils::*_op`
that the user uses any of its supported operators on.