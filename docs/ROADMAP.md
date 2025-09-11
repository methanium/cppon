# Roadmap

This roadmap is pragmatic and time‑boxed. It focuses on shipping v0.1.0, stabilizing APIs, and preparing CI and docs.

## v0.1.0 — First public release (short term)

- Documentation
  - Finalize: INTRODUCTION, BASIC_USAGE, PATHS, CONFIGURATION, API_REFERENCE, FAQ, GLOSSARY
  - Internals index: minimal “fast paths / SIMD / root stack” overview
  - README polish (badges later), CHANGELOG entry
- Examples
  - Ensure all examples build/run (basic_usage, formatting_options, references_example, binary_blobs)
- Tests
  - Smoke tests: parsing happy paths + errors (UTF‑8/BOM, numbers, strings, arrays/objects)
  - SIMD: global/thread overrides + NO_SIMD TU (GoogleTest)
  - JSON‑compat printing limits (int range)
- CI (initial)
  - GitHub Actions: windows‑latest (MSVC), ubuntu‑latest (GCC), macos‑latest (Clang)
  - Build Release C++17 + run tests
- Packaging & version
  - Generate single_include/cppon/c++on.h
  - Tag v0.1.0, publish release notes (link README/CHANGELOG)

## v0.2.0 — Quality and coverage

- Docs
  - PERFORMANCE page (methodology, harness usage, interpretation)
  - More “How‑to” snippets (paths/pointers/flatten; blobs; error handling)
- Tests
  - Increase coverage on edge cases (numeric suffixes, path errors, blobs)
  - Sanitizers (ASan/UBSan) on Linux/Clang
  - Basic fuzzing entry point (parse‑only)
- CI
  - Matrix with toolchain versions
  - Cache and artifacts (single include, example binaries)

## v0.3.0 — Options and ergonomics

- Printer
  - Options consolidation (buffer strategies, pretty/compact combos)
  - More JSON‑compat toggles if needed
- API
  - Helper sugar (safe getters, small utilities) without breaking changes
- Performance
  - Microbenchmarks per hot path (quote scan, digits, skip_spaces)

## v1.0.0 — API stability

- Finalize public API (types, functions, options)
- SemVer guarantee; migration guide (if needed)
- Full docs pass; examples curated; CI green across platforms

## Backlog (nice‑to‑have, post‑1.0)

- ARM64/NEON investigation (conditional SIMD backends)
- Streaming/SAX (if demand is clear; out‑of‑scope initially)
- More integrations (pkg-config/vcpkg/conan recipes)
- Extended diagnostics (error messages with context slices)

Notes
- SIMD overrides are optional and primarily for benchmarking; NO_SIMD build supported (overrides become no‑ops).
- “Trusted input” is opt‑in and documented (whitespace semantics).
- Bench harness is frozen by default; advanced metrics guarded by macros.