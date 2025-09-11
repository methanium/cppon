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

| Case                               | Printed as                         |
|------------------------------------|------------------------------------|
| Flatten=false                      | Path token for pointer_t           |
| Flatten=true, non-cyclic pointer   | Inline dereferenced subtree        |
| Flatten=true, cyclic pointer       | Path token (cycle safe)            |

Cycle detection via is_pointer_cyclic(pointer_t).

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

## Interactions

- REFERENCES: path reconstruction for printing pointer_t.
- PARSER: number_t vs converted numeric influences formatting.