# Alternatives internals (value model)

This file documents the core value model (`value_t`), its ordering, invariants, and rationale.

## Goals
- Compact in-memory representation for typical small JSON objects.
- Lazy numeric conversion (avoid double parse).
- Zero-copy strings where possible (string_view_t).
- Explicit reference forms (path_t textual, pointer_t in-memory).
- Safe incremental extension (clear insertion points in the variant).

## Variant ordering (current)

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

Ordering is not a stable ABI guarantee. Motivations:

- Containers first (frequent dynamic checks in traversal).
- Concrete numerics grouped for arithmetic folds.
- `number_t` near concrete numerics (lazy → eager transition).
- Views before owning forms (string_view_t vs string_t, blob_string_t vs blob_t).
- Reference-like forms grouped (path_t, blob_t, pointer_t).
- `nullptr_t` last (easy null test).

## Key alternative roles

| Type            | Purpose                              | Notes |
|-----------------|--------------------------------------|-------|
| object_t        | Ordered associative storage          | Vector of (string_view_t, cppon) pairs |
| array_t         | Sequential values                    | Amortized growth |
| number_t        | Textual numeric token + NumberType   | Converted on demand |
| string_view_t   | Non-owning UTF-8 view                | Valid while source buffer lives |
| string_t        | Owning UTF-8                         | Materialized only when needed |
| blob_string_t   | Base64 token                         | Decoded lazily |
| blob_t          | Decoded binary                       | Realized on demand |
| path_t          | Absolute document path               | Always starts with '/' |
| pointer_t       | Direct link to another node          | Non-owning |
| nullptr_t       | Explicit null                        | Thread-local sentinel used for misses |

## NumberType enumeration

| Kind          | Trigger                                          |
|---------------|--------------------------------------------------|
| json_int64    | Integer literal (no suffix / no '.' / no exp)    |
| json_double   | '.' or exponent (no float suffix)                |
| cpp_float     | 'f' / 'F' suffix (decimal or exponent)           |
| cpp_int8..64  | Explicit signed suffix (i8 / i16 / i32 / i / i64)|
| cpp_uint8..64 | Explicit unsigned suffix (u8 / u16 / u32 / u / u64)|

Suffix-less large integer (fits 64-bit) defaults to json_int64.

## Conversion & utility helpers

| Helper | Signature | Role | Notes |
|--------|-----------|------|-------|
| convert_to_numeric | void convert_to_numeric(value_t&) | In‑place promotion of a number_t into its concrete numeric alternative | Throws type_mismatch_error if not number_t / numeric |
| encode_base64 | string_t encode_base64(const blob_t&) | Encode binary blob into base64 textual form | Used by printer when emitting blob_t |
| decode_base64 | blob_t decode_base64(string_view_t, bool raise=true) | Decode textual base64 into blob_t | If raise=false returns empty blob on invalid input |

## Invariants

| Invariant                            | Enforced by                   |
|-------------------------------------|-------------------------------|
| path_t non-empty + starts with '/'  | path_t constructor            |
| number_t never converted twice      | Replaced in-place on convert  |
| string_view_t points into input     | Caller ensures lifetime       |
| pointer_t intra-document only       | User discipline               |

Invariant refinement:
- After a successful `convert_to_numeric`, a former `number_t` index in the variant is replaced by its concrete numeric alternative; no second conversion occurs.
- `decode_base64` never mutates in place; mutation (realization) is performed by `get_blob` inside visitors.

## Access helpers

All typed retrieval, lazy number promotion, blob realization and optional lookup helpers live in `visitors`:

See: [VISITORS.md](VISITORS.md) / “Value getters” (get_strict, get_cast, get_blob, get_optional).

## Error conditions (indirect)

| Situation                            | Exception                     |
|-------------------------------------|-------------------------------|
| Bad path literal                    | invalid_path_error            |
| Wrong container access              | type_mismatch_error           |
| Strict numeric access on lazy const | number_not_converted_error    |
| Blob access (const, not realized)   | blob_not_realized_error       |

## Performance notes
- object_t linear lookup beats hashing for ≤ ~16 members (cache locality).
- number_t avoids unnecessary strto* during parse-only or lazy reads.
- string_view_t reduces copies for common short-lived inspection.

## Extension guidelines
1. Add new alternative to variant (end preferred).
2. Provide print(...) overload (printer).
3. If container-like: add traversal semantics (visitors).
4. If it affects dereferencing or realization (like blob or number): update visitors’ getters accordingly.
5. Update docs (this file + API reference).
