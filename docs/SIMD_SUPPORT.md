# SIMD Support

C++ON selects the best implementation at runtime based on CPU features:
- SSE (128‑bit), AVX2 (256‑bit), AVX‑512 (512‑bit)
- Safe scalar fallback if SIMD is unavailable

## Detection and overrides

- Auto‑detection picks the max supported level for the current CPU.
- Optional overrides:
  - Global (process‑wide): set_global_simd_override(...), clear_global_simd_override()
  - Thread‑local (has precedence): set_thread_simd_override(...), clear_thread_simd_override()
  - Query: effective_simd_level()

Example:

```cpp
// Global override (process-wide)
ch5::set_global_simd_override(ch5::SimdLevel::AVX2);
auto eff = ch5::effective_simd_level();
ch5::clear_global_simd_override();

// Thread-local override has precedence over global
ch5::set_thread_simd_override(ch5::SimdLevel::SSE);
eff = ch5::effective_simd_level();
ch5::clear_thread_simd_override();
```

Notes
- Overrides are intended for diagnostics/benchmarks; default to auto‑detect in production.
- Thread override > Global override > Auto‑detect.

## Build modes

- Opt-in: define CPPON_ENABLE_SIMD (SSE / AVX2 / AVX‑512 dispatched at runtime).
- Scalar-only: omit CPPON_ENABLE_SIMD. Only SWAR (64-bit word-at-a-time) code is compiled; overrides are no-ops (`effective_simd_level() == None`).

## Safety and tail handling

- Implementations must never read past `std::string_view` bounds.
- Tail strategies:
  - AVX‑512: masked loads.
  - SSE/AVX2: bounded vector window + scalar epilogue for remaining bytes.
  - Digits: scalar epilogue peeks exactly 1 extra byte to catch the `'\0'` sentinel.
- Quote finders are strictly bounded to the provided window.

## Performance notes

- AVX2/AVX‑512 can down‑clock on some CPUs; on small inputs or scalar‑friendly hot paths, SSE or scalar may perform similarly or better.
- For stable measurements:
  - Warm up and pin priority (optional).
  - Consider thread affinity and steady_clock timing in benchmarks.
  - Compare levels on your dataset; keep overrides off in production.

## Build hints

- Keep runtime dispatch as default; optionally compile with ISA hints when you control the deployment target.
  - MSVC: __C/C++ > Code Generation > Enable Enhanced Instruction Set__ = AVX2 (optional)
  - GCC/Clang: `-O3 -std=c++17 -march=x86-64-v3` (or `-mavx2`), or `-march=native` on controlled hosts

## Testing

- Always test scalar paths (they are the ultimate fallback).
  - Sanitizers (ASan/UBSan) recommended on scalar builds.
- Include a NO_SIMD translation unit test to verify API stubs and scalar behavior.
- Exercise overrides in tests to ensure thread/global precedence and capping to CPU capabilities.