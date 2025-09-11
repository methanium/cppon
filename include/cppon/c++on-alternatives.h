/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-alternatives.h - Type alternatives and definitions
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_ALTERNATIVES_H
#define CPPON_ALTERNATIVES_H

#include "c++on-exceptions.h"
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <variant>
#include <cmath>

namespace ch5 {

#ifndef float_t
    #if defined(__STDC_IEC_559__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64)))
        using float_t = float; // On x64 and ARM64, float_t is float
    #else
        using float_t = double; // On other architectures, float_t is double
    #endif
#endif
#ifndef double_t
        #if defined(__STDC_IEC_559__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64)))
        using double_t = double; // On x64 and ARM64, double_t is double
    #else
        using double_t = long double; // On other architectures, double_t is long double
    #endif
#endif

enum class NumberType {
    json_int64,
    json_double,
    cpp_float,
    cpp_int8,
    cpp_uint8,
    cpp_int16,
    cpp_uint16,
    cpp_int32,
    cpp_uint32,
    cpp_int64,
    cpp_uint64
 };

class cppon; // forward declaration for pointer_t, array_t, member_t (object_t)

// Represents a boolean value within the cppon framework.
using boolean_t = bool;

// Represents a pointer to a cppon object.
using pointer_t = cppon*;

// Represents a std::string_view value within the cppon framework.
using string_view_t = std::string_view;

// Represents a string value within the cppon framework.
using string_t = std::string;

// Represents a path value within the cppon framework as a string.
struct path_t {
    std::string_view value;
    path_t(std::string_view v) : value(v) {
        if (v.empty())    throw invalid_path_error{"empty path_t"};
        if (v[0] != '/')  throw invalid_path_error{"path_t must start with '/'"};
    }
    path_t(const char* v) : value(v) {
        if (v == nullptr) throw invalid_path_error{"empty path_t"};
        if (v[0] != '/')  throw invalid_path_error{"path_t must start with '/'"};
    }
    operator std::string_view() const { return value; }
};

// Represents a numeric text representation within the cppon framework as a string view.
struct number_t {
    std::string_view value;
    NumberType type;
    number_t(std::string_view v, NumberType t) : value(v), type(t) {}
    number_t(const char* v, NumberType t) : value(v), type(t) {}
    operator std::string_view() const { return value; }
};

// Represents a blob encoded as a base64 string within the cppon framework.
// This type is used for efficiently storing binary data in text format using base64 encoding.
struct blob_string_t {
	std::string_view value;
	blob_string_t(std::string_view v) : value(v) {}
    blob_string_t(const char* v) : value(v) {}
	operator std::string_view() const { return value; }
};

// Represents a blob value within the cppon framework as a vector of bytes.
using blob_t = std::vector<uint8_t>;

// Represents an array of cppon objects.
using array_t = std::vector<cppon>;

/**
 * @brief JSON object representation
 *
 * object_t is a std::vector<std::tuple<string_view_t, cppon>> rather than a (unordered_)map for performance and ergonomics:
 *
 * - Fast sequential access
 *   - Traversal and serialization dominate runtime; a compact vector has excellent cache locality.
 *   - Typical JSON objects are small (dozens of entries) : linear lookup is very competitive.
 *
 * - Stable insertion order
 *   - Order matters for debugging, logging, and some expected layouts; vector preserves it naturally.
 *
 * - Path-based construction
 *   - Lazy creation by segments (/a/b/c) performs well with push_back/emplace_back (O(1) amortized), no rehash/rebalance.
 *   - cppon's noexcept moves make reallocations safe and fast.
 *
 * - Minimal memory overhead
 *   - No buckets/nodes like unordered_map/map. For small objects, vector wins on footprint and speed.
 *
 * Known trade-offs
 * - O(n) lookup by key. For very large objects, consider:
 *   - A small auxiliary index on hot paths (built by the caller).
 *   - Optional sorting + binary search if order no longer matters (not enabled by default).
 *
 * Best practices
 * - Parser: reserve(object_min_reserve) to reduce reallocations.
 * - Access: assume linear lookup; factor repeated searches on the caller side if needed.
 */
using object_t = std::vector<std::tuple<string_view_t, cppon>>;

// Represents a variant type for cppon values.
// This type encapsulates all possible types of values that can be represented in the cppon framework,
// enabling the construction of complex, dynamically typed data structures akin to JSON.
using value_t = std::variant<
    object_t,      // 0
    array_t,       // 1
    double_t,      // 2
    float_t,       // 3
    int8_t,        // 4
    uint8_t,       // 5
    int16_t,       // 6
    uint16_t,      // 7
    int32_t,       // 8
    uint32_t,      // 9
    int64_t,       // 10
    uint64_t,      // 11
    number_t,      // 12
    boolean_t,     // 13
    string_view_t, // 14
    blob_string_t, // 15
    string_t,      // 16
    path_t,        // 17
    blob_t,        // 18
    pointer_t,     // 19
    nullptr_t>;    // 20

} // namespace ch5

#endif // CPPON_ALTERNATIVES_H
