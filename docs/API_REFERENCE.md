# API Reference (Overview)

This page lists the main public types and functions. See the other documents for usage details and design notes.

- [Introduction:](./INTRODUCTION.md)
- [Configuration:](./CONFIGURATION.md)
- [Paths and References:](./PATHS.md)
- [Internals:](./internals/INDEX.md)

---

## Namespace

All symbols are under the `ch5` namespace.

---

## Value model

`cppon` holds a tagged union (variant) of the following alternatives:

| Type               | Description                          |
|--------------------|--------------------------------------|
| object_t           | associative ordered object as vector of (name,value) |
| array_t            | sequential array of values |
| string_view_t      | non‑owning UTF‑8 view |
| string_t           | owning UTF‑8 string |
| number_t           | lazily convertible numeric token |
| boolean_t          | bool |
| nullptr_t          | null |
| path_t             | textual path token: "$cppon-path:/a/b/0" |
| pointer_t          | direct in‑document pointer (see [PATHS.md](PATHS.md)) |
| blob_string_t      | textual Base64 blob: "$cppon-blob:..." |
| blob_t             | realized binary blob |
| float_t, double_t  | concrete floating types |
| i8/u8, i16/u16     | concrete integer types |
| i32/u32, i64/u64   | concrete integer types |

Notes
- `number_t` defers numeric parsing; it converts to a concrete numeric type on demand.
- `string_view_t` remains valid as long as the owning buffer outlives the `cppon` value.

---

## Core type

- class cppon : public value_t
  - Indexing (path and array)
    - cppon& operator[](string_view_t path)
    - const cppon& operator[](string_view_t path) const
    - cppon& operator[](size_t index)
    - const cppon& operator[](size_t index) const
  - Containers (throws on type mismatch)
    - array_t& array()
    - const array_t& array() const
    - object_t& object()
    - const object_t& object() const
  - Optional access (nullptr on mismatch)
    - array_t* try_array() noexcept
    - const array_t* try_array() const noexcept
    - object_t* try_object() noexcept
    - const object_t* try_object() const noexcept
  - Utilities
    - bool is_null() const
    - Assignments for all supported alternatives

Path resolution
- Paths starting with '/' are resolved from the active root. Use root_guard to pin a root across a scope; see [PATHS.md](PATHS.md).

---

## Document wrapper (owning buffer)

The `document` class wraps a `cppon` and owns the source text buffer so that all `strings` produced during parsing remain valid for its lifetime.

Constructors & Methods

```cpp
document(); // empty => {}
explicit document(const char* text, options = Quick);
explicit document(std::string_view text, options = Quick);
explicit document(std::string&& text, options = Quick);

bool empty() const noexcept;
std::string_view source_view() const noexcept;
const std::string& source() const noexcept;
document& eval(const char* text, options = Quick);
document& eval(std::string_view text, options = Quick);
document& eval(std::string&& text, options = Quick);
document& reparse(options = Quick);
document& rematerialize(const cppon& print_opts = cppon{}, options parse_mode = Quick);
std::string serialize(const cppon& print_opts = cppon{}) const;
document& clear() noexcept;
static document from_string(std::string&& text, options = Quick);
static document from_file(const std::string& filename, options = Quick);
``` 

Invariants & Note

-  Empty document == `{}` (object_t), never null.
- `eval(nullptr)` is treated as `eval("{}")` (buffer = "{}").
- `rematerialize` = serialize + reparse → re‑anchors all views (previous string_view_t values must not be cached across this).
- `serialize` does not touch the internal buffer (not a stabilization step).

Use cases

- Load text and navigate without copying substrings.
- Re-anchor views after structural transformations (rematerialize if needed).
- Fast export (serialize).

---

## Parsing

- cppon eval(string_view_t text, options opt = options::eval)

Options (materialization level)

- full  : full evaluation; blobs decoded
- eval  : full evaluation; blobs kept as blob_string_t
- quick : lazy numbers; tokens preserved where applicable (fast)
- parse : validation only; does not materialize a DOM

Errors
- Throws precise exceptions on malformed input (see Exceptions).

---

## Number helpers

- ```cppT get_strict(cppon& v)``` / ```cppT get_strict(const cppon& v)```
  - Requires exact numeric type; if `v` holds `number_t` and is non‑const, conversion occurs; otherwise throws.
- ```cppT get_cast(cppon& v)``` / ```cppT get_cast(const cppon& v)```
  - Casts across supported numeric types; converts `number_t` in non‑const; throws on invalid cast.

Notes
- JSON‑compat layout in the printer enforces double‑precision integer safety range; see Printing.

---

## Blobs

- ```cppblob_t& get_blob(cppon& v, bool raise = true)```
  - Non‑const: decodes `blob_string_t` to `blob_t` on demand.
- ```cppconst blob_t& get_blob(const cppon& v)```
  - Const: throws `blob_not_realized_error` if not realized (or if `raise` in non‑const was set true and realization fails).

---

## Printing

Overloads
```cpp
std::string to_string(const cppon& value, string_view_t options = {});
std::string to_string(const cppon& value, const cppon& options);
std::string to_string(const cppon& value, reference_vector_t* refs, const cppon& options);
```

Streaming
```cpp
printer& operator<<(printer& pr, const cppon& value);
std::ostream& operator<<(std::ostream& os, const cppon& value); // compact output by default
```

Notes
- Use to_string_view(...) + stream insertion if you need pretty / JSON layout without an intermediate std::string:

```cpp
std::cout << to_string_view(doc, R"({"layout":{"json":true,"pretty":true}})")
```

Common options (object form)
```json
{"pretty":true}
{"compact":true}
{"layout":{"json":true}}     // JSON‑compatible output
{"layout":{"flatten":true}}  // dereference references where possible
```

Buffer control
```json
{"buffer":"retain"}
{"buffer":{"reset":true,"reserve":true}}
```

JSON compatibility
- Enforces double‑precision integer safety range.
- Throws `json_compatibility_error` on out‑of‑range 64‑bit integers in JSON‑compat mode.

---

## Paths and References
```cpp
reference_vector_t resolve_paths(cppon& root);  // path_t → pointer_t (in‑place)
void restore_paths(const reference_vector_t& refs);
```

Helpers
```cpp
bool is_pointer_cyclic(pointer_t p);
std::string find_object_path(cppon& from, pointer_t object);
```

Notes
- Keep the returned `reference_vector_t` alive while optimized pointers are used (to allow restore).
- path_t is always absolute (must start with '/'); otherwise invalid_path_error is thrown at construction.
- `"$cppon-path:/"` is valid and resolves to the root (visitor(obj, "") returns obj).
- See [PATHS.md](PATHS.md) for absolute/relative access, root semantics, and best practices.

---

## Visitors and root

Root management (thread‑local)
```cpp
struct root_guard {
    explicit root_guard(const cppon& root);
    ~root_guard();
};
```

Note
- Internal traversal helpers (namespace ch5::visitors) are not part of the public API and may change without notice. See docs/internals/.

---

## SIMD control (runtime)
```cpp
enum class SimdLevel { None, SSE, AVX2, AVX512 };
void set_global_simd_override(SimdLevel level);
void clear_global_simd_override();
void set_thread_simd_override(SimdLevel level);
void clear_thread_simd_override();
SimdLevel effective_simd_level();
```

Notes
- Thread override takes precedence over global; clearing the thread override falls back to the global override; if none, auto‑detect applies.

---

## Version
```cpp
int cppon_version_major();
int cppon_version_minor();
int cppon_version_patch();
const char* cppon_version_string();
unsigned cppon_version_hex(); // 0x00MMmmpp
```

---

## Exceptions (selection)

Scanner and encoding
- unexpected_utf32_BOM_error
- unexpected_utf16_BOM_error
- invalid_utf8_sequence_error
- invalid_utf8_continuation_error
- unexpected_end_of_text_error
- unexpected_symbol_error
- expected_symbol_error

Parser and blobs
- invalid_base64_error
- blob_not_realized_error
- number_not_converted_error

Visitor and access
- null_value_error
- type_mismatch_error
- member_not_found_error
- bad_array_index_error
- invalid_path_segment_error
- invalid_path_error
- excessive_array_resize_error

Printer and options
- bad_option_error
- json_compatibility_error

Logic
- unsafe_pointer_assignment_error

---

## Configuration macros (see include/cppon/c++on.h)

Behavior
- CPPON_ENABLE_SIMD             // opt‑in SIMD (absence = scalar-only)
- CPPON_TRUSTED_INPUT           // opt‑in
- CPPON_ENABLE_STD_GET_INJECTION

Prefixes and sizing
- CPPON_PATH_PREFIX
- CPPON_BLOB_PREFIX
- CPPON_MAX_ARRAY_DELTA
- CPPON_PRINTER_RESERVE_PER_ELEMENT
- CPPON_OBJECT_MIN_RESERVE
- CPPON_ARRAY_MIN_RESERVE

See ./CONFIGURATION.md for details (including “Trusted input” and whitespace handling).

---

## Variant and NumberType indexes (reference)

Note
- These indexes describe the current layout of std::variant alternatives and NumberType values.
- They are implementation details and may change between releases. Prefer std::holds_alternative/std::get.

Variant alternatives (value_t)
- 0: object_t
- 1: array_t
- 2: double_t
- 3: float_t
- 4: int8_t
- 5: uint8_t
- 6: int16_t
- 7: uint16_t
- 8: int32_t
- 9: uint32_t
- 10: int64_t
- 11: uint64_t
- 12: number_t
- 13: boolean_t
- 14: string_view_t
- 15: blob_string_t
- 16: string_t
- 17: path_t
- 18: blob_t
- 19: pointer_t
- 20: nullptr_t

NumberType (textual numeric token number_t)
- 0: int64  (no suffix)
- 1: double (no suffix, decimal point and/or exponent)
- 2: float  (suffix f/F; decimal point and/or exponent)
- 3: int8   (suffix i8)
- 4: uint8  (suffix u8)
- 5: int16  (suffix i16)
- 6: uint16 (suffix u16)
- 7: int32  (suffix i32)
- 8: uint32 (suffix u32)
- 9: int64  (suffix i or i64)
- 10: uint64 (suffix u or u64)
