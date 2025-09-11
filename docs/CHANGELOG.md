# Changelog

All notable changes to this project will be documented in this file.

## [0.1.0] - 2025-09-05
- Initial public release.
- Header-only C++17 JSON parser with paths, references, typed numbers, blobs.
- SIMD detection and optional overrides; “Trusted input” mode (opt-in).
- Documentation kick-off (README, INTRODUCTION, CONFIGURATION, PATHS, Internals).

History note:
- The first prototype of C++ON dates back to 2019; this marks the first public release.
- The library has been used internally in various projects since then.
- The codebase has been refactored and improved over time, leading to this stable release.

## Unreleased
- Namespace: cppon → ch5
- Opt-in: CPPON_ENABLE_STD_GET_INJECTION
- Removed: CPPON_NO_UNSAFE_POINTER_
- Docs: index + initial stubs (examples/tests/benchmarks)

### Breaking
- Renamed version_* helpers to cppon_version_* (cppon_version_major/minor/patch/string/hex).

### Added
- invalid_path_error: thrown for malformed path_t (empty or missing leading '/').

### Changed
- deref_if_ptr: explicit support for "$cppon-path:/" (resolves to root via visitor(obj, "")).
- path_t: invariant strengthened (always absolute); _path literal reflects the same contract.
- Internal: clarified deref semantics (visitors::deref_if_ptr / visitors::deref_if_not_null) and absolute path handling.
- No public API changes.
