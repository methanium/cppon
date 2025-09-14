# Changelog

All notable changes to this project are documented here.
Format: Inspired by Keep a Changelog (https://keepachangelog.com) and SemVer.

## [Unreleased]

### Added
- `document` class: High-level wrapper that owns the input buffer, ensuring string_view/blob_string_t validity. Provides lifecycle management, file I/O (`from_file`, `to_file`), and serialization methods. Extends ABI without affecting existing code.
- `file_operation_error` exception to support file I/O operations with clear error messages.- Internal documentation: `docs/internals/VISITORS.md` + other internals (`ALTERNATIVES.md`, `PARSER.md`, `PRINTER.md`, `SCANNER.md`, `REFERENCES.md`, `TYPES.md`).
- Opt-in macro: `CPPON_ENABLE_STD_GET_INJECTION` (unqualified std::get / get_if usage).
- `invalid_path_error` (thrown on malformed `path_t`: empty or missing leading '/').
- Initial public release: header-only C++17 JSON parser with paths, in‑document references, typed numeric extensions, blobs (lazy base64 decode).
- Documentation bootstrap: README, INTRODUCTION, CONFIGURATION, PATHS, Internals index, API reference skeleton, examples/tests/benchmarks stubs.
- “Trusted input” mode (optional, branchless whitespace).
- SIMD detection + runtime overrides (SSE / AVX2 / AVX‑512 when available).

### Changed
- Docs: clarified that flatten mode does not require reference_vector_t; Refs are an optional optimization for pointer path emission.
- Refactoring of internal sub‑namespaces (no public ABI impact; only direct `namespace ch5` symbols are considered stable).
- Documentation: wording, numeric clarification, configuration clarifications.
- Internal: clarified deref helpers (`deref_if_ptr` / `deref_if_not_null`) and absolute path handling in docs.
- Internals index updated to link new visitor documentation.
- `path_t` invariant strengthened (always absolute); `_path` literal enforces same rule.
- `deref_if_ptr`: explicit support for `"$cppon-path:/"` (resolves to root).

### Fixed
- Added critical safety checks to prevent undefined behavior when accessing objects in `valueless_by_exception` state
- Added `check_valid()` (noexcept) and `ensure_valid()` (throwing) methods to `cppon` class for robust state validation
- Fixed potential reference invalidation issues when modifying nested objects (e.g., `Options["buffer"]` then `Options["layout"]`)
- Enhanced robustness of `try_*` methods to safely handle invalid object states while preserving ABI compatibility
- Removed a stray empty bullet in documentation.
- Corrected a mistyped numeric value in docs (no code impact).
- Fixed broken links in `API_REFERENCE.md` and `LITERALS.md`.

### Removed
- Obsolete macro reference: `CPPON_NO_UNSAFE_POINTER_` (cleanup in docs).

### Breaking
- Renamed `version_*` helpers to `cppon_version_*` (`cppon_version_major/minor/patch/string/hex`).

---

### Notes

- Prototype origins (internal use) date back to 2019.
- Codebase refactored progressively to reach this first public drop.
