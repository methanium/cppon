# References internals (path_t ↔ pointer_t)

Mechanism to convert textual paths into direct pointers and back, enabling faster repeated dereferencing.

## Goals

- Speed up deep repeated path lookups.
- Preserve reversibility (restore textual form for JSON output or persistence).
- Detect cycles for safe flattening decisions.

## Key structures

| Type                 | Purpose                                      |
|----------------------|----------------------------------------------|
| reference_vector_t   | Vector of (original path, slot pointer)      |
| pointer_map_t        | Temporary map path → resolved target pointer |

## Resolution flow (resolve_paths)

1. find_references: DFS collect all path_t slots.
2. find_objects: For each path:
   - Walk from root segment by segment.
   - If missing segment → null out source slot (broken path).
   - If fully resolved → register mapping in pointer_map_t.
3. Replace path_t slot with pointer_t (target).

Returns reference_vector_t for potential restoration.

## Restoration (restore_paths)

Iterates reference_vector_t and reassigns each pointer slot back to path_t (original string_view).

## Cycle detection

| Function             | Description                             |
|----------------------|------------------------------------------|
| is_object_inside     | Recursive containment / pointer scan     |
| is_pointer_cyclic    | Tests if pointer target ultimately contains itself |

Used by printer when flattening pointer_t targets.

## Reverse lookup

| Function          | Purpose                                      |
|-------------------|----------------------------------------------|
| find_object_path  | Build textual path from root to target ptr   |
| get_object_path   | Find associated path for a stored pointer    |

## Failure handling

| Condition              | Behavior                    |
|------------------------|-----------------------------|
| Broken path segment    | Slot set to nullptr pointer |
| Unmapped pointer query | Assertion + runtime error   |

## Safety constraints

- pointer_t stability relies on container reallocation discipline (avoid invalidating underlying vector storage before restore).
- reference_vector_t must outlive period where pointer_t form is relied upon (for restore correctness).

## Flatten interaction

- If flatten disabled: pointer_t printed as path token.
- If enabled + acyclic: subtree inlined.
- If cycle: fallback to path token (prevents infinite recursion).

## Recommended usage

Typical scenarios:
1. You want faster repeated printing of pointer-heavy structures (avoid repeated DFS path reconstruction).
2. You need stable original textual paths (round‑trip path_t ↔ pointer_t ↔ path_t).

Optional printing optimization:
- The printer does NOT require `Refs` to flatten pointers. It only uses them when:
  - Flatten=false (emit path tokens) OR
  - Flatten=true but a cycle forces a path fallback.
- Without `Refs`, each path token emission performs a DFS via `find_object_path`.

Call `resolve_paths(root)` when:
- The number of pointer_t nodes is large or frequently re‑serialized.
- You want deterministic reuse of original path spellings (e.g. after modifications).

Call `restore_paths(refs)` before:
- External JSON where you prefer explicit textual paths.
- Transformations that may invalidate pointer targets (structure rewrites).

## Limitations

- No incremental update (full pass).
- Duplicate paths collapse to last processed slot only.
- No relative path optimization (absolute only).

## Extension ideas

- Relative pointer caching for shared subroots.
- Hash-based path indexing for very large object graphs.