# Performance

This page describes the benchmark methodology and presents representative results. The harness used is the bundled `c++on` benchmark (Release).

## Environment

- Dataset: <name/size>, e.g. ~4.7 MB JSON
- Toolchain: Visual Studio 2022 (Release), C++17
- CPU/OS: <CPU model>, <RAM/DDR>, <Storage>, <Windows/Linux/macOS>
- Compile-time ISA: <SSE/AVX2/...> (printed by the harness)
- Trusted input: <ON/OFF>

Tip (MSVC Release):
- __C/C++ > Optimization > Optimization__ = /O2
- __C/C++ > Optimization > Whole Program Optimization__ = /GL
- __Linker > Optimization > Link Time Code Generation__ = /LTCG
- Optionnel: __C/C++ > Code Generation > Enable Enhanced Instruction Set__ = AVX2

## Methodology

- Runtime SIMD selection; we also force levels to SWAR/SSE/AVX2/(AVX‑512 if supported) for comparison.
- Eval modes:
  - parse (validation only)
  - quick (lazy numbers)
  - eval (full eval, blobs not decoded)
  - full (full eval + blob decode)
- Printing modes:
  - compact (default), pretty, JSON‑compatible (`{"layout":{"json":true}}`)
  - Flatten references (`{"layout":{"flatten":true}}`) when relevant
- Buffer strategy: default vs `{"buffer":"retain"}` across iterations
- Iterations: 30+, timing with steady_clock; report min / median / avg / p95.

Advanced outputs:
- Build with `CPPON_BENCH_ADVANCED=1` to enable median/p95 in the harness.
- `CPPON_BENCH_EXPERIMENTS=1` to add an extra “Default at end” run (warm caches/freq).

## Metrics

The harness reports (nanoseconds; MB/s derived):
- File → buffer
- Buffer → string
- String → object (parse throughput vs input size)
- Object → string (print throughput vs output size)

We present:
- min / median / avg / p95, in ms and MB/s
- Effective SIMD level (auto or forced)

## Representative results (to be filled)

Dataset: <name/size>, Trusted input: <ON/OFF>, Compile-time ISA: <...>

Parsing (string → object), MB/s
- SWAR:   min <...> | med <...> | avg <...> | p95 <...>
- SSE:    min <...> | med <...> | avg <...> | p95 <...>
- AVX2:   min <...> | med <...> | avg <...> | p95 <...>
- AVX-512 (if available):   min <...> | med <...> | avg <...> | p95 <...>

Printing (object → string, compact), MB/s
- SWAR:   min <...> | med <...> | avg <...> | p95 <...>
- SSE:    min <...> | med <...> | avg <...> | p95 <...>
- AVX2:   min <...> | med <...> | avg <...> | p95 <...>

Options impact
- JSON‑compat (`{"layout":{"json":true}}`): <delta vs compact>
- Flatten (`{"layout":{"flatten":true}}`): <delta when references present>
- Buffer retain (`{"buffer":"retain"}`): <allocation reduction, throughput delta>
- Pretty (`{"pretty":true}`): <delta vs compact>

Notes
- On some CPUs AVX2/AVX‑512 can down‑clock; scalar/SSE may be competitive on short, branch‑friendly paths.
- Use overrides for diagnostics only; default to auto‑detect in production.
## Runtime SIMD impact (Quick mode, Trusted input ON)

Setup
- Build: x64 Release, C++17, Windows
- Compile-time ISA: AVX2 available
- SIMD: enabled (runtime dispatch)
- Mode: quick (lazy numbers), Trusted input: ON
- Input: ~4.76 MB JSON (7 chunks)

## Representative results (quick mode, lazy numbers)

Parsing (string → object), MB/s (input size: 4,757,254 bytes)

| SIMD level | Peak (GB/s) | Avg (GB/s) | Notes |
|------------|------------:|-----------:|-------|
| SWAR       | 0.84        | 0.79–0.80  | SWAR baseline |
| SSE        | 0.89        | 0.84–0.86  | Scan phase gains |
| AVX2       | 0.95        | 0.86–0.87  | Matches SSE (logic bound) |

Printing (object → string, compact), MB/s (output size: 4,756,834 bytes)

| SIMD level | Peak (GB/s) | Avg (GB/s) | Notes |
|------------|------------:|-----------:|-------|
| SWAR       | 1.23        | 1.15–1.16  | Buffer reuse amortized |
| SSE        | 1.26–1.27   | 1.14–1.15  | Close to scalar |
| AVX2       | 1.28–1.29   | 1.17–1.19  | Mostly memory bound |

Detailed phase metrics (best AVX2 cycle)

| Phase            | Size (bytes) | Best ms | MB/s (peak) | Avg ms | MB/s (avg) |
|------------------|-------------:|--------:|------------:|-------:|-----------:|
| file → buffer    | 4,757,094    | 2.547   | 1,868       | 2.686  | 1,771 |
| buffer → string  | 4,757,094    | 0.992   | 4,797       | 1.035  | 4,596 |
| string → object  | 4,757,254    | 5.009   |   950       | 5.485  |   867 |
| object → string  | 4,756,834    | 3.700   | 1,286       | 4.034  | 1,179 |

*Peak = best sample after outlier trimming (10 lowest / 10 highest removed).*

(Outliers: 10 lowest / 10 highest removed per phase.)

Additional observations
- Memory I/O (buffer → string) reaches ~4.6–5.0 GB/s (near memory bandwidth).
- “Default/Warmup” can show lower figures due to frequency/thermal jitter; per-level runs are more stable.
- AVX‑512 requests were capped to AVX2 on the test CPU (Effective: AVX2).

How to reproduce
- Enable SIMD at compile time:
  - MSVC: /DCPPON_ENABLE_SIMD=1 (Release, /O2; optional /GL /LTCG, /arch:AVX2)
  - GCC/Clang: -DCPPON_ENABLE_SIMD=1 -O3 (optional -flto -march=native)
- Keep Trusted input ON for comparable whitespace performance (/DCPPON_TRUSTED_INPUT=1).
- At runtime, exercise overrides to lock a level:

```cpp
ch5::set_global_simd_override(ch5::SimdLevel::AVX2); // or SSE, SWAR
auto eff = ch5::effective_simd_level(); // check effective level
// run benchmark
ch5::clear_global_simd_override();
```

- Recommendations for stable numbers: 500–1000 iterations, steady_clock timing, CPU affinity, high process priority.

# Benchmarks

This page outlines how to run and interpret C++ON micro‑benchmarks.

## Harness checklist
- Separate I/O and compute: measure file→buffer, buffer→string, string→object, object→string independently.
- Warm up before timing; use steady_clock; run 500–1000 iterations per mode.
- Pin to a CPU core and set high process priority to reduce jitter.
- Build Release with optimizations; optionally enable LTO (MSVC /GL /LTCG, GCC/Clang -flto).

## SIMD controls
- Compile‑time opt‑in: define CPPON_ENABLE_SIMD=1.
- Runtime overrides for diagnostics:

```cpp
ch5::set_global_simd_override(ch5::SimdLevel::AVX2); // or SSE, SWAR
auto eff = ch5::effective_simd_level(); // check effective level
// run benchmark
ch5::clear_global_simd_override();
```

## Suggested scenarios
- Parsing: quick vs eval vs full (numbers/blobs conversion impact).
- Printing: compact vs pretty; JSON‑compatible layout.
- References: flatten on/off (resolve_paths).
- Trusted input: ON vs OFF (whitespace handling).

## Example flags
- MSVC: /O2 /std:c++17 /DCPPON_ENABLE_SIMD=1 [/GL /LTCG] [/arch:AVX2]
- GCC/Clang: -O3 -std=c++17 -DCPPON_ENABLE_SIMD=1 [-flto] [-march=native]

See [SIMD_SUPPORT.md](SIMD_SUPPORT.md) for a SIMD impact summary (SWAR vs SSE vs AVX2).