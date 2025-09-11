# Benchmarks: Configuration Heuristics

This document explains the default sizing / growth heuristics used by C++ON and how to tune them if your workload diverges.

## 1. Macro recap (current defaults)
 
| Macro                            | Role                               | Default |
|----------------------------------|------------------------------------|---------|
| CPPON_OBJECT_MIN_RESERVE         | Initial object member reserve      | 8       |
| CPPON_ARRAY_MIN_RESERVE          | Initial array element reserve      | 8       |
| CPPON_MAX_ARRAY_DELTA            | Sparse growth guard (indexed write)| 256     |
| CPPON_PRINTER_RESERVE_PER_ELEMENT| Printer per-element growth hint    | 16      |

## 2. Typical JSON profile (observed)

- 70–80% of objects have ≤ 8 members.
- Long tail: a few “hub” objects (30–200 members).
- Arrays: many small (3–12), some medium (50–300), rare large (1k–100k).
- Large index jumps (> +32) are usually accidental bugs (misused IDs or sparse misuse).

## 3. Recommended tuned defaults

```cpp
#define CPPON_OBJECT_MIN_RESERVE            8
#define CPPON_ARRAY_MIN_RESERVE             8
#define CPPON_MAX_ARRAY_DELTA               256
#define CPPON_PRINTER_RESERVE_PER_ELEMENT   16
```

Rationale:

- 8 covers the hottest object/array size band without over‑reserving tiny cases (1–4 elements).
- 256 as a guard still catches “wild” sparse array writes early while allowing moderate indexed growth bursts.
- 16 for printer reserve reduces the first 1–2 reallocations in moderately sized objects/arrays during serialization.

## 4. Why not higher?

- Raising OBJECT/ARRAY_MIN_RESERVE to 16 doubles the “empty” footprint of the most common small containers and only helps for objects > 8 members (less frequent).
- CPPON_MAX_ARRAY_DELTA above 512 starts hiding accidental large index jumps (sparse misuse) instead of surfacing them early.
- PRINTER_RESERVE_PER_ELEMENT above ~24 mostly benefits very large dumps where the amortized cost after the first growths is already negligible.

## 5. Light memory footprint variant (constrained targets)

Use when peak RSS matters more than a few extra reallocations:

```cpp
#define CPPON_OBJECT_MIN_RESERVE            4
#define CPPON_ARRAY_MIN_RESERVE             4
#define CPPON_MAX_ARRAY_DELTA               128
#define CPPON_PRINTER_RESERVE_PER_ELEMENT   12
```

Trade-offs:
- Slightly more reallocations on objects that naturally grow past 4.
- Stricter sparse guard (128) may require adjustment if you rely on batched index jumps.

## 6. How to measure impact

1. Parse a synthetic dataset of N objects with an average of 6–10 members. Count allocations (custom allocator or instrument operator new).
2. Serialize a representative DOM (pretty + compact) and log string capacity growth patterns.
3. Adjust OBJECT/ARRAY_MIN_RESERVE upward only if:
   - Allocation hotspots show >1 early growth for the median container, AND
   - The typical size comfortably exceeds the current reserve.
4. Validate CPPON_MAX_ARRAY_DELTA by intentionally writing far indices (e.g. index 300 on empty) to confirm error surfacing.

## 7. When to tune

| Symptom                                    | Consider tuning                        |
|--------------------------------------------|----------------------------------------|
| Many early reallocations in hot objects    | Increase *_MIN_RESERVE (step 8 → 12)   |
| Legitimate batched indexed writes failing  | Increase CPPON_MAX_ARRAY_DELTA         |
| Printer spends time in repeated growth     | Raise CPPON_PRINTER_RESERVE_PER_ELEMENT (e.g. 16 → 20) |
| Memory pressure / embedded target          | Use light variant                      |

## 8. Keep it simple

Avoid overfitting: start with defaults; only change after profiling with realistic data. Large jumps (e.g. doubling all reserves) rarely produce balanced wins and often inflate baseline memory.

---

For compile-time overrides, define the macros before including `<cppon/c++on.h>` or set them via your build system’s preprocessor definitions.