/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-parser.h : JSON parsing and evaluation facilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_PARSER_H
#define CPPON_PARSER_H

#include "c++on-types.h"
#include "c++on-scanner.h"

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

}//namespace parser

using parser::eval;

}//namespace ch5


#endif // CPPON_PARSER_H
