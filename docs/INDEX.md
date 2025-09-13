# C++ON Documentation

Welcome to the C++ON documentation. This index routes to all sections.

## Table of contents

- Overview
  - [About](./ABOUT.md)
  - [Introduction](./INTRODUCTION.md)
  - [FAQ](./FAQ.md)
  - [Glossary](./GLOSSARY.md)
  - [Contact](./CONTACT.md)
  - [Acknowledgements](./ACKNOWLEDGEMENT.md)

- Getting started
  - [Basic usage](./BASIC_USAGE.md)
  - [Paths and references](./PATHS.md)
  - [Document class](./DOCUMENT.md)
  - [Configuration](./CONFIGURATION.md)
  - [API reference](./API_REFERENCE.md)
  - [Literals](./LITERALS.md)
  - [Exceptions](./EXCEPTIONS.md)

- Performance & SIMD
  - [Performance](./PERFORMANCE.md)
  - [SIMD support](./SIMD_SUPPORT.md)

- Internals
  - [Design notes (internals index)](./internals/INDEX.md)

- Project
  - [Roadmap](./ROADMAP.md)
  - [Changelog](./CHANGELOG.md)
  - [License](../LICENSE)
  - [Contributing](../CONTRIBUTING.md)

## Examples, Tests, Benchmarks

- Examples
  - Build with single_include/cppon/c++on.h or modular headers in include/cppon/
  - Good starters:
    - examples/references_example.cpp — path_t ↔ pointer_t, cycles, flatten
    - examples/formatting_options.cpp — printing options, JSON compatibility, resolve_paths
    - examples/basic_usage.cpp, examples/binary_blobs.cpp

- Tests
  - Scope: UTF‑8/BOM, CPPON_MAX_ARRAY_DELTA, parser/visitor exceptions, references, serialization, blobs, accessors.
  - Infra: GoogleTest (ctest/CMake recommended).

- Benchmarks
  - Parse: Quick/Eval/Full; Print: compact/pretty, flatten on/off; effect of resolve_paths().
  - Tips: pin CPU, warmup, LTO/PGO. See [Performance](./PERFORMANCE.md).