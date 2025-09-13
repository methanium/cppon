# FAQ

First public release: this FAQ lists the most likely questions. Open an issue if something is missing.

## What is C++ON?
A header‑only C++17 library to parse/print JSON with typed numbers, access paths, and in‑document references. See ABOUT and INTRODUCTION.

## Which compilers and platforms?
GCC 7+, Clang 6+, MSVC 2019+. Windows, Linux, macOS. Standard: C++17.

## How to install?
Add `include/` to your include paths and `#include <cppon/c++on.h>`.  
A single‑header is available at `single_include/cppon/c++on.h`.

## Does it use SIMD?
Yes, optionally (compile-time opt-in + runtime detection: SSE / AVX2 / AVX‑512 when available). You can override at runtime:
- set_global_simd_override(...), clear_global_simd_override()
- set_thread_simd_override(...), clear_thread_simd_override()
- effective_simd_level()

Scalar build: do not define `CPPON_ENABLE_SIMD` (SIMD code excluded; overrides become no‑ops; effective level = None).

## What is “Trusted input”?
`CPPON_TRUSTED_INPUT` makes whitespace detection branchless by treating ASCII control bytes [0x01..0x20] as whitespace (`'\0'` is never whitespace). Faster on “spaced” JSON; more permissive than strict JSON. Enable only for controlled inputs. See CONFIGURATION.

## Which numeric types and suffixes?
- JSON‑compatible: int64 (no suffix), double (decimal/exponent).
- Extended C++ types via suffixes: `i8/u8, i16/u16, i32/u32, i64/u64, float (f/F)`.  
See “Numeric suffixes” in API_REFERENCE.

## JSON‑compatible printing?
Use options: `{"layout":{"json":true}}`.  
Enforces double‑precision integer safety range; out‑of‑range 64‑bit integers raise `json_compatibility_error`.

## Paths vs pointers — which one?
- path_t (text): stable across copies/reshape, good at API boundaries.
- pointer_t: fast in‑memory links when addresses are stable.
- Convert with `resolve_paths(doc)` and revert with `restore_paths(refs)`.
- Flatten at print: `{"layout":{"flatten":true}}` (cycles preserved as paths). See PATHS.

## Path validation (`invalid_path_error`)
- `path_t` is always absolute: it must start with `'/'`. Empty or relative paths throw `invalid_path_error` at construction.
- Parser: strings with the `CPPON_PATH_PREFIX` (default `"$cppon-path:"`) must be followed by an absolute path. Example:
  - OK: `"$cppon-path:/a/b"`
  - OK: `"$cppon-path:/"` (resolves to the current root)
  - Error: `"$cppon-path:a/b"` or `"$cppon-path:"` → `invalid_path_error`
- Literals: the `"_path"` UDL also requires a leading `'/'`:
  - `"/a/b"_path` is OK; `"a/b"_path` throws `invalid_path_error`.
 
## Blobs and Base64
`"$cppon-blob:..."` remains `blob_string_t` until realized.  
`get_blob(non‑const)` decodes to `blob_t`; `get_blob(const)` throws `blob_not_realized_error` if not realized.

## Thread‑safety and root context
State is thread‑local. The “root” stack is per‑thread.  
Absolute paths (`"/..."`) use the active root. Use `root_guard` to pin a root when interleaving accesses across documents. See PATHS.

## Error handling
Precise exceptions:
- Scanner/encoding: `unexpected_*`, `invalid_utf8_*`
- Parser/blobs: `invalid_base64_error`, `blob_not_realized_error`, `number_not_converted_error`
- Visitor/logic: `type_mismatch_error`, `member_not_found_error`, `bad_array_index_error`, `invalid_path_segment_error`, `invalid_path_error`, `excessive_array_resize_error`
- Printer: `bad_option_error`, `json_compatibility_error`
- Logic: `unsafe_pointer_assignment_error`  
See API_REFERENCE for the complete list.

## Performance and benchmarks
A benchmark harness is provided. Recommended MSVC Release flags: `/O2 /GL /LTCG` (optionally `/arch:AVX2`).  
Perf summary is in README; more details in PERFORMANCE (coming).

## Versioning
Semantic Versioning (SemVer). Runtime helpers: `cppon_version_string()`, `cppon_version_major()/minor()/patch()`, `cppon_version_hex()`.  
See CHANGELOG for changes.

## License and contributions
MIT License. Contributions welcome (CONTRIBUTING, CODE_OF_CONDUCT).  
Questions or issues: open a GitHub issue. Contact details: see CONTACT.

Useful links
- Basic usage: ./BASIC_USAGE.md
- API Reference: ./API_REFERENCE.md
- Configuration: ./CONFIGURATION.md
- Paths & References: ./PATHS.md
- Internals: ./internals/INDEX.md