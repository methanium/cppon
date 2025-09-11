# Visitors internals

This file documents how hierarchical access, dereferencing and autovivification work inside `ch5::visitors`.

## Goals

- Uniform path traversal for both objects (`object_t`) and arrays (`array_t`).
- Support for two indirections:
  - `pointer_t` (direct in‑memory links)
  - `path_t` (textual absolute path; always starts with '/')
- Distinct read vs write semantics (no accidental autovivification in const access).
- Thread‑local root stack used to resolve absolute paths and `path_t`.

## Root stack and thread-local storage

- - `static_storage` holds:
  - A thread-local root stack (vector<cppon*>), initialized with a bottom sentinel (nullptr)
  - A per-thread `Null` sentinel returned by `visitors::null()`
- Invariants:
  - `hoist_if_found` promotes an existing entry to the top (O(n) worst-case, small n in practice)
  - The root stack is never empty; `get_root()` asserts top != nullptr
  - Each pushed root is unique (push promotes if already deeper).
  - `push_root` enforces uniqueness (no duplicates); `pop_root` removes only if found
  - `pop_root(x)` is a no‑op if `x` is not found; safe with temporaries.
  - Lifetime: `cppon::~cppon()` calls `pop_root(*this)`.

## Dereferencing helpers

Quick summary
- deref_if_ptr (const/non-const, internal)
  - pointer_t: returns `*arg` or `null()` if the pointer is null
  - path_t: resolves against `get_root()`, unconditionally removes the leading '/'
  - others: passthrough (returns the original object)
- deref_if_not_null (non-const only, internal)
  - If the slot holds a null pointer_t, returns the slot itself (autovivify at the slot)
  - Otherwise delegates to deref_if_ptr

| Helper               | Input kind    | Result                                                                 |
|----------------------|---------------|------------------------------------------------------------------------|
| `deref_if_ptr`       | `pointer_t`   | `*p` or thread sentinel `null()` if `p == nullptr`                     |
|                      | `path_t`      | Resolves against `get_root()` (drops leading '/')                      |
|                      | other         | Returns original slot                                                  |
| `deref_if_not_null`  | `pointer_t`   | If null pointer → returns slot itself (so write autovivifies *here*)   |
|                      | others        | Delegates to `deref_if_ptr`                                            |

Why `deref_if_not_null`?  
When writing `/a/b/c`, if `/a/b` currently holds a null pointer_t, we want to *replace that slot* by an object or array – not mutate a sentinel behind it.

## Autovivification rules (write path)

For each non‑terminal segment:
1. Determine next segment token (numeric → array, otherwise → object).
2. If the current value is:
   - `null` sentinel or null `pointer_t` → slot becomes new container.
   - An object but next is numeric → `type_mismatch_error`.
   - An array but next is string → `type_mismatch_error`.
3. Arrays: expanding from `size` → `index` allowed only if  
   `index - size <= CPPON_MAX_ARRAY_DELTA`, else `excessive_array_resize_error`.

Const traversal never creates containers:
- Missing object member → returns `null()` at the leaf (leaf only).
- Out‑of‑bounds array index → returns `null()` at that position.
- Continuing past `null()` when more segments remain → throws:
  - `member_not_found_error` (object path),
  - `null_value_error` (array path).

## Complexity

| Operation type                      | Complexity (typical small JSON) |
|------------------------------------|----------------------------------|
| Object member lookup               | O(n) linear (n small, cache‑friendly) |
| Array index                        | O(1) (resize amortized)          |
| Autovivification decision          | O(1)                             |
| Root hoist (`hoist_if_found`)      | O(n) worst-case (n = depth of root stack, usually tiny) |

## Error summary

| Situation                                             | Exception                       |
|-------------------------------------------------------|---------------------------------|
| Non‑digit array segment                               | `bad_array_index_error`         |
| Crossing a `null` with more path remaining (object)   | `member_not_found_error`        |
| Crossing a `null` with more path remaining (array)    | `null_value_error`              |
| Non‑container for a non‑terminal segment              | `type_mismatch_error`           |
| Excessive sparse array growth                        | `excessive_array_resize_error`  |
| Malformed `path_t` construction                      | `invalid_path_error`            |

## pointer_t vs path_t

| Aspect          | pointer_t                     | path_t                               |
|-----------------|-------------------------------|--------------------------------------|
| Representation  | Raw pointer into live tree    | Absolute textual path                |
| Stability       | Invalid if node reallocated   | Stable as long as key/index layout   |
| Serialization   | Emitted as a path token (`$cppon-path:`)| Emitted unchanged as path token |
| Flatten (option)| Can be inlined when `flatten` enabled | Resolved then possibly re‑emitted as path |
| Round‑trip      | pointer_t → path_t → pointer_t via resolve_paths/restore_paths |
| Restoration     | `restore_paths(refs)`         | Not needed |

Notes:
- The printer never writes binary data for pointer_t: it is always converted to a path_t representation.
- The `flatten` option tries to inline the pointed-to subtree but leaves a path if a cycle is detected.

## Path model and invariants

- `path_t` is always absolute
  - Must start with `/`; malformed paths throw `invalid_path_error` at construction
  - The `"_path"` UDL enforces the same rule
- Root resolution
  - Strings of the form `"$cppon-path:/.../..."` produce path_t with leading '/'
  - `"$cppon-path:/"` is valid and resolves to the current root (visitor(obj, "") returns obj)
- deref behaviors (const vs non-const)
  - Read-only traversal uses `deref_if_ptr` (no autovivification)
  - Writable traversal uses `deref_if_not_null` (autovivify containers at the next segment when needed)

## Null sentinel vs null pointer_t

- `null()` sentinel: a real `cppon` instance holding `nullptr_t`.
- Null `pointer_t`: stored inside a slot – path deref sees it and may autovivify the slot itself (`deref_if_not_null`), never the sentinel.

## Read vs write flow (simplified)

Write `/a/0/x`:
- start:
  - root (object or autovivified object_t) segment 'a' (string) →
  - ensure object_t segment '0' (numeric) →
  - ensure array_t under /a segment 'x' (leaf) →
  - ensure object_t at /a/0 then return slot
  
Const read uses `deref_if_ptr` only – no autovivification; leaf missing → `null()`.

## Adding a new container-like alternative

1. Extend `value_t` variant.
2. Update:
   - Overloads in `visitors::visitor` (array/object dispatch).
   - `cppon::operator[]` if new indexing semantics.
3. Reassess error propagation for mixed paths.

## Safety notes

- Do not store references/pointers to `null()` across threads.
- Avoid calling `push_root` / `pop_root` manually unless implementing new traversal primitives—prefer `root_guard`.
- Keep `CPPON_MAX_ARRAY_DELTA` conservative to surface accidental sparse writes early.

## Examples

### Read

```cpp
auto& name = std::get<string_view_t>(doc["/user/name"]);
if (auto age = std::get_if<number_t>(&doc["/user/age"])) {
	// lazy number still present
}
```

### Write with autovivification

```cpp
doc["/config/servers/0/host"] = "localhost";
doc["/config/servers/0/port"] = 8080;
```

Creates `/config` (object), `/config/servers` (array), expands index 0, creates object at `[0]`.

### Null pointer slot
```cpp
doc["/p"] = pointer_t{nullptr};
doc["/p/sub"] = 42; // /p becomes object_t, not the sentinel
```

---

Planned extensions:
- Optional hashed side index for large objects.
- Iterator-based streaming traversal hooks.