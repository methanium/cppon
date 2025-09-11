/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-visitors.h : Object traversal and manipulation utilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_VISITORS_H
#define CPPON_VISITORS_H

#include "c++on-types.h"
#include <algorithm>
#include <deque>
#include <stack>
#include <cctype>

namespace ch5 {
namespace visitors {

constexpr size_t max_array_delta = CPPON_MAX_ARRAY_DELTA;

struct key_t {
	std::string_view value;
	explicit key_t(std::string_view v) : value(v) {}
	operator std::string_view() const { return value; }
};

/**
 * @brief Central utilities for managing cppon objects and their interconnections.
 *
 * This suite of utility functions is foundational to the cppon framework. It manages a per-thread root
 * context via a stack, exposes a per-thread null sentinel, and supports dereferencing of path- and pointer-
 * based references consistently across traversals.
 *
 * This module provides:
 * - A thread_local static_storage that holds:
 *   - a std::vector<cppon*> root stack (used with stack semantics),
 *   - a singleton cppon Null used as a null sentinel.
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

// The stack is initialized with a bottom sentinel (nullptr). This avoids empty() checks.
// The sentinel is never dereferenced because get_root() asserts top != nullptr.
struct static_storage {
	static_storage() noexcept : stack{ nullptr }, Null{nullptr} {}
	std::vector<cppon*> stack;
	cppon Null;
};
inline static_storage& get_static_storage() noexcept {
	static thread_local static_storage storage;
	return storage;
}
inline cppon& null() noexcept {
	return get_static_storage().Null;
}
inline std::vector<cppon*>& root_stack() noexcept {
	return get_static_storage().stack;
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

// Bounded helper to check if a string_view contains only digits
inline bool all_digits(string_view_t sv) noexcept {
	return std::all_of(sv.begin(), sv.end(), [](unsigned char c) { return (static_cast<unsigned char>(c - 0x30) <= 9u); });
}
// Bounded helper to convert a numeric string_view into size_t
inline size_t parse_index(string_view_t sv) noexcept {
	size_t idx = 0;
	// no sign or spaces expected; digits already validated via all_digits()
	for (char c : sv) {
		idx = idx * 10 + static_cast<size_t>(c - '0');
	}
	return idx;
}

/**
 * @brief Internal dereference helpers for pointer_t and path_t.
 *
 * deref_if_ptr:
 * - pointer_t: returns *arg (or the thread-local null sentinel if the pointer is null).
 * - path_t: resolves against the current root (get_root()); the leading '/' is removed unconditionally.
 * - other alternatives: returns the original object reference.
 *
 * deref_if_not_null (non-const only):
 * - If the slot holds a null pointer_t, returns the slot itself (enables autovivification at the slot).
 * - Otherwise delegates to deref_if_ptr.
 *
 * Notes
 * - These helpers are internal (namespace ch5::visitors). Public traversal uses visitor(...) and cppon::operator[].
 * - On path_t resolution, visitor(...) may throw: member_not_found_error, type_mismatch_error, null_value_error, etc.
 */
inline auto deref_if_ptr(const cppon& obj) -> const cppon& {
	return std::visit([&](auto&& arg) -> const cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, pointer_t>)
			return arg ? *arg : null();
		if constexpr (std::is_same_v<type, path_t>) {
			string_view_t tmp = arg.value;
			tmp.remove_prefix(1);
			return visitor(static_cast<const cppon&>(get_root()), tmp);
		}
		return obj;
	}, static_cast<const value_t&>(obj));
}
inline auto deref_if_ptr(cppon& obj) -> cppon& {
	return std::visit([&](auto&& arg) -> cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, pointer_t>)
			return arg ? *arg : null();
		if constexpr (std::is_same_v<type, path_t>) {
			string_view_t tmp = arg.value;
			tmp.remove_prefix(1);
			return visitor(get_root(), tmp);
		}
		return obj;
	}, static_cast<value_t&>(obj));
}

/**
 * @brief Selects the write reference for a non-leaf path segment.
 *
 * Behavior:
 * - If the slot currently holds a null pointer_t, returns the slot itself (autovivification at the slot).
 * - Otherwise returns the dereferenced target via deref_if_ptr (pointer_t/path_t/value).
 *
 * Exceptions:
 * - May propagate exceptions from deref_if_ptr -> visitor(get_root(), tmp) when resolving path_t
 *   (e.g., member_not_found_error, type_mismatch_error, null_value_error, ...).
 * - A null pointer_t path does not throw here.
 */
inline cppon& deref_if_not_null(cppon& slot) {
	if (auto p = std::get_if<pointer_t>(&slot); p && !*p)
		return slot;
	return deref_if_ptr(slot);
}

/**
 * @brief Visitor functions for accessing and potentially modifying members in an object using a key.
 *
 * These specializations of the visitor function serve a dual purpose within the cppon framework. They allow for direct access
 * to members within an object by their name key. The non-const version additionally supports dynamically resizing the object
 * to accommodate new members, facilitating the mutation of the object structure.
 *
 * The const version of the function provides read-only access to the members within a constant object, ensuring that the operation does not
 * modify the object. It checks if the requested key is present inside the object and returns a reference to the member's value at that
 * location. If the key is not found, a null object is returned, indicating that the value does not exist at the specified key.
 *
 * Exceptions:
 * - anything that std::vector::emplace_back might throw (e.g., std::bad_alloc).
 */
inline auto visitor(const object_t& object, key_t key) noexcept -> const cppon& {
	for (auto& [name, value] : object)
		if (name == key) return value;
	return null();
}
inline auto visitor(object_t& object, key_t key) -> cppon& {
	for (auto& [name, value] : object)
		if (name == key) return value;
	object.emplace_back(key, null());
	return std::get<cppon>(object.back());
}

/**
 * @brief Visitor functions for accessing and potentially modifying elements in an array using a numeric index.
 *
 * These specializations of the visitor function serve a dual purpose within the cppon framework. They allow for direct access
 * to elements within an array by their numerical index. The non-const version additionally supports dynamically resizing the array
 * to accommodate indices beyond the current bounds, facilitating the mutation of the array structure. This dynamic resizing is
 * constrained by a permissible range, defined by the current size plus a max_array_delta threshold, to prevent excessive expansion.
 *
 * The const version of the function provides read-only access to elements within a constant array, ensuring that the operation does not
 * modify the array. It checks if the requested index is within the bounds of the array and returns a reference to the element at that
 * index. If the index is out of bounds, a null object is returned, indicating that the element does not exist at the specified index.
 *
 * Note that the non-const version is the source of `excessive_array_resize_error` if the requested index exceeds the permissible range
 * (current size + max_array_delta).
 *
 * Exceptions:
 * - excessive_array_resize_error: The requested index exceeds the permissible range (current size + max_array_delta) in the non-const version.
 * - anything that std::vector::resize might throw (e.g., std::bad_alloc).
 */
inline const cppon& visitor(const array_t& array, size_t index) noexcept {
	if (index >= array.size()) {
		return null();
	}
	return array[index];
}
inline cppon& visitor(array_t& array, size_t index) {
	if (index >= array.size() ) {
		if (index > array.size()  + max_array_delta)
			throw excessive_array_resize_error();
		array.resize(index + 1, null());
	}
	return array[index];
}

/**
 * @brief Visitor functions for accessing and potentially modifying elements in an array using a string index.
 *
 * The first path segment must be a numeric index. If not, bad_array_index_error is thrown.
 * If more segments follow, traversal recurses into the child (arrays/objects only).
 *
 * Exceptions:
 * - bad_array_index_error: the immediate segment is not numeric.
 * - null_value_error (const variant): next segment requested but current value is null.
 * - type_mismatch_error: non-terminal segment is not array/object.
 */
inline auto visitor(const array_t& array, string_view_t index) -> const cppon& {
	auto next{ index.find('/') };
	auto digits{ index.substr(0, next) }; // digits are numbers
	if (!all_digits(digits))
		throw bad_array_index_error(digits);
	auto& element = visitor(array, parse_index(digits));
	if (next == index.npos) return element; // if there is no next segment, it's a value
	auto& value = deref_if_ptr(element); // if there is a next segment, it's a path
	if (value.is_null()) throw null_value_error(); // if the value is null, throw an error
    return visitor(value, index.substr(next + 1));
}
inline auto visitor(array_t& array, string_view_t index) -> cppon& {
	auto next{ index.find('/') };
	auto digits{ index.substr(0, next) }; // digits are numbers
	if (!all_digits(digits))
		throw bad_array_index_error(digits);
	auto& element = visitor(array, parse_index(digits));
	if (next == index.npos) return element; // if there is no next segment, it's a value
	auto& value = deref_if_not_null(element); // if there is a next segment, it's a path
	auto newIndex{ index.substr(next + 1) };
	auto nextKey{ newIndex.substr(0, newIndex.find('/')) };
	CPPON_ASSERT(!nextKey.empty() && "Next key shall never be empty here");
	if (all_digits(nextKey)) {
		// next key is a number
		if (value.try_object()) throw type_mismatch_error{};
		if (value.try_array() == nullptr) value = cppon{ array_t{} };
	} else {
		// next key is a string
		if (value.try_array()) throw type_mismatch_error{};
		if (value.try_object() == nullptr) value = cppon{ object_t{} };
	}
    return visitor(value, newIndex);
}

/**
 * @brief Visitor functions for accessing and potentially modifying members in an object using a string key.
 *
 * The first path segment can be any name key.
 * If more segments follow, traversal recurses into the child (arrays/objects only).
 *
 * Exceptions:
 * - member_not_found_error (const variant): next segment requested but current value is null.
 * - type_mismatch_error: non-terminal segment is not array/object.
 */
inline const cppon& visitor(const object_t& object, string_view_t index) {
	auto next{ index.find('/') };
	auto key = index.substr(0, next); // key is a name
	auto& element = visitor(object, key_t{ key });
	if (next == index.npos) return element; // if there is no next segment, it's a value
	auto& value = deref_if_ptr(element); // if there is a next segment, it's a path
	if (value.is_null())
		throw member_not_found_error{}; // if the value not defined, throw an error
	return visitor(value, index.substr(next + 1));
}
inline cppon& visitor(object_t& object, string_view_t index) {
	auto next{ index.find('/') };
	auto key = index.substr(0, next); // key is a name
	auto& element = visitor(object,  key_t{ key });
	if (next == index.npos) return element; // no next segment, return value reference
	auto& value = deref_if_not_null(element); // if there is a next segment, it's a path
	auto newIndex{ index.substr(next + 1) };
	auto nextKey{ newIndex.substr(0, newIndex.find('/')) };
	CPPON_ASSERT(!nextKey.empty() && "Next key shall never be empty here");
	if (all_digits(nextKey)) {
		// next key is a number
		if (value.try_object()) throw type_mismatch_error{};
		if (value.try_array() == nullptr) value = cppon{ array_t{} };
	} else {
		// next key is a string
		if (value.try_array()) throw type_mismatch_error{};
		if (value.try_object() == nullptr) value = cppon{ object_t{} };
	}
	return visitor(value, newIndex);
}

/**
 * @brief Visitor functions for accessing and potentially modifying elements within a cppon object by a numerical index.
 *
 * These functions facilitate direct access to elements within a cppon object, which is expected to be an array,
 * using a numerical index. They are designed to handle both const and non-const contexts, allowing for read-only or
 * modifiable access to the elements, respectively.
 *
 * In the non-const version, if the specified index exceeds the current size of the array, the array is automatically resized
 * to accommodate the new element at the given index. This resizing is subject to a maximum increment limit defined by max_array_delta to prevent
 * excessive and potentially harmful allocation. If the required increment to accommodate the new index exceeds this limit, a bad_array_index_error exception is thrown,
 * signaling an attempt to access or create an element at an index that would require an excessively large increase in the array size.
 *
 * In both versions, if the cppon object is not an array (indicating a misuse of the type), a type_mismatch_error exception is
 * thrown. This ensures that the functions are used correctly according to the structure of the cppon object and maintains type safety.
 *
 * The const version of these functions provides read-only access to the elements, ensuring that the operation does not modify the
 * object being accessed. It allows for dynamic dereferencing of objects within a const context, supporting scenarios where the
 * cppon object structure is accessed in a read-only manner.
 *
 * Key Points:
 * - Direct access to elements by numerical index.
 * - Automatic resizing of the array in the non-const version to accommodate new elements, with a safeguard increment limit (max_array_delta).
 * - Throws excessive_array_resize_error if the required increment to reach the specified index exceeds the permissible range.
 * - Throws type_mismatch_error if the cppon object is not an array.
 * - Supports both const and non-const contexts, enabling flexible data manipulation and access patterns.
 *
 * @param object The cppon object to visit. This object must be an array for index-based access to be valid.
 * @param index The numerical index to access the element. In the non-const version, if the index is greater than the size of the array, the array is resized according to the increment limit.
 * @return (const) cppon& A reference to the visited element in the array. The non-const version allows for modification of the element.
 * @throws excessive_array_resize_error If the required increment to accommodate the specified index exceeds the maximum limit allowed by max_array_delta.
 * @throws type_mismatch_error If the cppon object is not an array, indicating a type error.
 */
inline const cppon& visitor(const cppon& object, size_t index) {
    return std::visit([&](auto&& arg) -> const cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, array_t>)
			return visitor(arg, index);
		throw type_mismatch_error{};
    }, static_cast<const value_t&>(object));
}
inline cppon& visitor(cppon& object, size_t index) {
    return std::visit([&](auto&& arg) -> cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, array_t>)
			return visitor(arg, index);
		throw type_mismatch_error{};
    }, static_cast<value_t&>(object));
}

/**
 * @brief Dynamically accesses elements within a cppon object using a string index, with support for both const and non-const contexts.
 *
 * This function allows dynamic access to elements within a cppon object, which can be either an array or an object,
 * using a string index. The string index can represent either a direct key in an object or a numeric index in an array.
 *
 * Additionally, it supports hierarchical access to nested elements by interpreting the string index as a path, where segments
 * of the path are separated by slashes ('/'). This allows deep access to nested structures, with the non-const version
 * allowing modification of elements.
 *
 * The function first determines the type of the cppon object (array or object) based on its current state. It then attempts
 * to access the element specified by the index or path. If the element is within a nested structure, the function recursively
 * navigates through the structure to reach the desired element. In the non-const version, any missing elements along the path
 * are created as either objects or arrays, depending on the segment type.
 *
 * Exceptions:
 * - `type_mismatch_error` is thrown if the object is not an array or an object when expected.
 * - In the const version:
 *   - If the key is a path, `member_not_found_error` or `type_mismatch_error` may be thrown depending on the path.
 *   - For array segments, `bad_array_index_error` is thrown if the segment is not numeric.
 *   - When reaching the leaf of the path in an object_t and the member does not exist, it returns the null object, avoiding exceptions for non-existent members in read-only scenarios.
 *   - If the index is out of bounds for the array, it returns the null object.
 * - In the non-const version:
 *   - If the path exists and the next segment is numeric for object_t, it throws `type_mismatch_error`.
 *   - For array segments, `bad_array_index_error` is thrown if the segment is not numeric.
 *   - If the index is out of bounds for the array and requires excessive resizing, it throws `excessive_array_resize_error`.
 *
 * This ensures that the caller is informed of incorrect access attempts or structural issues within the cppon object.
 *
 * The const version of this function provides the same functionality but ensures that the operation does not modify the object
 * being dereferenced. It allows dynamic dereferencing of objects in a const context, supporting scenarios where the cppon object
 * structure is accessed in a read-only manner.
 *
 * @param object The cppon object to visit. This object can be an array or an object, and may contain nested structures.
 *
 * @param index A string representing the index or path to the element to access. This can specify direct access or hierarchical access to nested elements.
 *              If index is empty, the function returns the object itself.
 *
 * @return (const) cppon& A reference to the cppon element at the specified index or path, with the non-const version allowing modification of the element.
 *
 * @throws type_mismatch_error If the cppon object does not match the expected structure for the specified index or path, indicating a type mismatch or invalid access attempt.
 * @throws member_not_found_error In the specified scenarios, indicating that a specified member in the path does not exist.
 * @throws invalid_path_segment_error In the specified scenarios, indicating that a path segment is invalid or out of bounds.
 * @throws excessive_array_resize_error In the specified scenarios, indicating that excessive array resizing is required.
 */
inline const cppon& visitor(const cppon& object, string_view_t index) {
	if (index.empty()) return object;
    return std::visit([&](auto&& arg) -> const cppon& {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, object_t>)
            return visitor(arg, index);
        if constexpr (std::is_same_v<type, array_t>)
            return visitor(arg, index);
        throw type_mismatch_error{};
    }, static_cast<const value_t&>(object));
}
inline cppon& visitor(cppon& object, string_view_t index) {
	if (index.empty()) return object;
    return std::visit([&](auto&& arg) -> cppon& {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, object_t>)
            return visitor(arg, index);
        if constexpr (std::is_same_v<type, array_t>)
            return visitor(arg, index);
        throw type_mismatch_error{};
    }, static_cast<value_t&>(object));
}

} // namespace visitors
} // namespace ch5

#endif // CPPON_VISITORS_H
