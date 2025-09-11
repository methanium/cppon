# Glossary

- cppon
  The main value class. Holds a tagged union (see value_t) and provides path/index accessors and helpers.

- value_t
  The internal std::variant of all supported alternatives (object_t, array_t, strings, numbers, paths, pointers, blobs, …).

- object_t
  Order‑preserving object container: vector of members (key: std::string_view, value: cppon).

- array_t
  Sequential container of cppon values (vector<cppon>).

- string_view_t
  Non‑owning UTF‑8 view into external storage (valid while the backing buffer lives).

- string_t
  Owning UTF‑8 string.

- number_t
  Lazy numeric token storing a string slice plus a NumberType tag (e.g., json_int64, json_double, cpp_int32…). Converted on demand.

- numeric concrete types
  float_t, double_t, i8/u8, i16/u16, i32/u32, i64/u64. JSON‑compatible forms are int64 (no suffix) and double.

- path_t
  Textual reference token (e.g., "$cppon-path:/a/b/0"). Stable across copies/reshape.

- invalid_path_error
  Thrown when constructing an invalid path_t (empty or not starting with '/').

- pointer_t
  Direct in‑memory pointer to a node in the same cppon document. Fast but requires address stability.

- reference_vector_t
  The collection returned by resolve_paths(doc). Keeps bookkeeping to restore path_t from pointer_t via restore_paths(refs).

- blob_string_t
  Base64 textual blob token (e.g., "$cppon-blob:…"). Realized on demand.

- blob_t
  Realized binary blob (byte container).

- options
  Parsing/evaluation mode: full (decode blobs), eval (keep blob_string_t), quick (lazy numbers), parse (validation only).

- SimdLevel
  Runtime SIMD level: None, SSE, AVX2, AVX512. Auto‑detected, can be overridden (globally or per thread).

- root_guard
  RAII helper to pin a specific cppon as the active root within a scope (root stack is thread‑local).

- visitor
  Helper to navigate from a base node using an index or a single path segment (relative traversal).

- resolve_paths / restore_paths
  Converts path_t → pointer_t in place for faster deref, then reverts pointers back to textual paths.

- JSON‑compatible layout
  Printer option (`{"layout":{"json":true}}`) enforcing JSON compatibility (e.g., integer safety range for doubles). Throws json_compatibility_error on violations.

- flatten (printing)
  Printer option (`{"layout":{"flatten":true}}`) trying to inline referenced content. Cycles remain as paths.

- “Trusted input”
  Build‑time macro (CPPON_TRUSTED_INPUT) making whitespace detection branchless by accepting ASCII controls [0x01..0x20] as whitespace (except '\0'). Faster, more permissive than strict JSON.

- CPPON_NO_SIMD
  Build‑time macro that excludes all SIMD code (scalar only). Overrides become no‑ops; effective SIMD level is None.

- CPPON_PATH_PREFIX / CPPON_BLOB_PREFIX
  Build‑time prefixes for path and blob tokens ("$cppon-path:", "$cppon-blob:").

- CPPON_OBJECT_MIN_RESERVE / CPPON_ARRAY_MIN_RESERVE
  Initial reserve hints used during materialization.

- CPPON_PRINTER_RESERVE_PER_ELEMENT
  Printer’s per‑element reserve hint to reduce reallocations.

- CPPON_MAX_ARRAY_DELTA
  Guard against accidental sparse growth when writing arrays. Writes that jump forward beyond this delta throw excessive_array_resize_error (default: 100).

- effective_simd_level
  Returns the currently effective SIMD level (after considering detection and overrides).