# About C++ON

C++ON is a high‑performance, header‑only C++17 library for parsing and generating JSON with extended features such as typed numbers, access paths, and in‑document references. It focuses on an intuitive API, predictable latency, and short hot paths, leveraging modern C++ and SIMD where it pays.

## Goals

- Make JSON parsing and printing fast enough for most workloads without sacrificing clarity.
- Provide an elegant, easy‑to‑use API built on modern C++17 (std::variant, std::string_view).
- Offer practical extensions (paths, references, binary blobs) for real‑world data models.
- Keep the codebase portable, header‑only, and easy to integrate.

## Design principles

- Header‑only, no external dependencies.
- Modern C++17 types and patterns (variant/visit, string_view, RAII).
- Zero‑copy where reasonable; lazy numeric conversion on demand.
- Predictable hot paths; branchless fallbacks where appropriate.
- SIMD (optional, via CPPON_ENABLE_SIMD) used selectively where it pays (quote/digit scans).
- Clear error handling with precise exceptions.

## What C++ON is not

- A JSON schema validator.
- A full database or query engine.
- A streaming SAX parser (C++ON focuses on a practical, ergonomic DOM‑like API).

## Features at a glance

- Typed numbers with C++‑style suffixes (i8/u8/i16/u16/i32/u32/i64/u64, float with `f`).
- Access paths and in‑document references (path_t and pointer_t).
- Binary blobs with built‑in Base64 (realized on demand).
- JSON‑compatible output layout.
- Thread‑local state isolation and SIMD level overrides.

## Status & versioning

C++ON is actively developed and versioned with semantic intent (SemVer). Interfaces may evolve between minor versions while striving to keep migrations straightforward. See CHANGELOG.md for noteworthy changes.

## Compatibility

- Compilers: GCC 7+, Clang 6+, MSVC 2019+
- Platforms: Windows, Linux, macOS
- Standard: C++17

## License

C++ON is released under the MIT License. See the [LICENSE](../LICENSE) file.

## Learn more

- Introduction: [./INTRODUCTION.md](./INTRODUCTION.md)
- Configuration: [./CONFIGURATION.md](./CONFIGURATION.md)
- Basic usage: [./BASIC_USAGE.md](./BASIC_USAGE.md)
- Paths and references: [./PATHS.md](./PATHS.md)
- Internals (design notes): [./internals/INDEX.md](./internals/INDEX.md)
- API Reference: [./API_REFERENCE.md](./API_REFERENCE.md)
- Literals: [./LITERALS.md](./LITERALS.md)
- Exceptions: [./EXCEPTIONS.md](./EXCEPTIONS.md)
- FAQ: [./FAQ.md](./FAQ.md)
- Glossary: [./GLOSSARY.md](./GLOSSARY.md)

## Community

- Contributions: [../CONTRIBUTING.md](../CONTRIBUTING.md)
- Code of Conduct: [./CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md)
- Acknowledgements: [./ACKNOWLEDGEMENT.md](./ACKNOWLEDGEMENT.md)
- Contact: [./CONTACT.md](./CONTACT.md)