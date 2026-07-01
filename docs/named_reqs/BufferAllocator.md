# Named Requirement: <i>BufferAllocator</i>
A <i>BufferAllocator</i> is an <i>Allocator</i> which operates on a contiguous block of buffer memory,
usually pre-allocated by an upstream memory source.
<i>BufferAllocator</i>s are typically used in scenarios where memory allocation and deallocation need to be fast and predictable,
such as in real-time systems or high-performance applications.
Optionally, a <i>BufferAllocator</i> may provide thread-safe guarantees for concurrent allocator operations,
in which case it is a <i>SynchronizedBufferAllocator</i>.
If it further allows construction and assignment between its non-synchronized counterpart and itself,
it is a <i>ConvertibleBufferAllocator</i>.

## Requirements
A class type `A` meets the <i>BufferAllocator</i> named requirement if it satisfies <i>Allocator</i> and all requirements below.
### Notations
| Type                                                                         | Definition                                                                             |
|------------------------------------------------------------------------------|----------------------------------------------------------------------------------------|
| `AT`                                                                         | `std::allocator_traits<T>`                                                             |
| `T`                                                                          | `A::value_type`                                                                        |
| `P`                                                                          | `AT::pointer`                                                                          |
| `U`                                                                          | A cv-unqualified object type, which may or may not be same as `T`                      |
| `B`                                                                          | `A` rebound to `U`, i.e. `std::allocator_traits<A>::rebind_alloc<U>`                   |
| `C` <span style="color:green">(for <i>ConvertibleBufferAllocator</i>)</span> | `A` rebound to `U` without synchronization, i.e. `A::template rebind<U, false>::other` |

| Value   | Definition                                                                                                              |
|---------|-------------------------------------------------------------------------------------------------------------------------|
| `a`     | A (non-cv-qualified) object of type `A`                                                                                 |
| `ca`    | A potentially const-qualified object of type `A`                                                                        |
| `b`     | A (non-cv-qualified) object of type `B`                                                                                 |
| `cb`    | A potentially const-qualified object of type `B`                                                                        |
| `c`     | A (non-cv-qualified) object of type `C`                                                                                 |
| `cc`    | A potentially const-qualified object of type `C`.                                                                       |
| `buf`   | An `std::byte` pointer to a contiguous block of memory                                                                  |
| `space` | A value of type `std::size_t` that represents the size (in bytes) of the contiguous block of memory pointed to by `buf` |
| `n`     | A value of type `AT::size_type`                                                                                         |
| `p`     | A value of type `P` obtained from a call to `AT::allocate`.                                                             |
| `cvp`   | A value of type `AT::const_void_pointer` obtained by conversion from `p`.                                               |                                                                                                      
### Supported Operations
<table>
<tr><th>Expression</th><th>Return Type</th><th>Semantics</th></tr>
<tr>
    <td>
        <span style="color:green">(for <i>SynchronizedBufferAllocator</i>)</span><br>
        <code>A::synchronized</code>
    </td>
    <td>bool</td>
    <td>Must be <code>true</code>.</td>
</tr>
<tr>
    <td>
        <span style="color:green">(for <i>ConvertibleBufferAllocator</i>)</span><br>
        <code>A::template rebind&lt;U, false&gt;::other</code>
    </td>
    <td><code>C</code></td>
    <td><code>C::template rebind&lt;T, true&gt;::other</code> must be the same as <code>A</code>.</td>
</tr>
<tr>
    <td><code>A(buf, space)</code></td>
    <td><code>A</code></td>
    <td>
        <b>Conditions</b>
        <ul>
            <li>
                There are no external reads or writes to <code>buf</code> except by the allocator(s) which manage it,
                until it is released by all such allocators.
            </li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>Constructs an allocator that manages <code>buf</code>.</li>
        </ul>
    </td>
</tr>
<tr>
    <td>
        <code>A(cb)</code><br><code>A(std::move(b))</code><br>
        <span style="color:green">(for <i>ConvertibleBufferAllocator</i>)</span><br>
        <code>A(cc)</code><br><code>A(std::move(c))</code>
    </td>
    <td><code>A</code></td>
    <td rowspan="2">
        Let <code>L</code> be the constructed type and <code>r</code> be the argument.<br>
        <b>Effects</b>
        <ul>
            <li>Constructs an allocator <code>l</code> that manages the same buffer as <code>r</code> (<code>l == r</code>).</li>
            <li>If <code>r</code> is const-qualified, <code>r</code> retains management of its buffer.</li>
            <li>Otherwise, it is implementation-defined whether <code>r</code> releases management of its buffer.</li>
        </ul>
    </td>
</tr>
<tr>
    <td>
        <span style="color:green">(for <i>ConvertibleBufferAllocator</i>)</span><br>
        <code>C(ca)</code><br><code>C(std::move(a))</code>
    </td>
    <td><code>C</code></td>
</tr>
<tr>
    <td>
        <code>a = cb</code><br><code>a = std::move(b)</code><br>
        <span style="color:green">(for <i>ConvertibleBufferAllocator</i>)</span><br>
        <code>a = cc</code><br><code>a = std::move(c)</code>
    </td>
    <td><code>A&</code></td>
    <td rowspan="2">
        Let <code>l</code> be the left operand and <code>r</code> be the right operand.<br>
        <b>Effects</b>
        <ul>
            <li><code>l</code> releases management of its current buffer.</li>
            <li>After the assignment, <code>l</code> manages the same buffer as <code>r</code> (<code>l == r</code>).</li>
            <li>If <code>r</code> is const-qualified, it retains management of its buffer.</li>
            <li>Otherwise, it is implementation-defined whether <code>r</code> releases management of its buffer.</li>
        </ul>
    </td>
</tr>
<tr>
    <td>
        <span style="color:green">(for <i>ConvertibleBufferAllocator</i>)</span><br>
        <code>c = ca</code><br><code>c = std::move(a)</code>
    </td>
    <td><code>C&</code></td>
</tr>
<tr>
    <td><code>ca.~A()</code></td>
    <td>-</td>
    <td>
        <b>Effects</b>
        <ul>
            <li>Releases management of the buffer currently managed by <code>ca</code>.[^1]</li>
        </ul>
    </td>
</tr>
<tr>
    <td><code>ca == cb</code><br><code>ca != cb</code></td>
    <td><code>bool</code></td>
    <td>
        Returns <code>true</code> (resp. <code>false</code>) if and only if
        <code>ca</code> and <code>cb</code> manage the same buffer.
    </td>
</tr>
<tr>
    <td><code>a.allocate(n)</code><br><code>a.allocate(n, cvp)</code> <span style="color:green">(optional)</span></td>
    <td><code>A::pointer</code></td>
    <td rowspan="2">
        In addition to requirements of <i>Allocator</i>:<br>
        <b>Conditions</b>
        <ul>
            <li>
                There is no concurrent call to <code>allocate</code> or <code>deallocate</code>
                from another allocator that manages the same buffer.
                <span style="color:green">(N/A to SynchronizedBufferAllocator)</span>
            </li>
            <li>
                There is no concurrent call to <code>allocate</code> or <code>deallocate</code>
                from another non-synchronized allocator that manages the same buffer.
                <span style="color:green">(for ConvertibleBufferAllocator)</span>
            </li>
        </ul>
        <b>Effects</b>
        <ul>
            <li>If an array of <code>T</code> is constructed, it lives on the managed buffer.</li>
        </ul>
    </td>
</tr>
<tr>
    <td><code>a.allocate_at_least(n)</code> <span style="color:green">(optional)</span></td>
    <td><code>std::allocation_result&lt;P&gt;</code></td>
</tr>
<tr>
    <td><code>a.deallocate(p, n)</code></td>
    <td><i>(not used)</i></td>
    <td>
        In addition to requirements of <i>Allocator</i>:<br>
        <b>Conditions</b>
        <ul>
            <li>
                There is no concurrent call to <code>allocate</code> or <code>deallocate</code>
                from another allocator that manages the same buffer.
                <span style="color:green">(N/A to SynchronizedBufferAllocator)</span>
            </li>
            <li>
                There is no concurrent call to <code>allocate</code> or <code>deallocate</code>
                from another non-synchronized allocator that manages the same buffer.
                <span style="color:green">(for ConvertibleBufferAllocator)</span>
            </li>
        </ul>
    </td>
</tr>
</table>

[^1]: This is purely a semantic requirement. When and whether the buffer is actually freed is implementation-defined.