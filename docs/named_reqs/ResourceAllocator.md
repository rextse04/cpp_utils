# Named Requirement: <i>ResourceAllocator</i> {#ResourceAllocator}
A resource allocator is an <i>Allocator</i> which operates on buffer memory allocated from an upstream `std::pmr::memory_resource`.
It can be wrapped in `pmr::allocator_resource` to act as an `std::pmr::memory_resource`,
which is useful when one would like to reuse the same allocation strategy with or without virtual dispatch.

## Requirements
A class type `A` meets the named requirement if it satisfies <i>Allocator</i> and all requirements below.
### Notations
| Value       | Definition                                                            |
|-------------|-----------------------------------------------------------------------|
| `a`         | A (non-cv-qualified) object of type `A`                               |
| `mr`        | A (non-cv-qualified) l-value reference of `std::pmr::memory_resource` |
| `space`     | A value of type `std::size_t`                                         |
| `bytes`     | A value of type `std::size_t`                                         |
| `alignment` | A value of type `std::size_t` which is a power of 2                   |
| `p`         | A value of type `std::byte*`                                          |
### Supported Operations
<table>
<tr><th>Expression</th><th>Return Type</th><th>Semantics</th></tr>
<tr>
    <td><code>A::value_type</code></td>
    <td><code>std::byte</code></td>
    <td>-</td>
</tr>
<tr>
    <td><code>A(mr, space, v0, ..., vk)</code></td>
    <td><code>A</code></td>
    <td>
        There must be a sequence of types <code>V1,...,Vk</code> such that the expression is well-formed for
        any <code>v0</code> of type <code>V0</code>, and so on.<br>
        <b>Effects</b>
        <ul>
            <li>
                A storage of <code>space</code> bytes is allocated through <code>mr</code> and
                used as the underlying buffer of <code>a</code>.
            </li>
        </ul>
    </td>
</tr>
<tr>
    <td><code>a.release(mr, space)</code></td>
    <td><i>(not used)</i></td>
    <td>
        The call must be <code>noexcept</code>.<br>
        <b>Conditions</b>
        <ul>
            <li>
                <code>a</code> is equal to an allocator (implicitly or explicitly)
                constructed in the form <code>A(mr, space, ...)</code>.
            </li>
            <li><code>release</code> has not been called on any allocator (of type <code>A</code>) equal to <code>a</code>.</li>
            <li>The underlying buffer is not used in any way after the call, including through allocator operations.</li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>The underlying buffer is deallocated through <code>mr</code>.</li>
        </ul>
    </td>
</tr>
<tr>
    <td><code>a.allocate(bytes, alignment)</code></td>
    <td><code>std::byte*</code></td>
    <td>
        <b>Effects</b>
        <ul>
            <li>
                Allocates storage suitable for an array object of type <code>std::byte[bytes]</code>
                from the underlying buffer with the specified <code>alignment</code>, and creates the array.
            </li>
        </ul>
    </td>
</tr>
<tr>
    <td><code>a.deallocate(p, bytes, alignment)</code></td>
    <td><i>(not used)</i></td>
    <td>
        <b>Conditions</b>
        <ul>
            <li><code>p</code> is obtained (potentially by conversion) through a call to <code>a.allocate(bytes, alignment)</code>.</li>
            <li><code>p</code> has not been deallocated by an allocator equal to <code>a</code>.</li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>Deallocates storage pointed to by <code>p</code>.</li>
            <li>No exceptions are thrown.</li>
        </ul>
    </td>
</tr>
</table>

## Associated Concept
The library provides `pmr::resource_allocator<A>` as an equivalent constraint to the named requirement.