# Paths and References

C++ON supports two complementary reference mechanisms:
- path_t: textual path tokens (e.g. "$cppon-path:/a/b/0")
- pointer_t: direct pointers to nodes in the same document

Use path_t when you want a stable, text-only reference (safe across copies/reshapes).
Use pointer_t for fast in-memory links when object addresses are stable.

## Absolute vs relative paths

- Absolute path access (starts with '/') sets the root automatically for that access.
- In most cases, absolute access pushes the proper root for the duration of the access and the internal RAII will pop it.

```cpp
auto& item = doc["/absolute/path/to/item"]; // root = doc for this access
auto& next  = item["deeper/access"];        // relative to 'item'
auto& other = item["other/deeper"];         // still relative to 'item'
```

## Root context and root_guard

- The root stack is thread-local; root changes affect only the current thread.
- Use `root_guard` only when two documents can compete for the top of the root stack.
- `root_guard` is useful to pin a specific document as root across a scope when you are interleaving accesses from multiple cppon objects in the same thread.

```cpp
// object1 and object2 are both alive in the same scope
ch5::cppon object1 = ch5::eval(R"({
    "object1": { "path": { "to": { "item": {
      "deeper": { "access": { "to": { "other": { "stuff": 42 } } } },
      "other": { "stuff": true }
    } } } }
})");

ch5::cppon object2 = ch5::eval(R"({
    "object2": { "path": { "to": { "item": 3.14 } } }
})");

// Absolute access sets root to object1 for this access
auto& item = object1["/object1/path/to/item"];
auto& rel1 = item["deeper/access/to/other/stuff"]; // relative to item

{
    ch5::root_guard guard(object2); // pin root to object2 in this scope    
    auto& x = object2["/object2/path/to/item"]; // absolute on object2
    // guard dtor restores previous root (object1 context)
}

auto& rel2 = item["other/stuff"]; // still relative to item
```

## Reading a path token

If a member stores a path token, you can resolve it:

```cpp
auto obj = ch5::eval(R"({
  "data": {"x":"v"},
  "ref": "$cppon-path:/data/x"
})");

// The token is stored as path_t until resolved
auto p = std::get<ch5::path_t>(obj["/ref"]).value; // "/data/x"

// Ensure the correct root when resolving the path:
// - Either use obj["/..."] once (absolute) before resolving,
// - Or use ch5::root_guard(obj) around the resolution.
auto& target = ch5::visitors::visitor(obj, p.substr(1)); // resolve relative to obj
auto v = std::get<std::string_view>(target);   // "v"
// 'obj' as base here
```

## Optimizing paths into pointers

Convert path_t to pointer_t for repeated access and faster dereferencing:

```cpp
ch5::cppon doc = ch5::eval(R"({
  "a": {"x": 1},
  "b": {"ref": "$cppon-path:/a"}
})");

auto refs = ch5::resolve_paths(doc); // path_t -> pointer_t (in place)
assert(std::holds_alternative<ch5::pointer_t>(doc["/b/ref"]));

// Restore to path_t if needed (e.g., before JSON-compatible printing)
ch5::restore_paths(refs);
```

Tip:
- Keep the returned reference_vector_t alive while the optimized pointers are used.

## From pointer to path (reverse lookup)

When needed (e.g., for diagnostics), you can compute a textual path from a pointer:

```cpp
ch5::cppon doc = ch5::eval(R"({
    "a": {"x": 1},
    "b": {"ref": "$cppon-path:/a"}
})");

auto p = std::get<ch5::pointer_t>(doc["/b/ref"]);
auto path = ch5::find_object_path(doc, p); // "/a"
```

## Flattening during printing

Flatten inlines referenced content when possible. Cycles are preserved as paths.

```cpp
ch5::cppon d;
d["/a/value"] = "x";
d["/b"] = &d["/a"];       // pointer to /a

// Cycle
d["/a/self"] = &d["/a"];

auto refs = ch5::resolve_paths(d);
std::string flat = ch5::to_string(d, &refs, R"({"layout":{"flatten":true}})");
// Referenced objects are inlined; cyclic edges remain as paths
```

## Lifetimes and stability

- `pointer_t` is non-owning; its validity depends on the lifetime and address stability of the target node.
- Avoid inserting/erasing in containers that would reallocate while `pointer_t` is in use. Reserve first, or assign pointers after structure is set.
- Keep `reference_vector_t` alive while dereferencing optimized pointers so that you can restore textual paths if needed.

## Safety and best practices

- Prefer path_t on API boundaries; convert to pointer_t internally when needed.
- pointer_t requires address stability. Reserve containers or assign after structure is set.
- To avoid order-of-evaluation pitfalls, always sequence:

```cpp
auto& slot = doc["/p"]; 
slot = &doc["/a"];
```

## Path syntax and errors

- Segments are '/'-separated.
- A segment consisting of decimal digits selects an array index; otherwise it selects an object member.
- Empty or malformed segments raise `invalid_path_segment_error`.
- Nonexistent members raise `member_not_found_error`; invalid indices raise `bad_array_index_error`.
- path_t is always absolute (must start with '/'); otherwise invalid_path_error is thrown at construction.
- "$cppon-path:/" is valid and resolves to the root (visitor(obj, "") returns obj).
- Use object()/array() accessors when you need finer control or keys that cannot be expressed as path segments.

## Troubleshooting

- Access fails with `member_not_found_error`: check the active root or use an absolute path (`"/..."`) to reset it for the access.
- `invalid_path_segment_error`: verify that segments are non-empty and valid; numbers for array indices, otherwise member names.
- Printing with `flatten` duplicates or cycles: cycles are intentionally preserved as paths; inspect with `is_pointer_cyclic`.
- `json_compatibility_error` when printing: check for out-of-range 64-bit integers when using JSON-compatible layout.
- `unsafe_pointer_assignment_error`: ensure the target node is within the same document and that its address is stable.
