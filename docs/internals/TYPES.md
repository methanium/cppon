# Internals: Core Types (cppon, root management)

This file covers the `cppon` wrapper around `value_t`, root stack integration, and helper accessors.

## Goals
- Thin façade around std::variant with ergonomic access.
- Path-based autovivification (non-const) with strict invariants.
- Safe root context management for absolute navigation.
- Separation of throwing and non-throwing access patterns.

## cppon highlights

| Feature             | Description |
|---------------------|-------------|
| Variant base        | Publicly inherits `value_t` |
| `operator[]("/a/b")`| Pushes root (if absolute), then relative traversal |
| Autovivification    | Non-const path creates intermediate containers |
| No-op root reuse    | push_root promotes if already deeper |
| Destructor          | Calls pop_root to avoid stale stack entries |
| Pointer safety      | Assignment checks `valueless_by_exception()` |

## Root stack (see VISITORS.md)

- thread_local vector<cppon*> with a bottom sentinel (nullptr).
- Uniqueness: entry hoisted if already present.
- Balanced automatically by object lifetime + optional root_guard.

## Accessors

| Method         | Behavior                               |
|----------------|----------------------------------------|
| array()        | Returns array_t& or throws type_mismatch_error |
| object()       | Same for object_t                      |
| try_array()    | Returns pointer or nullptr             |
| try_object()   | Same                                    |
| is_null()      | Holds nullptr_t alternative            |

## Path semantics

| Form             | Effect                                    |
|------------------|--------------------------------------------|
| Absolute "/a/b"  | Sets root to *this then traverses "a/b"    |
| Relative "k/x"   | Traverses from current node                |
| Numeric [i]      | Array index; may grow if non-const         |
| Mixed segments   | Pattern-based container synthesis          |

## Autovivification rules (write)

- Next segment numeric → create array_t if absent / non-conflicting.
- Next segment name   → create object_t if absent / non-conflicting.
- Conflict (object vs numeric or array vs string) → type_mismatch_error.
- Sparse array growth bounded by CPPON_MAX_ARRAY_DELTA.

## Null vs empty

- Sentinel null() used for missing reads (const leaf).
- User-facing writes replace null slot with container or value.

## Assignment templates

Two SFINAE traits (is_in_variant_lv / is_in_variant_rv) constrain overload resolution to valid alternative types.

## Pointer assignment guard

Throws unsafe_pointer_assignment_error if assigning a pointer_t to an object in valueless_by_exception state (defensive).

## Best practices

- Use `root_guard` when mixing multiple documents in a scope.
- Prefer `try_*` helpers to avoid exceptions in tentative probing.
- Resolve textual paths → pointer_t via resolve_paths for hot loops.

## Extension

When adding alternative types:
1. Extend value_t.
2. Provide print overload.
3. Possibly adjust visitor(...) if hierarchical.
4. Update scanner/parser if new literal syntax.