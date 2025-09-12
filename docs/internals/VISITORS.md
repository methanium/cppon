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

## Value getters (numeric / blob / optional helpers)

Resolution order (all getters):

1. If the current node is a `path_t`, traverse it (absolute if it starts with '/').
2. If it is a `pointer_t`, follow it (stop if null → failure for strict/cast, or returns nullptr in optional).
3. Operate on the resolved concrete alternative.

### Overview

| Getter                        | Mutates node? (non‑const) | Lazy number (`number_t`) handling | Blob handling | Const behaviour on lazy form | Return |
|------------------------------|---------------------------|-----------------------------------|---------------|------------------------------|--------|
| `get_strict<T>(cppon&)`      | Yes (promotes)            | Converts in place (type → concrete) | N/A         | Throws (`number_not_converted_error`) | Value `T` |
| `get_strict<T>(const cppon&)`| No                        | Throws                            | N/A          | Throws                        | Value `T` |
| `get_cast<T>(cppon&)`        | Yes (promotes)            | Converts first if `number_t`      | N/A          | Throws if still lazy         | Value `T` (cast) |
| `get_cast<T>(const cppon&)`  | No                        | Throws                            | N/A          | Throws                        | Value `T` (cast) |
| `get_blob(cppon&)`           | Yes (realizes)            | N/A                                | Decodes `blob_string_t` → `blob_t` | N/A | `blob_t&` |
| `get_blob(const cppon&)`     | No                        | N/A                                | Throws on `blob_string_t` (`blob_not_realized_error`) | N/A | `const blob_t&` |
| `get_optional<T>(cppon&)`    | No (unless blob realize)  | Does NOT auto‑convert (returns nullptr if still lazy and `T` is concrete) | Realizes only if `T = blob_t` via manual get_blob | Same as non‑const | `T*` |
| `get_optional<T>(const cppon&)`| No                     | Returns nullptr on lazy numeric   | Returns `blob_t*` only if already realized | Same | `const T*` |

Notes:

- “Promotes” = replaces the `number_t` alternative by the concrete numeric (via `convert_to_numeric(value_t&)`, defined in ALTERNATIVES).
- `get_cast<T>` will accept any arithmetic alternative and `static_cast` to `T` (after promotion if needed).
- `get_strict<T>` requires the stored type to already be exactly `T` (after promotion if non‑const), otherwise throws `type_mismatch_error`.
- `get_blob(non‑const)` realizes a `blob_string_t` eagerly; `const` refuses and throws.
- `get_optional<T>` never throws on mismatch: it returns `nullptr` if, after dereference, the slot does not hold `T`. It still walks through `path_t` / `pointer_t` indirections.

### Error mapping (primary)

| Condition                                 | Exception |
|-------------------------------------------|-----------|
| `get_strict<T>` wrong concrete type       | `type_mismatch_error` |
| `get_strict<T>` const on lazy number_t    | `number_not_converted_error` |
| `get_cast<T>` const on lazy number_t      | `number_not_converted_error` |
| `get_blob(const)` on `blob_string_t`      | `blob_not_realized_error` |
| Blob decode invalid (raise=true)          | `invalid_base64_error` |
| Getter on incompatible non‑numeric type   | `type_mismatch_error` |

### Practical examples

```cpp
// Lazy numeric extracted strictly (non-const promotes)
auto& n = doc["/stats/count"];          // number_t (lazy)
int exact = ch5::get_strict<int>(n);    // promotes number_t -> int32_t (or wider) then returns

// Casting
double d = ch5::get_cast<double>(doc["/stats/count"]);

// Optional pattern
if (auto p = ch5::get_optional<uint64_t>(doc["/user/id"])) {
    // use *p
}

// Blob realization
auto& b = ch5::get_blob(doc["/payload/raw"]); // blob_string_t -> blob_t
```

### Design rationale

- Keep numeric / blob transformation logic close to the variant definition (ALTERNATIVES) while centralizing dereference in one place (VISITORS).
- Separation of strict vs cast keeps failure explicit and avoids silent narrowing / promotion surprises.
- Allow advanced callers to decide when to pay decoding / numeric conversion (lazy parse path).

### Caveats

- After promotion, the original textual form (number_t) is lost; re‑serialization will produce the canonical numeric formatting.
- Realizing a blob invalidates any expectation that emission will preserve the original base64 layout (re-encoding may differ in padding grouping).
- `get_optional<T>` does not trigger numeric promotion—explicitly call a strict/cast getter first if needed.

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