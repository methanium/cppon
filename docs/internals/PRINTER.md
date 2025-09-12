# Printer internals

Explains formatting, option resolution, buffer reuse, and safety rules.

## Goals

- Configurable layouts (compact, pretty, alternative layout tweaks).
- Optional flattening of internal references.
- JSON compatibility mode (53-bit safe integer enforcement).
- Reduced reallocations via tiered preallocation.

## State components

Thread-local `printer_state`:

| Field         | Role                                |
|---------------|--------------------------------------|
| Out           | Output buffer (retained optional)    |
| Compacted     | Labels to inline (no indentation)    |
| Level/Tabs    | Indentation management               |
| Margin        | Initial left offset                  |
| Pretty        | Enables newline / indentation        |
| AltLayout     | Alternate exit ordering              |
| Compatible    | JSON-safe emission                   |
| Flatten       | Inline pointer targets               |
| RetainBuffer  | Preserve previous capacity           |
| Reserve       | Enable preallocation heuristics      |

## Options resolution

Accepted root keys:

- buffer: "reset" / "retain" / "reserve"/"noreserve" or object form.
- layout: object or string ("json","flatten","cppon").
- compact: bool or array of labels.
- pretty: bool.
- margin: int.
- tabulation: int.

Invalid shapes → bad_option_error.

## Compaction behaviors

| Mode                        | Effect                                 |
|-----------------------------|----------------------------------------|
| compact = true              | All objects printed minimal            |
| compact = ["a","b"]         | Only matching top-level labels inlined |
| pretty + compaction subset  | Selective inline, others multiline     |

## JSON compatibility

Enforced when Compatible = true:

| Type     | Constraint                                          |
|----------|------------------------------------------------------|
| int64_t  | Must fit ±9007199254740991                          |
| uint64_t | Must be ≤ 9007199254740991                          |

Violations → json_compatibility_error.

Float formatting:

- `%.16lg` (double), `%.7g` (float).
- Force .0 if integral-like w/o exponent.
- Append 'f' suffix only in non-compatible float_t.

## Flatten logic

| Case                                        | Printed as                                  | Needs Refs? |
|---------------------------------------------|---------------------------------------------|-------------|
| Flatten = false                             | Path token (`"$cppon-path:/.../..."`)       | Optional (Refs speed up path lookup) |
| Flatten = true & non‑cyclic pointer_t       | Inlined dereferenced subtree                | No          |
| Flatten = true & cyclic pointer_t           | Path token (cycle-safe fallback)            | Optional    |

Notes:
- Passing `Refs` (result of `resolve_paths`) is optional.  
  - Without it, path tokens are reconstructed on demand via `find_object_path(visitors::get_root(), ptr)`.
  - With it, `get_object_path(*Refs, ptr)` avoids a DFS per pointer and preserves original textual paths if they were present.
- Flatten never needs `Refs` to inline a subtree; it only dereferences the pointer directly unless a cycle is detected.
- Cycles: `is_pointer_cyclic(ptr)` triggers the fallback to a path token even under flatten=true.

Performance guidance:
- Small/medium trees or few pointers: skip `resolve_paths`.
- Many repeated pointer_t emissions or deep trees: call `auto refs = resolve_paths(root);` then `to_string(root, &refs, opts)` for fewer path recomputations.

## Preallocation heuristic

- Initial guess: printer_reserve_per_element * element_count.
- Dynamic adjust: if actual usage exceeds predicted mean, recompute per-element average and reserve remainder.

## String emission

Current implementation does NOT escape control characters (future extension). Inputs assumed sane or pre-sanitized.

## Adding new type

1. Implement `print(printer&, const T&)`.
2. Respect Compatible mode (avoid suffixes, enforce numeric constraints).
3. Integrate with flatten rules if referencing graph nodes.

## Failure modes

| Issue                          | Consequence              |
|--------------------------------|--------------------------|
| Missing option type matches    | bad_option_error         |
| Large integer overflow (compat)| json_compatibility_error |
| Cyclic pointer flatten attempt | Falls back to path token |
| Missing Refs (when provided earlier) | N/A (Refs are strictly optional) |

## Interactions

- REFERENCES: path reconstruction for printing pointer_t.
- PARSER: number_t vs converted numeric influences formatting.