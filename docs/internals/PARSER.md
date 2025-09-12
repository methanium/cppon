# Parser internals

Covers JSON parsing pipeline, mode behavior, and numeric / reference token handling.

## Modes

| Mode  | Numeric Conversion        | Blobs                      | DOM Allocation |
|-------|---------------------------|----------------------------|----------------|
| parse | None (validation only)    | None                       | No (structure skipped) |
| quick | Lazy (number_t)           | blob_string_t (base64 text)| Yes            |
| eval  | Immediate (concrete types)| blob_string_t              | Yes            |
| full  | Immediate                 | blob_t (decoded)           | Yes            |

## Pipeline

1. Validate BOM / UTF-8 first-byte constraints.
2. Strip optional UTF-8 BOM.
3. skip_spaces (sentinel safe).
4. accept_value dispatch (string/number/object/array/literal).
5. Return any value (no enforced root object requirement).

## String parsing

- find_quote_pos (SIMD or SWAR) locates candidate.
- Escape handling: backward scan for '\\' parity.
- No transform of escape sequences (literal retention).
- Path / blob prefix detection ($cppon-path:/ , $cppon-blob:...).

## Number parsing

Phases:

1. Sign + zero special-case (leading zero check).
2. Integral digits via scan_digits (SIMD/SWAR).
3. Fraction part ('.' + digits).
4. Exponent ('e/E' + sign? + digits).
5. Type classification:
   - json_int64 / json_double
   - Suffix (i,u,i8,u16,...,f) → concrete NumberType
6. If opt < quick → convert_to_numeric.

Error conditions:

| Condition                | Exception                        |
|--------------------------|----------------------------------|
| EOF mid-number           | unexpected_end_of_text_error     |
| Invalid suffix sequence  | unexpected_symbol_error          |
| Non-digit where required | unexpected_symbol_error          |

## Blobs & paths

| Prefix            | Result (quick/eval) | full mode effect          |
|-------------------|--------------------|---------------------------|
| $cppon-path:/foo  | path_t("/foo")     | Same                      |
| $cppon-blob:XXXX  | blob_string_t      | Decoded to blob_t in full |

## Whitespace policy

| Mode                      | Definition                          |
|--------------------------|--------------------------------------|
| Strict                   | SP / TAB / LF / CR                   |
| CPPON_TRUSTED_INPUT      | Any 0x01..0x20 except '\0'           |

Branchless predicate under trusted mode: `(c - 1) < 0x20`.

## Parser fast paths

- Strings
  - `accept_string`: uses `scanner::find_quote_pos` (SIMD or scalar), handles escapes by counting trailing backslashes
- Numbers
  - `accept_number`: uses `scanner::scan_digits` (SIMD or scalar) on integer and fractional parts; suffix/exponent handling; lazy `number_t` materialization; conversion on demand (Eval/Full)
- Encoding checks
  - BOM guards; basic UTF-8 validity checks for first byte
- Options
  - Modes: `full`, `eval`, `quick`, `parse` (validation only)

## UTF-8 guards

Reject:
- UTF-32 / UTF-16 BOM (unsupported).
- 5 or 6 byte sequences (invalid).
- Continuation byte as first leading byte.

## Memory heuristics

- object_t.reserve(object_min_reserve)
- array_t.reserve(array_min_reserve)
Used only when materializing (non-parse modes).

## Access helpers

The parser does not expose value access/getter utilities.  
All typed retrieval, lazy number promotion, blob realization and optional lookup helpers live in `visitors`:

See: [VISITORS.md](VISITORS.md) / “Value getters” (get_strict, get_cast, get_blob, get_optional).

Rationale:
- Keeps parser focused on syntax + structural materialization.
- Centralizes indirection (path_t / pointer_t) resolution before transformations.

## Lazy numeric rationale

- Avoid double strto* cost for unused numeric fields.
- Allows cheap string forwarding in quick mode.

## Extension guidelines

To add a new literal form:
1. Extend accept_value switch.
2. Provide accept_* function.
3. Add variant alternative (if new type).
4. Update printer and docs.

## Interactions

- SCANNER: SIMD/SWAR quote & digit scanning.
