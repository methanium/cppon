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
#include <array>
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

/**
 * @brief Encodes a blob into a base64 string.
 *
 * This function encodes a blob into a base64 string using the standard base64 encoding.
 *
 * @param Blob The blob to encode.
 * @return string_t The base64-encoded string.
 */
inline auto encode_base64(const blob_t& Blob) -> string_t {
    constexpr const std::array<char, 64> base64_chars{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    string_t result;
    result.reserve((Blob.size() + 2) / 3 * 4);
    auto scan = Blob.data();
    auto end = scan + Blob.size();

    while (scan + 2 < end) {
        uint32_t buffer = (scan[0] << 16) | (scan[1] << 8) | scan[2];
        result.push_back(base64_chars[(buffer & 0x00FC0000) >> 18]);
        result.push_back(base64_chars[(buffer & 0x0003F000) >> 12]);
        result.push_back(base64_chars[(buffer & 0x00000FC0) >> 6]);
        result.push_back(base64_chars[(buffer & 0x0000003F)]);
        scan += 3;
    }

    if (scan < end) {
        uint32_t buffer = (scan[0] << 16);
        result.push_back(base64_chars[(buffer & 0x00FC0000) >> 18]);
        if (scan + 1 < end) {
            buffer |= (scan[1] << 8);
            result.push_back(base64_chars[(buffer & 0x0003F000) >> 12]);
            result.push_back(base64_chars[(buffer & 0x00000FC0) >> 6]);
            result.push_back('=');
        } else {
            result.push_back(base64_chars[(buffer & 0x0003F000) >> 12]);
            result.push_back('=');
            result.push_back('=');
        }
    }
    return result;
}

/**
 * @brief Decodes a base64 string into a blob.
 *
 * This function decodes a base64 string into a blob using the standard base64 decoding.
 *
 * @param Text The base64-encoded string to decode.
 * @param Raise Whether to raise an exception if an invalid character is encountered.
 * @return blob_t The decoded blob, or an empty blob if invalid and `Raise` is set to false.
 *
 * @throws invalid_base64_error if an invalid character is encountered and `Raise` is set to true.
 */
inline auto decode_base64(const string_view_t& Text, bool raise = true) -> blob_t {
    constexpr const std::array<uint8_t, 256> base64_decode_table{
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 0-15
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 16-31
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, // 32-47
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, // 48-63
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 64-79
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, // 80-95
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, // 112-127
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 128-143
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 144-159
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 160-175
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 176-191
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 192-207
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 208-223
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 224-239
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64  // 240-255
    };

    blob_t result;
    result.reserve((Text.size() + 3) / 4 * 3);
    auto scan = Text.data();
    auto end = scan + Text.size();

    while (scan < end) {
        uint32_t buffer = 0;
        int padding = 0;

        for (int i = 0; i < 4; ++i) {
            if (scan < end && *scan != '=') {
                uint8_t decoded_value = base64_decode_table[static_cast<uint8_t>(*scan)];
                if (decoded_value == 64) {
                    if (raise)
						throw invalid_base64_error{};
                    else
                        return blob_t{};
                }
                buffer = (buffer << 6) | decoded_value;
            } else {
                buffer <<= 6;
                ++padding;
            }
            ++scan;
        }

        result.push_back((buffer >> 16) & 0xFF);
        if (padding < 2) {
            result.push_back((buffer >> 8) & 0xFF);
        }
        if (padding < 1) {
            result.push_back(buffer & 0xFF);
        }
    }
	return result;
}

/**
 * @brief Converts a cppon value to its corresponding numeric type based on the specified NumberType.
 *
 * This function uses std::visit to handle different types stored in the cppon variant. If the type is
 * number_t, it converts the string representation of the number to the appropriate numeric type based on
 * the NumberType enum. If the type is already a numeric type, no conversion is performed. If the type is
 * neither number_t nor a numeric type, a type_mismatch_error is thrown.
 *
 * @param value The cppon value to be converted. This value is modified in place.
 *
 * @throws type_mismatch_error if the value is neither number_t nor a numeric type.
 * @throws std::invalid_argument or std::out_of_range if the conversion functions (e.g., std::strtoll) fail.
 *
 * @note The function uses compiler-specific directives (__assume, __builtin_unreachable) to indicate that
 * certain code paths are unreachable, which can help with optimization and avoiding compiler warnings.
 */
inline void convert_to_numeric(value_t& value) {
    std::visit([&](auto&& arg) {
        using U = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<U, number_t>) {
            string_view_t str_value = static_cast<string_view_t>(arg);
            switch (arg.type) {
                case NumberType::json_int64:
                    value = static_cast<int64_t>(std::strtoll(str_value.data(), nullptr, 10));
                    break;
                case NumberType::json_double:
                    value = static_cast<double_t>(std::strtod(str_value.data(), nullptr));
                    break;
                case NumberType::cpp_float:
                    value = static_cast<float_t>(std::strtof(str_value.data(), nullptr));
                    break;
                case NumberType::cpp_int8:
                    value = static_cast<int8_t>(std::strtol(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_uint8:
                    value = static_cast<uint8_t>(std::strtoul(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_int16:
                    value = static_cast<int16_t>(std::strtol(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_uint16:
                    value = static_cast<uint16_t>(std::strtoul(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_int32:
                    value = static_cast<int32_t>(std::strtol(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_uint32:
                    value = static_cast<uint32_t>(std::strtoul(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_int64:
                    value = static_cast<int64_t>(std::strtoll(str_value.data(), nullptr, 10));
                    break;
                case NumberType::cpp_uint64:
                    value = static_cast<uint64_t>(std::strtoull(str_value.data(), nullptr, 10));
                    break;
                default:
                    #if defined(_MSC_VER)
                        __assume(false);
                    #elif defined(__GNUC__) || defined(__clang__)
                        __builtin_unreachable();
                    #else
                        std::terminate(); // Fallback for other platforms
                    #endif
            }
        } else if constexpr (std::is_arithmetic_v<U>) {
            // No-op if it's already a numeric type
        } else {
            throw type_mismatch_error{};
        }
    }, value);
}

} // namespace ch5

#endif // CPPON_ALTERNATIVES_H
