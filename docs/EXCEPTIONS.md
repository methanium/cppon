# Exceptions

Overview of exceptions thrown by C++ON, grouped by area. Unless noted otherwise, they derive from `std::runtime_error`.

Tip
- Catch broad categories at API boundaries (parse/print) and more specific ones in unit tests.

## Scanner (input/UTF-8)

- **unexpected_utf32_BOM_error**<br>
  UTF‑32 (BOM) input not supported.
- **unexpected_utf16_BOM_error**<br>
  UTF‑16 (BOM) input not supported.
- **invalid_utf8_sequence_error**<br>
  Invalid UTF‑8 sequence (e.g., bytes 0xF8–0xFD).
- **invalid_utf8_continuation_error**<br>
  Continuation byte found at start position.
- **unexpected_end_of_text_error**<br>
  End of text reached while a token was expected (e.g., unterminated string).
- **unexpected_symbol_error**<br>
  Unexpected symbol encountered (includes character and position).
- **expected_symbol_error**<br>
  Expected symbol missing (includes character and position).

Where
- `parser::eval(...)`, `accept_string/number/object/array`, `skip_spaces`.

## Parser / Blobs / Numbers

- **invalid_base64_error**<br>
  Base64 decoding error.
- **blob_not_realized_error**<br>
  `get_blob(const)` called on a non‑decoded `blob_string_t`.
- **number_not_converted_error**<br>
  Strict numeric access in const context on an unconverted `number_t`.

Where
- `parser::decode_base64`, `get_blob(...)`, `get_strict(const)`.

## Visitors / Access (paths, indices)

- **null_value_error**<br>
  Dereferencing a path/pointer that resolves to `null`.
- **type_mismatch_error**<br>
  Unexpected type (e.g., expected `array_t`/`object_t`).
- **member_not_found_error**<br>
  Missing key during object access (non‑terminal).
- **bad_array_index_error** (std::out_of_range)<br>
  Invalid array index (non‑numeric segment).
- **invalid_path_segment_error**<br>
  Invalid path segment.
- **invalid_path_error**<br>
  Invalid full path value (e.g., empty or missing leading '/'); thrown at `path_t` construction.
- **excessive_array_resize_error**<br>
  Array growth exceeds defensive bound `CPPON_MAX_ARRAY_DELTA`.

Where
- `visitor(...)`, `cppon::operator[]`, path resolution.
- `path_t` construction (parser path tokens, `"_path"` literal).

## Printer (format/output)

- **bad_option_error**<br>
  Invalid print option or type (e.g., `{"buffer":{"reset":123}}`).
- **json_compatibility_error**<br>
  Out of safe JSON integer range (64‑bit) in compatibility mode.

Where
- `to_string(...)` and option parsing, `print(...)` (integers).

## Logic / Safety

- **unsafe_pointer_assignment_error** (std::logic_error)<br>
  Assigning a `pointer_t` to an object that is `valueless_by_exception` (unsafe).

Where
- `cppon::operator=(pointer_t)`.

## Best practices

- Parsing
  - `try { auto v = ch5::eval(text); } catch (const std::runtime_error& e) { ... }`
  - For precise tests: catch `unexpected_*`/`invalid_utf8_*` earlier.
- Access
  - Prefer `std::get_if<T>`/`get_optional<T>` to avoid exceptions on tentative paths.
  - For arrays, validate indices; tune `CPPON_MAX_ARRAY_DELTA` if needed.
- Printing
  - Validate option types in tests; in JSON mode, check 64‑bit integer bounds.

References
- [API](API_REFERENCE.md): full function and exception list.
- [Configuration/macros](CONFIGURATION.md): macros affecting behavior.
- [Paths/References](PATHS.md): usage and exceptions.