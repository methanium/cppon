# Changelog

All notable changes to this project are documented here.
Format: Inspired by Keep a Changelog (https://keepachangelog.com) and SemVer.

## [Unreleased]

## [0.2.0] - 2025-10-08

### Added
- Full C++20 compatibility support
  - Implicit constructors for all variant types to match C++17 behavior
  - Explicit comparison operators for custom types
  - String literal to string_view_t explicit conversions for comparisons
  - Dedicated print function for `string_t` type
- Dynamic configuration system with thread-local settings and automatic updates
- `config` class: Thread-safe configuration management with dirty flag tracking and automatic updates
- `exact_number_mode`: Configuration option to preserve exact numeric representation rather than converting to JSON-compatible types
- Array of updater functions for efficient configuration management without conditionals
- Unified configuration interface that works consistently across different output methods
- Global configuration system with automatic updates to dependent components
- `document` class: High-level wrapper that owns the input buffer, ensuring string_view/blob_string_t validity. Provides lifecycle management, file I/O (`from_file`, `to_file`), and serialization methods. Extends ABI without affecting existing code.
- `file_operation_error` exception to support file I/O operations with clear error messages.
- Internal documentation: `docs/internals/VISITORS.md` + other internals (`ALTERNATIVES.md`, `PARSER.md`, `PRINTER.md`, `SCANNER.md`, `REFERENCES.md`, `TYPES.md`).
- Opt-in macro: `CPPON_ENABLE_STD_GET_INJECTION` (unqualified std::get / get_if usage).
- `invalid_path_error` (thrown on malformed `path_t`: empty or missing leading '/').
- Initial public release: header-only C++17 JSON parser with paths, in‑document references, typed numeric extensions, blobs (lazy base64 decode).
- Documentation bootstrap: README, INTRODUCTION, CONFIGURATION, PATHS, Internals index, API reference skeleton, examples/tests/benchmarks stubs.
- “Trusted input” mode (optional, branchless whitespace).
- SIMD detection + runtime overrides (SSE / AVX2 / AVX‑512 when available).
- Ability to retrieve previous printer configuration when setting a new one
- Thread-local printer state management with automatic restoration via RAII

### Changed
- Reimplemented configuration system to use thread-local variables instead of macros for better flexibility
- Configuration changes now automatically trigger updates to relevant components (parser, scanner, memory)
- Enhanced operators for variant comparisons to improve compatibility between `cppon` and contained types
- Moved hardcoded settings to dynamic configuration system with sensible defaults
- Printer configuration is now shared across all output methods, including stream operators
- Improved consistency between `to_string()`, `to_string_view()` and stream output via `operator<<`
- Enhanced `printer` to use thread-local configuration state for all output operations
- `exact_number_mode` in printer now respects the global configuration setting by default
- Docs: clarified that flatten mode does not require reference_vector_t; Refs are an optional optimization for pointer path emission.
- Refactoring of internal sub‑namespaces (no public ABI impact; only direct `namespace ch5` symbols are considered stable).
- Documentation: wording, numeric clarification, configuration clarifications.
- Internal: clarified deref helpers (`deref_if_ptr` / `deref_if_not_null`) and absolute path handling in docs.
- Internals index updated to link new visitor documentation.
- `path_t` invariant strengthened (always absolute); `_path` literal enforces same rule.
- `deref_if_ptr`: explicit support for `"$cppon-path:/"` (resolves to root).

### Fixed
- Resolved C++20 compatibility issues with variant constructors and comparisons
- Fixed string literal comparisons with string_view_t for C++20
- Added explicit casts for type comparisons to work with C++20's stricter rules
- Resolved overload ambiguities introduced by C++20's spaceship operator
- Fixed comparison operators to properly handle type variants in different contexts
- Improved error handling for configuration-related operations
- Added safeguards to prevent invalid state propagation when updating configurations
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
