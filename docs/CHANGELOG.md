# Changelog

All notable changes to this project are documented here.
Format: Inspired by Keep a Changelog (https://keepachangelog.com) and SemVer.

## [Unreleased]

### Added
- Internal documentation: `docs/internals/VISITORS.md` (deref semantics, autovivification rules).
- Opt-in macro: `CPPON_ENABLE_STD_GET_INJECTION` (unqualified std::get / get_if usage).
- `invalid_path_error` (thrown on malformed `path_t`: empty or missing leading '/').

### Changed
- `path_t` invariant strengthened (always absolute); `_path` literal enforces same rule.
- `deref_if_ptr`: explicit support for `"$cppon-path:/"` (resolves to root).
- Internal: clarified deref helpers (`deref_if_ptr` / `deref_if_not_null`) and absolute path handling in docs.
- Documentation: wording, numeric clarification, configuration clarifications.
- Internals index updated to reference new visitor documentation.

### Fixed
- Removed a stray empty bullet in documentation.
- Corrected a mistyped numeric value in docs (no code impact).
- Fixed broken links in `API_REFERENCE.md` and `LITERALS.md`.

### Removed
- Obsolete macro reference: `CPPON_NO_UNSAFE_POINTER_` (cleanup in docs).

### Breaking
- Renamed `version_*` helpers to `cppon_version_*` (`cppon_version_major/minor/patch/string/hex`).

---

## [0.1.0] - 2025-09-11
### Added
- Initial public release: header-only C++17 JSON parser with paths, in‑document references, typed numeric extensions, blobs (lazy base64 decode).
- SIMD detection + runtime overrides (SSE / AVX2 / AVX‑512 when available).
- “Trusted input” mode (optional, branchless whitespace).
- Documentation bootstrap: README, INTRODUCTION, CONFIGURATION, PATHS, Internals index, API reference skeleton, examples/tests/benchmarks stubs.

### Notes
- Prototype origins (internal use) date back to 2019.
- Codebase refactored progressively to reach this first public drop.
