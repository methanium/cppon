# C++ON Internals Index

This section documents implementation details that are not part of the public API. Symbols in `ch5::visitors` and other internals may change without notice.

## Visitors and deref semantics

- Overview and rationale, including read vs write flows and autovivification rules:
  - See [VISITORS.md](VISITORS.md))

Quick summary
- deref_if_ptr (const/non-const, internal)
  - pointer_t: returns `*arg` or `null()` if the pointer is null
  - path_t: resolves against `get_root()`, unconditionally removes the leading '/'
  - others: passthrough (returns the original object)
- deref_if_not_null (non-const only, internal)
  - If the slot holds a null pointer_t, returns the slot itself (autovivify at the slot)
  - Otherwise delegates to deref_if_ptr

## Root stack and thread-local storage

- `static_storage` holds:
  - A thread-local root stack (vector<cppon*>), initialized with a bottom sentinel (nullptr)
  - A per-thread `Null` sentinel returned by `visitors::null()`
- Invariants:
  - `hoist_if_found` promotes an existing entry to the top (O(n) worst-case, small n in practice)
  - The root stack is never empty; `get_root()` asserts top != nullptr
  - `push_root` enforces uniqueness (no duplicates); `pop_root` removes only if found

## Scanner API (low-level)

Low-level helpers used by the parser; available to advanced users in `ch5::scanner`.

```cpp
namespace ch5::scanner {
enum class SimdLevel { None, SSE, AVX2, AVX512 };

// Find the next '"' position starting at 'start'; returns npos if not found
size_t find_quote_pos(std::string_view text, size_t start = 0) noexcept;

//Digit scanning from 'start'; returns pointer to the first non-digit (may be the '\0' sentinel)
const char* scan_digits(std::string_view text, size_t start = 0) noexcept;
}
```

Contract
- Inputs must be backed by a NUL-terminated buffer (readable sentinel at `data()+size()`).
- Quote search is strictly bounded to the requested window.
- Digit scanning uses a scalar epilogue that peeks exactly one extra byte to detect the `'\0'` sentinel.

## SIMD ENABLED build (opt‑in)

Define CPPON_ENABLE_SIMD to include all SIMD code to the build:

- Opt-in to SIMD paths (SSE/AVX2/AVX-512 if available at runtime).
- Without it (or on unsupported targets), scalar paths are used. Runtime overrides are still available but may be no-ops.

## NO SIMD build (default)

Do not define CPPON_ENABLE_SIMD to exclude all SIMD code from the build:

- SIMD headers are not included; only scalar paths are compiled.
- Overrides become no‑ops; effective_simd_level() returns None.

Runtime overrides

```cpp
// No-ops in NO_SIMD builds; effective_simd_level() returns None there

void set_global_simd_override(ch5::scanner::SimdLevel) noexcept;
void clear_global_simd_override() noexcept;

void set_thread_simd_override(ch5::scanner::SimdLevel) noexcept;
void clear_thread_simd_override() noexcept;

ch5::scanner::SimdLevel effective_simd_level() noexcept;
```

## Whitespace and hot paths

- Skip spaces
  - Strict vs Trusted mode (`CPPON_TRUSTED_INPUT`): strict = SP/TAB/LF/CR; trusted = [0x01..0x20], never '\0'
  - Sentinel-based loop: assumes null-terminated buffers; no out-of-bounds reads
  - Minimal loop: updates `pos` only if whitespace was consumed (fast path without stores)
- Digits scanning
  - SIMD when beneficial (xmm/ymm/zmm) with bounded epilogues
  - Scalar fast path: branchless ASCII test `unsigned(*p - '0') <= 9`
  - Avoids `std::isdigit` (locale/branches)
- SIMD strategy
  - Runtime detection (SSE/AVX2/AVX512) with global/thread overrides
  - Lazy binding via function pointers; epoch-based rebinding on override change
  - No SIMD for `skip_spaces` (cost/benefit unfavorable)
  - Recommended: Release `/O2 /GL /LTCG /arch:AVX2` (or platform equivalent)
- Errors and safety
  - `unexpected_end_of_text_error`: thrown if end reached after whitespace-only tail
  - Requirement: input buffers must be null-terminated (guaranteed by cppon)
- Thread-safety
  - SIMD overrides are thread-local; parser state is isolated via `thread_local`
- Performance notes
  - “Spaced” JSON benefits from trusted mode (branchless whitespace)
  - Compact JSON keeps hot paths short; stable perf without downclock

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

## Parser fast paths

- Strings
  - `accept_string`: uses `scanner::find_quote_pos` (SIMD or scalar), handles escapes by counting trailing backslashes
- Numbers
  - `accept_number`: uses `scanner::scan_digits` (SIMD or scalar) on integer and fractional parts; suffix/exponent handling; lazy `number_t` materialization; conversion on demand (Eval/Full)
- Encoding checks
  - BOM guards; basic UTF-8 validity checks for first byte
- Options
  - Modes: `full`, `eval`, `quick`, `parse` (validation only)

## Printer internals

- `details::printer_state` (thread-local) persists buffers/options across calls when requested
- Options:
  - Pretty / alternative layout, compacting by label, JSON-compatible layout
  - Buffer control (reset/retain/reserve)
- JSON-compatible limits
  - Enforces IEEE-754 double-safe integer range for 64-bit values; throws `json_compatibility_error` on violations
- Blobs
  - `blob_string_t` vs `blob_t` printing; base64 encode on demand

## SIMD dispatch controls

- `SimdLevel { None, SSE, AVX2, AVX512 }`
- Overrides:
  - Global (atomic) and thread-local overrides, with epoch-driven rebinding
  - `effective_simd_level()` reflects detection after overrides

## Testing and safety nets

- Null sentinel usage in read-only paths avoids exceptions; callers decide whether to throw (`null_value_error`, `member_not_found_error`, …)
- Non-const visitors handle structure creation conservatively and guard sparse array growth via `CPPON_MAX_ARRAY_DELTA`

---
Planned additions:
- More detailed notes on printer compaction heuristics
- Extended UTF-8 validation trade-offs
- Additional perf footnotes for large objects (member lookup patterns)