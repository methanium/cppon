/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-types.h : Core type definitions
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_TYPES_H
#define CPPON_TYPES_H

#include "c++on-alternatives.h"
#include <type_traits>

namespace ch5 {

/**
 * @brief Central utilities for managing cppon objects and their interconnections.
 *
 * This suite of utility functions is foundational to the cppon framework. It manages the current root
 * context using a thread-local stack, provides a per-thread null sentinel, and enables consistent path
 * resolution across nested traversals while keeping threads isolated.
 *
 * - Root stack and accessors:
 *   - `push_root(const cppon&)` and `pop_root(const cppon&)`: Maintain a thread-local stack of roots.
 *     push_root() pushes the given object if it is not already at the top; pop_root() pops it only if
 *     it is the current top. This guarantees balanced push/pop in scoped contexts.
 *   - `get_root()`: Returns a reference to the current root. Asserts that the stack is non-empty and
 *     the top is non-null. Use this when resolving absolute paths or path_t values.
 *   - `root_guard`: RAII helper that pushes a root on construction and pops it on destruction, ensuring
 *     exception-safe stack balancing.
 *
 * - Invariants:
 *   - The root stack is thread_local and never empty (the bottom entry is a per-thread null sentinel).
 *   - The top entry must never be nullptr (enforced by assertions).
 *   - Each root must be unique within the stack (no duplicates allowed).
 *
 * - Absolute paths and operator[]:
 *   - When an index starts with '/', operator[] calls push_root(*this) and resolves the remainder of the
 *     path against `get_root()`. Prefer `root_guard` when switching the root for a scope.
 *
 * - `visitor(cppon&, size_t)`, `visitor(cppon&, string_view_t)`, `visitor(const cppon&, size_t)`, and
 *   `visitor(const cppon&, string_view_t)`: Provide mechanisms to access cppon objects by numeric index
 *   or string path. Resolution of `path_t` segments is performed against the current root.
 *
 * - Threading:
 *   - The root stack is per-thread (`thread_local`). Root changes are not visible across threads and
 *     do not require external synchronization.
 *
 * @note Lifetime: `cppon::~cppon()` calls `pop_root(*this)` to keep the stack consistent in the presence
 *       of temporaries or scoped objects.
 *
 * @warning Call `pop_root` only when you own the current top (typically via `root_guard` or the owning
 *          object's lifetime). Do not attempt to manually pop non-top entries.
 *
 * Collectively, these utilities underscore the cppon framework's flexibility, robustness, and capability to represent
 * and manage complex data structures and relationships. They are instrumental in ensuring that cppon objects can be
 * dynamically linked, accessed, and manipulated within a coherent and stable framework.
 */

namespace visitors {
void push_root(const cppon& root);
void pop_root(const cppon& root) noexcept;
cppon& get_root() noexcept;

cppon& visitor(cppon& object, size_t index);
cppon& visitor(cppon& object, string_view_t index);
const cppon& visitor(const cppon& object, size_t index);
const cppon& visitor(const cppon& object, string_view_t index);
} //namespace visitors

class root_guard {
    cppon& new_root;
public:
    root_guard(const cppon& new_root_arg) noexcept
        : new_root(const_cast<cppon&>(new_root_arg)) {
        visitors::push_root(new_root);
    }
    ~root_guard() noexcept {
        visitors::pop_root(new_root);
    }
};

// Type trait to check if a type is contained in a std::variant and is an r-value reference
template<typename T, typename Variant>
struct is_in_variant_rv;

template<typename T, typename... Types>
struct is_in_variant_rv<T, std::variant<Types...>>
    : std::disjunction<std::is_same<std::decay_t<T>, Types>..., std::is_rvalue_reference<T>> {};

// Type trait to check if a type is contained in a std::variant and is an l-value reference
template<typename T, typename Variant>
struct is_in_variant_lv;

template<typename T, typename... Types>
struct is_in_variant_lv<T, std::variant<Types...>>
    : std::disjunction<std::is_same<std::decay_t<T>, Types>..., std::negation<std::is_rvalue_reference<T>>> {};

/**
 * @brief The cppon class represents a versatile container for various types used within the cppon framework.
 *
 * This class extends from `value_t`, which is an alias for a `std::variant` that encapsulates all types managed by cppon.
 * It provides several utility functions and operator overloads to facilitate dynamic type handling and hierarchical data
 * structure manipulation.
 *
 * - `is_null()`: Checks if the current instance holds a null value.
 *
 * - `operator[](index_t index)`: Overloaded subscript operators for non-const and const contexts. These operators allow
 *   access to nested cppon objects by index. If the root is null, the current instance is set as the root.
 *
 * - `operator=(const T& val)`: Template assignment operators for copying and moving values into the cppon instance.
 *
 * - `operator=(const char* val)`: Assignment operator for C-style strings, converting them to `string_view_t` before assignment.
 *
 * - `operator=(const string_t& val)`: Assignment operator for `string_t` type.
 *
 * The class leverages `std::visit` to handle the variant types dynamically, ensuring that the correct type-specific
 * operations are performed. This design allows cppon to manage complex data structures and relationships efficiently.
 */

class cppon : public value_t {
public:
    auto is_null() const -> bool {return std::holds_alternative<nullptr_t>(*this);}

    cppon() = default;
    cppon(const cppon&) = default;
    cppon(cppon&&) noexcept = default;
    ~cppon() noexcept { visitors::pop_root(*this); }
    cppon& operator=(const cppon&) = default;
    cppon& operator=(cppon&&) noexcept = default;

    auto operator[](string_view_t index)->cppon& {
        CPPON_ASSERT(!index.empty() && "Index cannot be empty");
        if (index.front() == '/') {
            visitors::push_root(*this);
            return visitors::visitor(visitors::get_root(), index.substr(1));
        }
        return visitors::visitor(*this, index);
    }

    auto operator[](string_view_t index)const->const cppon& {
        CPPON_ASSERT(!index.empty() && "Index cannot be empty");
        if (index.front() == '/') {
            visitors::push_root(*this);
            return visitors::visitor(static_cast<const cppon&>(visitors::get_root()), index.substr(1));
        }
        return visitors::visitor(*this, index);
    }

    auto operator[](size_t index)->cppon& {
        return visitors::visitor(*this, index);
    }

    auto operator[](size_t index)const->const cppon& {
        return visitors::visitor(*this, index);
    }

    auto& operator=(const char* val) {
        value_t::operator=(string_view_t{ val });
        return *this;
    }

    auto& operator=(pointer_t pointer) {
        if (pointer && pointer->valueless_by_exception()) {
            throw unsafe_pointer_assignment_error(
                "RHS points to a valueless_by_exception object. "
                "Sequence the operation or use path_t"
            );
        }
        value_t::operator=( pointer );
        return *this;
    }

    // Template for lvalue references, disabled for types not contained in value_t
    template<
        typename T,
        typename std::enable_if<is_in_variant_lv<const T, value_t>::value, int>::type = 0>
    auto& operator=(const T& val) { value_t::operator=(val); return *this; }

    // Template for rvalue references, disabled for types not contained in value_t
    template<
        typename T,
        typename std::enable_if<is_in_variant_rv<T, value_t>::value, int>::type = 0>
    auto& operator=(T&& val) { value_t::operator=(std::forward<T>(val)); return *this; }

    // Underlying container access (throws type_mismatch_error on wrong type)
    auto array() -> array_t& {
        if (auto p = std::get_if<array_t>(this)) return *p;
        throw type_mismatch_error{ "array_t expected" };
    }
    auto array() const -> const array_t& {
        if (auto p = std::get_if<array_t>(this)) return *p;
        throw type_mismatch_error{ "array_t expected" };
    }
    auto object() -> object_t& {
        if (auto p = std::get_if<object_t>(this)) return *p;
        throw type_mismatch_error{ "object_t expected" };
    }
    auto object() const -> const object_t& {
        if (auto p = std::get_if<object_t>(this)) return *p;
        throw type_mismatch_error{ "object_t expected" };
    }

    // Optional non-throw helpers
    auto try_array() noexcept -> array_t* { return std::get_if<array_t>(this); }
    auto try_array() const noexcept -> const array_t* { return std::get_if<array_t>(this); }
    auto try_object() noexcept -> object_t* { return std::get_if<object_t>(this); }
    auto try_object() const noexcept -> const object_t* { return std::get_if<object_t>(this); }
};

#ifdef CPPON_ENABLE_STD_GET_INJECTION
using std::get_if;
using std::get;
#endif

}//namespace ch5

#endif //CPPON_TYPES_H
