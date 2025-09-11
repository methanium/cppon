# C++ON Internals Index

This section documents implementation details that are not part of the public API. Anything here may change between minor versions.

## Files overview

| Topic | File | Focus |
|-------|------|-------|
| Value model & variant | [ALTERNATIVES.md](ALTERNATIVES.md) | Ordering, NumberType, invariants |
| Core wrapper & root | [TYPES.md](TYPES.md) | cppon class, root stack usage, autovivification |
| Traversal & deref | [VISITORS.md](VISITORS.md) | Path walking, deref semantics, write vs read |
| Parsing pipeline | [PARSER.md](PARSER.md) | Modes, number handling, UTF-8 guards |
| Printing engine | [PRINTER.md](PRINTER.md) | Layout, compaction, JSON compatibility, flatten |
| Scanner & SIMD | [SCANNER.md](SCANNER.md) | SWAR primitives, SIMD dispatch, safety |
| Path ↔ pointer optimization | [REFERENCES.md](REFERENCES.md) | resolve_paths / restore_paths, cycles |

---

## Quick reference (selected internals)

### Dereferencing helpers (see VISITORS)
- `deref_if_ptr`:
  - pointer_t → value or `null()`
  - path_t → resolves against current root (drops leading '/')
  - others → passthrough
- `deref_if_not_null` (non-const):
  - Null pointer slot returns the slot itself (enables autovivification there)

### Root stack (see TYPES / VISITORS)
- Thread-local vector with bottom sentinel (nullptr)
- Uniqueness enforced by hoisting
- Destructor `~cppon()` pops if present

### Scanner (see SCANNER)
- `find_quote_pos(text, start)`
- `scan_digits(text, start)` (returns pointer to first non-digit or sentinel)
- NUL-terminated buffer contract required

### Parser fast paths (see PARSER)
- Lazy number (`number_t`) in quick mode
- Immediate conversion in eval/full
- Blob prefix `$cppon-blob:` → blob_string_t or blob_t (full)
- Path prefix `$cppon-path:/...` → path_t

### Printer (see PRINTER)
- JSON-compatible mode: enforce 53-bit integer safety
- Compaction modes: global / label-based
- Flatten: inline pointer targets unless cyclic
- Thread-local retained buffer (optional)

### References (see REFERENCES)
- `resolve_paths`: path_t → pointer_t (storing original path + slot)
- `restore_paths`: pointer_t → path_t restoration
- Cycle detection for safe flatten

### SIMD runtime (see SCANNER)
- Levels: None / SSE / AVX2 / AVX512
- Overrides: thread > global > auto-detect
- Capped to CPU support

### Array growth guard
- `CPPON_MAX_ARRAY_DELTA` limits sparse jumps (throws `excessive_array_resize_error`)

---

## Exception hotspots

| Area | Key exceptions |
|------|----------------|
| Scanner/encoding | unexpected_utf16_BOM_error, invalid_utf8_* |
| Parser structure | unexpected_symbol_error, unexpected_end_of_text_error |
| Traversal | member_not_found_error, null_value_error, type_mismatch_error |
| Arrays | bad_array_index_error, excessive_array_resize_error |
| Printer | bad_option_error, json_compatibility_error |
| Numbers | number_not_converted_error (strict const lazy number) |
| Blobs | blob_not_realized_error |
| Paths | invalid_path_error |
| Logic | unsafe_pointer_assignment_error |

---

## Planned additions
- Extended string escaping strategy
- Large object auxiliary index heuristics
- Optional streaming traversal hooks
- UTF-8 deeper validation trade-offs

(See individual files for deeper detail.)
