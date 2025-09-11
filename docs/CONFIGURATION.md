# Configuration Guidelines for C++ON

This page lists the main build-time switches and how to enable them per platform, plus runtime SIMD overrides.

## Build-time macros

- CPPON_ENABLE_SIMD (opt‑in) 
  Enables inclusion of SIMD code paths (SSE / AVX2 / AVX‑512 runtime detection). If absent the build is scalar-only.
   - Effects when defined:
     - `CPPON_USE_SIMD` = 1 (internal); without it = 0.
     - Runtime dispatch picks the highest supported level unless overridden.
     - `effective_simd_level()` reflects detected/overridden SIMD level.
   - To force scalar build: do not define the macro (or explicitly undefine with `-UCPPON_ENABLE_SIMD`).


- CPPON_TRUSTED_INPUT (opt‑in)  
  Makes `is_space` branchless by treating ASCII controls in [0x01..0x20] as whitespace (`'\0'` is never whitespace). Faster on “spaced” JSON; more permissive than strict JSON.
  - Enable:
    - In code:
      ```cpp
      #define CPPON_TRUSTED_INPUT
      #include <cppon/c++on.h>
      ```
    - Visual Studio: __C/C++ > Preprocessor > Preprocessor Definitions__ = `CPPON_TRUSTED_INPUT;%(PreprocessorDefinitions)`

- CPPON_ENABLE_STD_GET_INJECTION
  Enable std::get/std::get_if “injection” into the ch5 namespace so you can call `get/get_if` unqualified. Opt‑in; only if you prefer ADL-style calls. Note: may increase risk of ambiguities if you define similar symbols.

- CPPON_VERSION_MAJOR / MINOR / PATCH / STRING / HEX
  Version info macros; informational only (not configuration).

- CPPON_OBJECT_MIN_RESERVE, CPPON_ARRAY_MIN_RESERVE  
  Initial reserve hints for objects/arrays during parsing.

- CPPON_MAX_ARRAY_DELTA  
  Guard against accidental “sparse” arrays during writes. If an indexed write jumps forward by more than this delta from the current size, the operation throws `excessive_array_resize_error`.  
  Default: `256`. Increase only if you intentionally skip large ranges, otherwise append sequentially.

- CPPON_PRINTER_RESERVE_PER_ELEMENT  
  Printer’s per‑element reserve hint (replaces the old “initial printer reserve per element” name).

### Path and blob prefixes

- CPPON_PATH_PREFIX (default "$cppon-path:")
  - Strings starting with this prefix create a path_t whose value always begins with '/'.
  - The _path literal must start with '/', otherwise invalid_path_error is thrown.

- CPPON_BLOB_PREFIX (default "$cppon-blob:")
  - Strings starting with this prefix create a blob_string_t whose value represents a base 64 encoded blob_t.

Notes
- Prefixes macros must start with '$' and contain only URL-safe characters (alphanumeric, '-', '_', '.', '~').
- Keep macro definitions consistent across translation units (define before including `<cppon/c++on.h>` or set globally in the build system).

## Scanner, SIMD, and input contract

- CPPON_ENABLE_SIMD
  - Opt-in to SIMD paths (SSE/AVX2/AVX-512 if available at runtime).
  - Without it (or on unsupported targets), scalar paths are used. Runtime overrides are still available but may be no-ops.
 
- CPPON_TRUSTED_INPUT
  - Speeds up whitespace skipping (branchless, treats 0x01..0x20 as space; `'\0'` is never a space).
  - Does not change scanner contracts.

- Input buffer contract (important)
  - Scanner functions assume a readable `'\0'` sentinel at `text.data() + text.size()`.
  - Digit scanning reads exactly one extra byte in the scalar epilogue to catch that sentinel.
  - If you pass non NUL-terminated storage (e.g., `vector<uint8_t>`), either append a readable sentinel or avoid low-level scanner APIs.

## Runtime SIMD overrides

SIMD is auto‑detected at runtime (SSE/AVX2/AVX‑512 when available). You can override it:

```cpp
// Global override (process-wide)
ch5::set_global_simd_override(ch5::SimdLevel::AVX2);
auto eff = ch5effective_simd_level();
ch5::clear_global_simd_override();

// Thread-local override has precedence over global
ch5::set_thread_simd_override(ch5::SimdLevel::SSE);
eff = ch5::effective_simd_level();
ch5::clear_thread_simd_override();
```

Guidelines
- Use overrides mainly for benchmarking or diagnostics.
- Thread override > Global override > Auto-detect.

## Visual Studio (MSVC) setup

Recommended Release options (Performance)
- __C/C++ > Language > C++ Language Standard__ = ISO C++17
- __C/C++ > Optimization > Optimization__ = `/O2`
- __C/C++ > Optimization > Whole Program Optimization__ = `/GL`
- __Linker > Optimization > Link Time Code Generation__ = `/LTCG`
- __C/C++ > Code Generation > Enable Enhanced Instruction Set__ = AVX2 (optional, if you target AVX2)

Set macros
- __C/C++ > Preprocessor > Preprocessor Definitions__:
  - Enable SIMD: `CPPON_ENABLE_SIMD;CPPON_TRUSTED_INPUT;%(PreprocessorDefinitions)`
  - Scalar only: (omit `CPPON_ENABLE_SIMD`)

## GCC/Clang (Linux/macOS)

Examples
- Enable AVX2 codegen: `-O3 -march=x86-64-v3` (or `-mavx2`) when targeting modern CPUs.
- Define macros:
  - Enable SIMD: `-DCPPON_ENABLE_SIMD`
  - Trusted input (optional): `-DCPPON_TRUSTED_INPUT`
  - Scalar only: (do not pass `-DCPPON_ENABLE_SIMD`)

Example command
```bash
# SIMD enabled + trusted input
g++ -O3 -std=c++17 -march=x86-64-v3 -DCPPON_ENABLE_SIMD -DCPPON_TRUSTED_INPUT your_app.cpp -o your_app

# Scalar only
g++ -O3 -std=c++17 your_app.cpp -o your_app
```

## Practical tips

- Prefer enabling `CPPON_TRUSTED_INPUT` only for controlled inputs (bench, ETL pipelines, internal APIs) and keep it off by default for untrusted sources.
- If you need predictable memory usage on large objects/arrays, tune:
  - `CPPON_OBJECT_MIN_RESERVE`, `CPPON_ARRAY_MIN_RESERVE`
  - `CPPON_MAX_ARRAY_DELTA`
  - `CPPON_PRINTER_RESERVE_PER_ELEMENT`
- For cross‑platform builds, avoid setting contradictory macros across files to prevent ODR surprises.