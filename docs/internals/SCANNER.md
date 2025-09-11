# Scanner internals (and SIMD Dispatch)

Low-level primitives for fast character classification and token boundary detection.

## Goals

- Word-at-a-time (SWAR) baseline: portable, predictable.
- Optional SIMD (SSE/AVX2/AVX512) dispatch chosen at runtime.
- Strict bounds: never read past provided window or sentinel.

Contract
- Inputs must be backed by a NUL-terminated buffer (readable sentinel at `data()+size()`).
- Quote search is strictly bounded to the requested window.
- Digit scanning uses a scalar epilogue that peeks exactly one extra byte to detect the `'\0'` sentinel.
 
## SWAR core

Constants:

- kOnes (0x0101010101010101)
- kHigh (0x8080808080808080)

Masks:

| Function         | Purpose                               |
|------------------|----------------------------------------|
| zero_byte_mask   | Detect any zero byte                  |
| eq_byte_mask     | Byte-wise equality                    |
| lt_byte_mask     | Byte < threshold                      |
| gt_byte_mask     | Byte > threshold                      |
| not_digit_mask   | Non-digit detection (optimized)       |
| not_space_mask   | Non-space classification (mode aware) |

## Parallel operations

| Function                     | Role                                |
|------------------------------|-------------------------------------|
| m64_parallel_digits          | Advance until first non-digit       |
| m64_parallel_find_quote      | Locate next '"'                     |
| m64_parallel_skip_spaces     | Skip contiguous whitespace          |
| *_pos variants               | Return index or npos                |

Alignment strategy:

1. Byte loop to 8-byte boundary.
2. 64-bit word loop.
3. Scalar epilogue (includes sentinel check).

## Trusted input mode

- Spaciness predicate: `(c - 1) < 0x20` (branchless).
- '\0' excluded (terminator).
- Accepts extended control region for speed.

## Whitespace and hot paths

- Skip spaces
  - Strict vs Trusted mode (`CPPON_TRUSTED_INPUT`): strict = SP/TAB/LF/CR; trusted = [0x01..0x20], never '\0'
  - Sentinel-based loop: assumes null-terminated buffers; no out-of-bounds reads
  - Minimal loop: updates `pos` only if whitespace was consumed (fast path without stores)
- Digits scanning
  - SIMD when beneficial (xmm/ymm/zmm) with bounded epilogues
  - Scalar fast path: branchless ASCII test `unsigned(*p - '0') <= 9`
  - Avoids `std::isdigit` (locale/branches)
- SIMD strategy
  - Runtime detection (SSE/AVX2/AVX512) with global/thread overrides
  - Lazy binding via function pointers; epoch-based rebinding on override change
  - No SIMD for `skip_spaces` (cost/benefit unfavorable)
  - Recommended: Release `/O2 /GL /LTCG /arch:AVX2` (or platform equivalent)
- Errors and safety
  - `unexpected_end_of_text_error`: thrown if end reached after whitespace-only tail
  - Requirement: input buffers must be null-terminated (guaranteed by cppon)
- Thread-safety
  - SIMD overrides are thread-local; parser state is isolated via `thread_local`
- Performance notes
  - “Spaced” JSON benefits from trusted mode (branchless whitespace)
  - Compact JSON keeps hot paths short; stable perf without downclock

## SIMD ENABLED build (opt‑in)

Define CPPON_ENABLE_SIMD to include all SIMD code to the build:

- Opt-in to SIMD paths (SSE/AVX2/AVX-512 if available at runtime).
- Without it (or on unsupported targets), scalar paths are used. Runtime overrides are still available but may be no-ops.

## NO SIMD build (default)

Do not define CPPON_ENABLE_SIMD to exclude all SIMD code from the build:

- SIMD headers are not included; only scalar paths are compiled.
- Overrides become no‑ops; effective_simd_level() returns None.

## SIMD dispatch

| Level    | Functions bound                       |
|----------|---------------------------------------|
| None     | SWAR m64_* implementations            |
| SSE      | xmm versions (quote/digit)            |
| AVX2     | ymm versions                          |
| AVX512   | zmm versions (if supported)           |

Binding:

- Per-thread function pointers.
- Overrides: thread > global > auto-detect.
- Unsupported requested level is capped (e.g. AVX512 on AVX2 CPU).

API (public):

```cpp
// No-ops in NO_SIMD builds; effective_simd_level() returns None there

void set_global_simd_override(ch5::scanner::SimdLevel) noexcept;
void clear_global_simd_override() noexcept;

void set_thread_simd_override(ch5::scanner::SimdLevel) noexcept;
void clear_thread_simd_override() noexcept;

ch5::scanner::SimdLevel effective_simd_level() noexcept;
```

No-SIMD build:
- Stubs keep API stable, always return SimdLevel::None.

## Safety
| Concern             | Mitigation                          |
|---------------------|-------------------------------------|
| Over-read           | Strict loop bounds + sentinel byte  |
| Misalignment        | Prologue realignment or memcpy path |
| Null termination    | Required by parser contract         |

## Performance notes
- SIMD speedup strongest on large homogeneous spans.
- Downclock risk (wide vectors) can make SSE competitive on small inputs.
- SWAR path is the correctness + sanitizer baseline.

## Extension pattern
1. Add SIMD specialized function(s).
2. Extend binding switch.
3. Add tests for override + fallback parity.

## Interaction
- PARSER: quote and digit boundaries.
- PRINTER: not used (string building is sequential).
- BENCH harness: per-level comparisons.