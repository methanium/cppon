/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-literals.h : C++ON
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_ROOTS_H
#define CPPON_ROOTS_H

#include "c++on-types.h"
#include "c++on-thread.h"

namespace ch5 {

inline cppon& null() noexcept {
	CPPON_ASSERT(thread::null.is_null());
	return thread::null;
}

namespace roots {

/**
 * @brief Central utilities for managing cppon objects and their interconnections.
 *
 * This suite of utility functions is foundational to the cppon framework. It manages a per-thread root
 * context via a stack, exposes a per-thread null sentinel, and supports dereferencing of path- and pointer-
 * based references consistently across traversals.
 *
 * Sentinel and invariants:
 * - The root stack is constructed with a bottom sentinel (nullptr) to avoid repeated stack.empty() checks.
 *   This sentinel is never dereferenced. Access to the current root goes through get_root(), which asserts
 *   that the top entry is non-null.
 * - Uniqueness: each pushed root must be unique within the stack (no duplicates). In Debug, push_root() asserts this.
 * - LIFO discipline: pop_root() only pops if the given object is at the top; otherwise it is a no-op (harmless temporaries).
 *
 * - `null()`: Provides a per-thread singleton instance representing a null value, ensuring consistent
 *   representation of null or undefined states across the cppon object hierarchy.
 *
 * - Root stack and accessors:
 *   - `root_stack()`: Access to the thread-local stack that holds `cppon*` roots.
 *   - `get_root()`: Returns a reference to the current root. Asserts the stack is never empty and the
 *     top is never nullptr (thus the sentinel is never accessed).
 *   - `push_root(const cppon&)` and `pop_root(const cppon&)`: Push the given object if not already on top,
 *     and pop it only if it is the current top, respectively. Guarantees balanced stack usage in scoped code.
 *
 * - Absolute paths and guards:
 *   - Absolute-path access (string indices starting with '/') uses `push_root(*this)` and resolves the
 *     remainder of the path against `get_root()`.
 *   - `root_guard` is the RAII helper to scope a root change safely (push on construction, pop on destruction).
 *
 * @note `deref_if_ptr` resolves `pointer_t` directly and `path_t` against the current root. Assertions enforce
 *       that `get_root()` is only used when the top is non-null.
 *
 * Collectively, these utilities underscore the cppon framework's flexibility, robustness, and capability to represent
 * and manage complex data structures and relationships. They are instrumental in ensuring that cppon objects can be
 * dynamically linked, accessed, and manipulated within a coherent and stable framework.
 */

// Notes:
// - The stack is initialized with a bottom sentinel (nullptr). This avoids empty() checks.
// - The sentinel is never dereferenced because get_root() asserts top != nullptr.

inline std::vector<cppon*>& root_stack() noexcept {
	return thread::stack;
}
inline cppon& get_root() noexcept {
	CPPON_ASSERT(!root_stack().empty() && root_stack().back() != nullptr &&
		"Root stack shall never be empty and stack top shall never be nullptr");
	return *root_stack().back();
}
// Promote the entry to top if found; returns true if present (already top or promoted)
inline bool hoist_if_found(std::vector<cppon*>& s, const cppon* p) noexcept {
	if (s.back() == p) return true;
	auto found = std::find(s.rbegin(), s.rend(), p);
	if (found == s.rend()) return false;
	std::swap(*found, s.back());
	return true;
}
inline void pop_root(const cppon& root) noexcept {
	auto& stack = root_stack();
	if (hoist_if_found(stack, &root))
		stack.pop_back();
}
inline void push_root(const cppon& root) {
	auto& stack = root_stack();
	if (!hoist_if_found(stack, &root))
		stack.push_back(&const_cast<cppon&>(root));
}

} // namespace roots
} // namespace ch5


#endif // CPPON_ROOTS_H
