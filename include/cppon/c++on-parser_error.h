/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-parser.h++ : JSON parsing and evaluation facilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_PARSER_H
#define CPPON_PARSER_H

#include "c++on-types.h"
#include "c++on-visitors.h"
#include <array>
#include <atomic>
#include <cctype>

// Using this dynamic solution
#if CPPON_USE_SIMD
    #include "../simd/simd_comparisons.h"
    #include "../platform/processor_features_info.h"

    namespace ch5 {
    namespace scanner {
        // Dynamic detection of SIMD capabilities
        enum class SimdLevel {
            None,
            SSE,
            AVX2,
            AVX512
        };

        // Maximum CPU-supported SIMD level (cached)
        inline SimdLevel max_supported_simd_level() noexcept {
            static SimdLevel max_level = []() {
                platform::processor_features_info cpu_info;
                auto features = cpu_info.cpu_features();
                if (features.AVX512F) return SimdLevel::AVX512;
                if (features.AVX2)    return SimdLevel::AVX2;
                if (features.SSE4_2)  return SimdLevel::SSE;
                return SimdLevel::None;
            }();
            return max_level;
        }
        inline SimdLevel cap_to_supported(SimdLevel lvl) noexcept {
            auto maxlvl = max_supported_simd_level();
            return (static_cast<int>(lvl) <= static_cast<int>(maxlvl)) ? lvl : maxlvl;
        }

        // Global process-wide override; -1 = none
        inline namespace simd_config {
            inline std::atomic<int> g_simd_override_global{ -1 };
            inline thread_local int g_simd_override_thread = -1;
            inline std::atomic<uint64_t> g_simd_cfg_epoch{ 0 };

            inline void set_simd_override_global(SimdLevel lvl) noexcept {
                g_simd_override_global.store(static_cast<int>(cap_to_supported(lvl)), std::memory_order_relaxed);
                g_simd_cfg_epoch.fetch_add(1, std::memory_order_relaxed);
            }
            inline void clear_simd_override_global() noexcept {
                g_simd_override_global.store(-1, std::memory_order_relaxed);
                g_simd_cfg_epoch.fetch_add(1, std::memory_order_relaxed);
            }
            inline void set_simd_override_thread(SimdLevel lvl) noexcept {
                g_simd_override_thread = static_cast<int>(cap_to_supported(lvl));
                g_simd_cfg_epoch.fetch_add(1, std::memory_order_relaxed);
            }
            inline void clear_simd_override_thread() noexcept {
                g_simd_override_thread = -1;
                g_simd_cfg_epoch.fetch_add(1, std::memory_order_relaxed);
            }

            inline bool has_simd_override() noexcept {
                return g_simd_override_thread >= 0 || g_simd_override_global.load(std::memory_order_relaxed) >= 0;
            }
            inline SimdLevel current_simd_override() noexcept {
                if (g_simd_override_thread >= 0) return static_cast<SimdLevel>(g_simd_override_thread);
                int v = g_simd_override_global.load(std::memory_order_relaxed);
                return v < 0 ? SimdLevel::None : static_cast<SimdLevel>(v);
            }
        }

        inline SimdLevel detect_simd_level() {
            // Honor override first
            if (has_simd_override()) {
                return cap_to_supported(current_simd_override());
            }
            return max_supported_simd_level();
        }

        // Lazily bound function pointer dispatch
        using find_quote_fn = size_t(*)(std::string_view, size_t);
        using scan_digits_fn = const char* (*)(std::string_view, size_t);

        // Scalar implementation (fallback)
        inline size_t scalar_find_quote_pos(std::string_view text, size_t start = 0) {
            return text.find('"', start);
        }
        inline const char* scalar_scan_digits(std::string_view text, size_t start = 0) {
            const char* ptr = text.data() + start;
            while (*ptr && static_cast<unsigned char>(*ptr - '0') <= 9) ++ptr;
            return ptr;
        }

        // Active function pointers (per thread)
        inline thread_local find_quote_fn  p_find_quote = nullptr;
        inline thread_local scan_digits_fn p_scan_digits = nullptr;

        // Per-thread epoch cache to avoid calling detect_simd_level() every time
        inline thread_local uint64_t t_last_epoch = ~uint64_t{0};
        inline thread_local bool t_bound_once = false;
        
        inline void bind_dispatch(SimdLevel lvl) noexcept {
            switch (lvl) {
            case SimdLevel::AVX512:
                p_find_quote = simd::find_quote_pos_avx512;
                p_scan_digits = simd::scan_digits_avx512;
                break;
            case SimdLevel::AVX2:
                p_find_quote = simd::find_quote_pos_avx2;
                p_scan_digits = simd::scan_digits_avx2;
                break;
            case SimdLevel::SSE:
                p_find_quote = simd::find_quote_pos_sse;
                p_scan_digits = simd::scan_digits_sse;
                break;
            default:
                p_find_quote = scalar_find_quote_pos;
                p_scan_digits = scalar_scan_digits;
                break;
            }
        }

        inline void ensure_dispatch_bound() noexcept {
            // Initial bind (no ongoing costly override check)
            if (!t_bound_once) {
                t_bound_once = true;
                bind_dispatch(detect_simd_level());
                t_last_epoch = g_simd_cfg_epoch.load(std::memory_order_relaxed);
                return;
            }
            uint64_t cur_epoch = g_simd_cfg_epoch.load(std::memory_order_relaxed);
            if (cur_epoch != t_last_epoch) {
                t_last_epoch = cur_epoch;
                bind_dispatch(detect_simd_level());
            }
        }

        // API: now using function pointers (no per-call switch)
        inline size_t find_quote_pos(std::string_view text, size_t start = 0) {
            ensure_dispatch_bound();
            return p_find_quote ? p_find_quote(text, start) : scalar_find_quote_pos(text, start);
        }

        inline const char* scan_digits(std::string_view text, size_t start = 0) {
            ensure_dispatch_bound();
            return p_scan_digits ? p_scan_digits(text, start) : scalar_scan_digits(text, start);
        }
    }
    }
#else
    // Implementations without SIMD
    namespace ch5 {
    namespace scanner {
        enum class SimdLevel {
            None,
            SSE,
            AVX2,
            AVX512
        };

        inline size_t find_quote_pos(std::string_view text, size_t start = 0) {
            return text.find('"', start);
        }

        inline const char* scan_digits(std::string_view text, size_t start = 0) {
            const char* ptr = text.data() + start;
            while (*ptr && static_cast<unsigned char>(*ptr - '0') <= 9) ++ptr;
            return ptr;
        }
    }
    }
#endif

namespace ch5 {

constexpr string_view_t path_prefix = CPPON_PATH_PREFIX;
constexpr string_view_t blob_prefix = CPPON_BLOB_PREFIX;
constexpr size_t object_min_reserve = CPPON_OBJECT_MIN_RESERVE;
constexpr size_t array_min_reserve = CPPON_ARRAY_MIN_RESERVE;

enum class options {
    full,  // full evaluation, with blob conversion
    eval,  // full evaluation, with conversion
    quick, // text evaluation (no conversion)
    parse  // parse only (validation)
};

constexpr options Full = options::full;
constexpr options Eval = options::eval;
constexpr options Quick = options::quick;
constexpr options Parse = options::parse;

namespace parser {

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
inline void convert_to_numeric(cppon& value) {
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
    }, static_cast<value_t&>(value));
}

/**
 * @brief Returns true if the character is considered whitespace.
 *
 * Modes:
 * - Default (strict JSON): SPACE (0x20), TAB (0x09), LF (0x0A), CR (0x0D).
 * - With CPPON_TRUSTED_INPUT defined: any control byte in [0x01..0x20] is treated as whitespace
 *   (branchless fast path). The null byte ('\0') is never considered whitespace.
 *
 * @param c Character to test.
 * @return true if c is whitespace according to the active mode; false otherwise.
 */
inline bool is_space(char c) {
#if defined(CPPON_TRUSTED_INPUT)
    // Branchless: treat 0x01..0x20 as whitespace; '\0' excluded.
    return static_cast<unsigned char>(c - 1) < 0x20u;
#else
    switch (static_cast<unsigned char>(c)) {
    case 0x20: case 0x09: case 0x0A: case 0x0D: return true;
    default: return false;
    }
#endif
}

/**
 * @brief Advances past JSON whitespace and updates the position in place.
 *
 * Scans forward starting at 'pos' until the first non‑whitespace character or the
 * terminating '\0' sentinel is reached. Assumes the underlying buffer is null‑terminated
 * (as guaranteed by cppon).
 *
 * Whitespace definition:
 * - Default (strict JSON): SPACE (0x20), TAB (0x09), LF (0x0A), CR (0x0D).
 * - With CPPON_TRUSTED_INPUT defined: any control byte in [0x01..0x20] is treated as whitespace
 *   (branchless fast path). The null byte ('\0') is never considered whitespace.
 *
 * On success, 'pos' is set to the index of the first non‑whitespace character.
 * If end of text is reached before any non‑whitespace, throws unexpected_end_of_text_error.
 *
 * @param text Input text to scan (must be backed by a null‑terminated buffer).
 * @param pos  In/out byte offset where scanning starts; updated to the first non‑whitespace.
 * @param err  Context string used in the exception message on end of text.
 * @throws unexpected_end_of_text_error If only whitespace remains up to the sentinel.
 */
inline void skip_spaces(string_view_t text, size_t& pos, const char* err) {
    const char* scan = text.data() + pos;
    if (is_space(*scan)) {
        do
            ++scan;
        while (is_space(*scan));
        pos = static_cast<size_t>(scan - text.data());
    }
    if (*scan)
        return;
    throw unexpected_end_of_text_error{ err };
}

/**
 * @brief Expects a specific character in the given text at the specified position.
 *
 * This function checks if the character at the specified position in the given text
 * matches the expected character.
 * If the characters do not match, an expected_symbol_error is thrown.
 *
 * @param text The text to check.
 * @param match The expected character.
 * @param pos The position in the text to check.
 *            'Pos' is updated to point to the next character after the expected character.
 *
 * @exception expected_symbol_error
 *            Thrown if the character at the specified position does not match the expected character.
 */
inline auto expect(string_view_t text, char match, size_t& pos) {
    auto scan{ text.data() + pos };
    if (*scan != match)
        throw expected_symbol_error{ string_view_t{ &match, 1 }, pos };
    ++pos;
}

/**
  * Forward declaration for arrays and objects
  */
inline auto accept_value(string_view_t text, size_t& pos, options opt) -> cppon;

/**
 * @brief Accepts a specific entity ("null", "true", or "false") from the given text.
 *
 * This function accepts a specific entity from the given text at the specified position.
 * The entity can be one of the following: "null", "true", or "false".
 * The function updates the `Pos` to point to the next character after the accepted entity.
 *
 * @param Text The text to accept the entity from.
 * @param Pos The position in the text to start accepting the entity.
 *            `Pos` is updated to point to the next character after the accepted entity.
 * @return cppon An instance of `cppon` containing the accepted entity.
 *
 * @exception expected_symbol_error
 *            Thrown if the characters at the specified position is not a valid entity.
 */
inline auto accept_null(string_view_t text, size_t& pos) -> cppon {
    if (text.compare(pos, 4, "null") == 0) {
        pos += 4;
        return cppon{nullptr};
    }
    throw expected_symbol_error{"null", pos};
}
inline auto accept_true(string_view_t text, size_t& pos) -> cppon {
    if (text.compare(pos, 4, "true") == 0) {
        pos += 4;
        return cppon{true};
    }
    throw expected_symbol_error{"true", pos};
}
inline auto accept_false(string_view_t text, size_t& pos) -> cppon {
    if (text.compare(pos, 5, "false") == 0) {
        pos += 5;
        return cppon{false};
    }
    throw expected_symbol_error{"false", pos};
}

/**
 * @brief Extracts a string from a JSON text, handling escape sequences.
 *
 * This function analyzes the JSON text starting from the position specified by `pos`
 * to extract a string. It correctly handles escape sequences such as \" and \\",
 * allowing quotes and backslashes to be included in the resulting string. Other
 * escape sequences are left as is, without transformation.
 *
 * @param text The JSON text to analyze.
 * @param pos The starting position in the text for analysis.
 *            `pos` is updated to point after the extracted string at the end of the analysis.
 * @return cppon An instance of `cppon` containing the extracted string.
 *
 * @exception unexpected_end_of_text_error
 *            Thrown if the end of the text is reached before finding a closing quote.
 */
inline auto accept_string(string_view_t text, size_t& pos, options opt) -> cppon {
    /*< expect(text, '"', pos); already done in accept_value */
    ++pos; // Advance the position to skip the opening quote
    size_t peek, found = pos - 1;
    do {
        found = scanner::find_quote_pos(text, found + 1);

        // If no quote is found and the end of the text is reached, throw an error
        if (found == text.npos)
            throw unexpected_end_of_text_error{ "string" };
        // Check for escaped characters
        peek = found;
        while (peek != pos && text[peek - 1] == '\\')
            --peek;
    } while (found - peek & 1); // Continue if the number of backslashes is odd

    // Update the position to point after the closing quote
    peek = pos;
    pos = found + 1;

    // Return the extracted substring
    auto value = text.substr(peek, found - peek);

    if(!value.empty() && value.front() == '$') {
        // If the path prefix is found, return a path object
        if (value.rfind(path_prefix, 0) == 0)
		    return cppon{ path_t{ value.substr(path_prefix.size()) } };

        // If the blob prefix is found, return a blob object
        if (value.rfind(blob_prefix, 0) == 0) {
            if (opt == options::full)
                // If the full option is set, return a blob object with the decoded value
			    return cppon{ decode_base64(value.substr(blob_prefix.size())) };
		    else
                // Otherwise, return a blob object with the encoded value
			    return cppon{ blob_string_t{ value.substr(blob_prefix.size()) } };
        }
	}

    // Otherwise, return a string object
    return cppon{ value };
}

/**
 * @brief Accepts a number from the given text at the specified position.
 *
 * This function accepts a number from the given text at the specified position.
 * The function updates the `pos` to point to the next character after the accepted number.
 * It handles different numeric formats based on the provided options.
 *
 * @param text The text to accept the number from.
 * @param pos The position in the text to start accepting the number.
 *            `pos` is updated to point to the next character after the accepted number.
 * @param opt The options for parsing the number, which determine the parsing behavior.
 * @return cppon An instance of `cppon` containing the accepted number.
 *
 * @exception unexpected_end_of_text_error
 *            Thrown if the end of the text is reached before finding a valid number.
 * @exception unexpected_symbol_error
 *            Thrown if an unexpected symbol is encountered while parsing the number.
 *
 * @note This function supports parsing of integers, floating-point numbers, and scientific notation.
 *       It uses the `convert_to_numeric` function to convert the parsed string to the appropriate numeric type.
 *       Although `convert_to_numeric` can throw `type_mismatch_error`, it is impossible in the context of this function
 *       because the input is always a valid numeric string when this function is called.
 */
inline auto accept_number(string_view_t text, size_t& pos, options opt) -> cppon {
   /**
     * JSON compatible types:
     * 0: int64  (without  suffix)
     * 1: double (without  suffix, with decimal point or exponent, also for C++ types)
     * Not JSON compatible types (C++ types):
     * 2: float  (with f   suffix and with decimal point or exponent)
     * 3: int8   (with i8  suffix)
     * 4: uint8  (with u8  suffix)
     * 5: int16  (with i16 suffix)
     * 6: uint16 (with u16 suffix)
     * 7: int32  (with i32 suffix)
     * 8: uint32 (with u32 suffix)
     • 9: int64  (with i/i64 suffix)
     •10: uint64 (with u/u64 suffix)
     */
    auto type = NumberType::json_int64;
    auto is_unsigned = false;
    auto has_suffix = false;
    auto start = pos;
    auto scan = &text[start];
    auto is_negative = scan[0] == '-';
    auto is_zero = scan[is_negative] == '0';

    scan += is_negative + is_zero;

    if (!is_zero) {
        const size_t ofs = static_cast<size_t>(scan - text.data());
        scan = scanner::scan_digits(text, ofs);
        //scan = scanner::scan_digits({ scan, size_t(&text.back() - scan + 1u) });
    }
    if (!scan) throw unexpected_end_of_text_error{ "number" };
    if (!(static_cast<unsigned char>(scan[-1] - '0') <= 9))
        throw unexpected_symbol_error{ string_view_t{ &scan[-1], 1u }, size_t((scan - &text[start]) - 1u) };
    // scan decimal point
    if (*scan == '.' && (static_cast<unsigned char>(scan[1] - '0') <= 9)) {
        // scan fractional part
        ++scan;
        const size_t ofs = static_cast<size_t>(scan - text.data());
        scan = scanner::scan_digits(text, ofs);
        //scan = scanner::scan_digits({ scan, size_t(&text.back() - scan + 1u) });
        if (!scan)
            throw unexpected_end_of_text_error{ "number" };
        type = NumberType::json_double; // double by default
    }
    else if (*scan == 'i' || *scan == 'I') {
        // scan unsigned flag
        ++scan;
        has_suffix  = true;
        is_unsigned = false;
    }
    else if (*scan == 'u' || *scan == 'U') {
        // scan unsigned flag
        ++scan;
        has_suffix  = true;
        is_unsigned = true;
    }
    // scan exponent
    if (!has_suffix && (*scan == 'e' || *scan == 'E')) {
        ++scan;
        // scan exponent sign
        if (*scan == '-' || *scan == '+') ++scan;
        // scan exponent digits
        while ((static_cast<unsigned char>(*scan - '0') <= 9))
            ++scan;
        if (!(static_cast<unsigned char>(scan[-1] - '0') <= 9))
            throw unexpected_symbol_error{ string_view_t{ &scan[-1], 1u },  size_t((scan - &text[pos]) - 1u) };
        type = NumberType::json_double; // double by default
    }
    if (type == NumberType::json_double && (*scan == 'f' || *scan == 'F')) {
		++scan;
		type = NumberType::cpp_float;
    }
    else if (has_suffix) {
        switch(*scan) {
        case '1':
			++scan;
			if (!*scan) throw unexpected_end_of_text_error{ "number" };
			if (*scan != '6') throw unexpected_symbol_error{ string_view_t{ scan, 1u }, size_t(scan - &text[start]) };
			++scan; type = NumberType::cpp_int16; break;
        case '3':
			++scan;
			if (!*scan) throw unexpected_end_of_text_error{ "number" };
			if (*scan != '2') throw unexpected_symbol_error{ string_view_t{ scan, 1u }, size_t(scan - &text[start]) };
			++scan; type = NumberType::cpp_int32; break;
        case '6':
			++scan;
			if (!*scan) throw unexpected_end_of_text_error{ "number" };
			if (*scan != '4') throw unexpected_symbol_error{ string_view_t{ scan, 1u }, size_t(scan - &text[start]) };
			++scan; type = NumberType::cpp_int64; break;
        case '8':
			++scan; type = NumberType::cpp_int8; break;
        default:
            // unnumbered suffix (i or u) defaults to cpp_int64/cpp_uint64
            type = NumberType::cpp_int64; break;
        }
	    if (is_unsigned) type = static_cast<NumberType>(static_cast<int>(type) + 1);
    }
    auto scan_size = scan - &text[start];
    pos += scan_size;

    cppon value{ number_t{ text.substr(start, scan_size), type } };

    if (opt < options::quick) // options:{full,eval}
        convert_to_numeric(value);

    return value;
}

/**
 * @brief Accepts an array from the given text at the specified position.
 *
 * This function accepts an array from the given text at the specified position.
 * An array can contain multiple values separated by commas.
 * The function updates the `pos` to point to the next character after the accepted array.
 *
 * @param text The text to accept the array from.
 * @param pos The position in the text to start accepting the array.
 *            `pos` is updated to point to the next character after the accepted array.
 * @param opt The options for parsing the array.
 * @return cppon An instance of `cppon` containing the accepted array.
 *
 * @exception unexpected_end_of_text_error
 *            Thrown if the end of the text is reached before finding a closing bracket for the array.
 * @exception unexpected_symbol_error
 *            Thrown if an unexpected symbol is encountered while parsing the array.
 */
inline auto accept_array(string_view_t text, size_t& pos, options opt) -> cppon {
    ++pos;
    skip_spaces(text, pos, "array");
    if (text[pos] == ']') {
        ++pos; // empty array
        return cppon{ array_t{} };
    }
    array_t array;
    if (opt != options::parse) // options:{eval,quick}
        array.reserve(array_min_reserve);
    do {
        skip_spaces(text, pos, "array");
        auto value = accept_value(text, pos, opt);
        skip_spaces(text, pos, "array");
        if (opt != options::parse) // options:{eval,quick}
            array.emplace_back(std::move(value));
    } while (text[pos] == ',' && ++pos);
    expect(text, ']', pos);
    return cppon{ std::move(array) };
}

/**
 * @brief Accepts an object from the given text at the specified position.
 *
 * This function accepts an object from the given text at the specified position.
 * An object contains key-value pairs separated by commas.
 * The function updates the `pos` to point to the next character after the accepted object.
 *
 * @param text The text to accept the object from.
 * @param pos The position in the text to start accepting the object.
 *            `pos` is updated to point to the next character after the accepted object.
 * @param opt The options for parsing the object.
 * @return cppon An instance of `cppon` containing the accepted object.
 *
 * @exception unexpected_end_of_text_error
 *            Thrown if the end of the text is reached before finding a closing brace for the object.
 * @exception unexpected_symbol_error
 *            Thrown if an unexpected symbol is encountered while parsing the object.
 */
inline auto accept_object(string_view_t text, size_t& pos, options opt) -> cppon {
     ++pos;
    skip_spaces(text, pos, "object");
    if (text[pos] == '}') {
        ++pos; // empty object
        return cppon{ object_t{} };
    }
    object_t object;
    if (opt != options::parse) // options:{eval,quick}
        object.reserve(object_min_reserve);
    do {
        skip_spaces(text, pos, "object");
        auto key = accept_string(text, pos, opt);
        skip_spaces(text, pos, "object");
        expect(text, ':', pos);
        skip_spaces(text, pos, "object");
        auto value = accept_value(text, pos, opt);
        skip_spaces(text, pos, "object");
        if (opt != options::parse) // options:{eval,quick}
            object.emplace_back(std::get<string_view_t>(key), std::move(value));
    } while (text[pos] == ',' && ++pos);
    expect(text, '}', pos);
    return cppon{ std::move(object) };
}

/**
 * @brief Accepts a value from the given text at the specified position.
 *
 * This function accepts a value from the given text at the specified position.
 * The value can be a string, number, object, array, true, false, or null.
 * The function updates the `pos` to point to the next character after the accepted value.
 *
 * @param text The text to accept the value from.
 * @param pos The position in the text to start accepting the value.
 *            `pos` is updated to point to the next character after the accepted value.
 * @param opt The options for parsing the value.
 * @return cppon An instance of `cppon` containing the accepted value.
 *
 * @exception unexpected_end_of_text_error
 *            Thrown if the end of the text is reached before finding a valid value.
 * @exception unexpected_symbol_error
 *            Thrown if an unexpected symbol is encountered while parsing the value.
 */
inline auto accept_value(string_view_t text, size_t& pos, options opt) -> cppon {
    switch (text[pos]) {
    case '"': return accept_string(text, pos, opt);
    case '{': return accept_object(text, pos, opt);
    case '[': return accept_array(text, pos, opt);
    case 'n': return accept_null(text, pos);
    case 't': return accept_true(text, pos);
    case 'f': return accept_false(text, pos);
    case '-': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        return accept_number(text, pos, opt);
    default:
        throw unexpected_symbol_error{ text.substr(pos, 1), pos };
    }
}

/**
 * @brief Evaluates a JSON text and returns the corresponding cppon structure.
 *
 * This function evaluates a JSON text and returns the corresponding cppon structure.
 * The JSON text is passed as a string view, and the options parameter allows for different
 * evaluation modes: Eval (full evaluation), Quick (text evaluation without conversion), and
 * Parse (parse only for validation).
 *
 * @param text The JSON text to evaluate.
 * @param opt The options for evaluating the JSON text. Default is Eval.
 * @return cppon The cppon structure representing the evaluated JSON text.
 */
inline auto eval(string_view_t text, options opt = Eval) -> cppon {
    if (text.empty())
        return cppon{ nullptr };

    if (text.size() >= 4 &&
        ((static_cast<unsigned char>(text[0]) == 0x00 && static_cast<unsigned char>(text[1]) == 0x00 &&
          static_cast<unsigned char>(text[2]) == 0xFE && static_cast<unsigned char>(text[3]) == 0xFF) ||
         (static_cast<unsigned char>(text[0]) == 0xFF && static_cast<unsigned char>(text[1]) == 0xFE &&
          static_cast<unsigned char>(text[2]) == 0x00 && static_cast<unsigned char>(text[3]) == 0x00))) {
        // UTF-32 BOM (not supported)
        throw unexpected_utf32_BOM_error{};
    }
    if (text.size() >= 2 &&
        // UTF-16 BOM (not supported)
        ((static_cast<unsigned char>(text[0]) == 0xFE && static_cast<unsigned char>(text[1]) == 0xFF) ||
         (static_cast<unsigned char>(text[0]) == 0xFF && static_cast<unsigned char>(text[1]) == 0xFE))) {
        throw unexpected_utf16_BOM_error{};
    }
    if ((static_cast<unsigned char>(text[0]) & 0xF8) == 0xF8) {
        // 5 or 6 byte sequence, not valid in UTF-8
        throw invalid_utf8_sequence_error{};
    }
    if ((static_cast<unsigned char>(text[0]) & 0xC0) == 0x80) {
        // continuation byte, not valid as first byte
        throw invalid_utf8_continuation_error{};
    }

    // At this point, only valid UTF-8 sequences remain:
    //   - ASCII bytes     (0xxxxxxx)       : (text[0] & 0x80) == 0x00
    //   - 2-byte sequence (110xxxxx)       : (text[0] & 0xE0) == 0xC0
    //   - 3-byte sequence (1110xxxx)       : (text[0] & 0xF0) == 0xE0
    //   - 4-byte sequence (11110xxx)       : (text[0] & 0xF8) == 0xF0

    // accept UTF-8 BOM
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.remove_prefix(3);
    }

    size_t pos{};
    skip_spaces(text, pos, "eval");

    // accept any value, not object only
    return accept_value(text, pos, opt);
}
template<size_t N>
inline auto eval(const char (&text)[N], options opt = Eval) -> cppon {
	return eval(string_view_t{ text, N ? N - 1 : 0 }, opt);
}


/**
 * @brief Retrieves the blob data from a cppon object.
 *
 * This function attempts to retrieve the blob data from a given cppon object. If the object contains a `blob_string_t`,
 * it decodes the base64 string into a `blob_t` and updates the cppon object. If the object already contains a `blob_t`,
 * it simply returns it. If the object contains neither, it throws a `type_mismatch_error`.
 *
 * @param value The cppon object from which to retrieve the blob data.
 * @param raise Whether to raise an exception if base64 decoding fails. If false, returns an empty blob instead.
 *              Defaults to true.
 * @return A reference to the blob data contained within the cppon object.
 *
 * @exception type_mismatch_error Thrown if the cppon object does not contain a `blob_string_t` or `blob_t`.
 * @exception invalid_base64_error Thrown if the base64 string is invalid and `raise` is set to true.
 */
inline blob_t& get_blob(cppon& value, bool raise = true) {
    return std::visit([&](auto&& arg) -> blob_t& {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_blob(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_blob(*arg);
        }
        else if constexpr (std::is_same_v<type, blob_string_t>) {
            value = decode_base64(arg, raise);
            return std::get<blob_t>(value);
        }
        else if constexpr (std::is_same_v<type, blob_t>) {
            return arg;
        }
        else {
            throw type_mismatch_error{};
        }
        }, value);
}

/**
 * @brief Retrieves a constant reference to blob data from a cppon object.
 *
 * This function attempts to retrieve a constant reference to the blob data from a given cppon object.
 * Unlike the non-const version, this function does not perform any conversion of `blob_string_t` to `blob_t`.
 * If the object contains a `blob_t`, it returns a reference to it. If the object contains a `blob_string_t`,
 * it throws a `blob_not_realized_error` since the blob must be decoded first. For all other types, it throws
 * a `type_mismatch_error`.
 *
 * @param value The cppon object from which to retrieve the blob data.
 * @return A constant reference to the blob data contained within the cppon object.
 *
 * @exception type_mismatch_error Thrown if the cppon object does not contain a `blob_t` or `blob_string_t`.
 * @exception blob_not_realized_error Thrown if the cppon object contains a `blob_string_t` (not yet decoded).
 */
inline const blob_t& get_blob(const cppon& value) {
    return std::visit([&](const auto& arg) -> const blob_t& {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_blob(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_blob(*arg);
        }
        else if constexpr (std::is_same_v<type, blob_string_t>) {
            throw blob_not_realized_error{};
        }
        else if constexpr (std::is_same_v<type, blob_t>) {
            return arg;
        }
        else {
            throw type_mismatch_error{};
        }
        }, value);
}

/**
 * @brief Retrieves a value of type T from a cppon value, ensuring strict type matching.
 *
 * This function attempts to retrieve a value of type T from the cppon variant. If the value is of type
 * number_t, it first converts it to the appropriate numeric type using convert_to_numeric. If the value
 * is already of type T, it is returned directly. If the value is of a different type, a type_mismatch_error
 * is thrown.
 *
 * @tparam T The type to retrieve. Must be a numeric type.
 * @param value The cppon value from which to retrieve the type T value.
 * @return T The value of type T.
 *
 * @throws type_mismatch_error if the value is not of type T or cannot be converted to type T.
 *
 * @note This function uses std::visit to handle different types stored in the cppon variant.
 */
template<typename T>
T get_strict(cppon& value) {
    static_assert(std::is_arithmetic_v<T>, "T must be a numeric type");
    return std::visit([&](auto&& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_strict<T>(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_strict<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            convert_to_numeric(value);
            return get_strict<T>(value);
        }
        else if constexpr (std::is_same_v<type, T>) {
            return arg;
        }
        else {
            throw type_mismatch_error{};
        }
        }, value);
}

template<typename T>
T get_strict(const cppon& value) {
    static_assert(std::is_arithmetic_v<T>, "T must be a numeric type");
    return std::visit([&](const auto& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_strict<T>(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_strict<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            throw number_not_converted_error{};
        }
        else if constexpr (std::is_same_v<type, T>) {
            return arg;
        }
        else {
            throw type_mismatch_error{};
        }
        }, value);
}

/**
 * @brief Retrieves a value of type T from a cppon value, allowing type casting.
 *
 * This function attempts to retrieve a value of type T from the cppon variant. If the value is of type
 * number_t, it first converts it to the appropriate numeric type using convert_to_numeric. If the value
 * is of a different numeric type, it is cast to type T. If the value is of a non-numeric type, a
 * type_mismatch_error is thrown.
 *
 * @tparam T The type to retrieve. Must be a numeric type.
 * @param value The cppon value from which to retrieve the type T value.
 * @return T The value of type T.
 *
 * @throws type_mismatch_error if the value is not a numeric type and cannot be cast to type T.
 *
 * @note This function uses std::visit to handle different types stored in the cppon variant.
 */
template<typename T>
T get_cast(cppon& value) {
    static_assert(std::is_arithmetic_v<T>, "T must be a numeric type");
    return std::visit([&](auto&& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_cast<T>(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_cast<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            convert_to_numeric(value);
            return get_cast<T>(value);
        }
        else if constexpr (std::is_arithmetic_v<type>) {
            return static_cast<T>(arg);
        }
        else {
            throw type_mismatch_error{};
        }
        }, value);
}

template<typename T>
T get_cast(const cppon& value) {
    static_assert(std::is_arithmetic_v<T>, "T must be a numeric type");
    return std::visit([&](auto&& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_cast<T>(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_cast<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            throw number_not_converted_error{};
        }
        else if constexpr (std::is_arithmetic_v<type>) {
            return static_cast<T>(arg);
        }
        else {
            throw type_mismatch_error{};
        }
        }, value);
}

template<typename T>
constexpr T* get_optional(cppon& value) noexcept {
    return std::visit([&](auto&& arg) -> T* {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_optional<T>(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_optional<T>(*arg);
        }
        else {
            return std::get_if<T>(&value);
        }
        }, value);
}

template<typename T>
constexpr const T* get_optional(const cppon& value) noexcept {
    return std::visit([&](auto&& arg) -> const T* {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_optional<T>(visitors::deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_optional<T>(*arg);
        }
        else {
            return std::get_if<T>(&value);
        }
        }, value);
}

}//namespace parser

using scanner::SimdLevel;

#if CPPON_USE_SIMD
// Public export of SIMD override helpers at ch5 scope
inline void set_global_simd_override(scanner::SimdLevel lvl) noexcept {
    scanner::set_simd_override_global(lvl);
}
inline void clear_global_simd_override() noexcept {
    scanner::clear_simd_override_global();
}
inline void set_thread_simd_override(scanner::SimdLevel lvl) noexcept {
    scanner::set_simd_override_thread(lvl);
}
inline void clear_thread_simd_override() noexcept {
    scanner::clear_simd_override_thread();
}
inline SimdLevel effective_simd_level() noexcept {
    return scanner::detect_simd_level();
}
#else
// No-SIMD build: keep API stable with no-op stubs
inline void set_global_simd_override(scanner::SimdLevel) noexcept {}
inline void clear_global_simd_override() noexcept {}
inline void set_thread_simd_override(scanner::SimdLevel) noexcept {}
inline void clear_thread_simd_override() noexcept {}
inline scanner::SimdLevel effective_simd_level() noexcept { return scanner::SimdLevel::None; }
#endif

using parser::eval;
using parser::get_blob;
using parser::get_strict;
using parser::get_cast;
using parser::get_optional;

}//namespace ch5


#endif // CPPON_PARSER_H
