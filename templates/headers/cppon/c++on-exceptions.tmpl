/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-exceptions.h : Exception classes for error handling
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_EXCEPTIONS_H
#define CPPON_EXCEPTIONS_H

#include <stdexcept>
#include <string_view>
#include <string>
#include <cassert>

#ifdef NDEBUG
#define CPPON_ASSERT(cond) ((void)0)
#else
#define CPPON_ASSERT(cond) assert(cond)
#endif

namespace ch5 {
namespace detail {

/**
 * @brief Translates special characters to their escape sequences.
 */
inline std::string_view translate_char(std::string_view sym) {
    switch (sym.front()) {
    case '\0': return "\\0";
    case '\r': return "\\r";
    case '\n': return "\\n";
    case '\t': return "\\t";
    default: return sym;
    }
}

} // namespace detail

/*EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
 * @section Scanner Exceptions
 *EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/

/**
 * @brief Exception for unexpected UTF-32 encoding.
 */
class unexpected_utf32_BOM_error : public std::runtime_error {
public:
    unexpected_utf32_BOM_error()
        : std::runtime_error{ "UTF-32 BOM detected: this parser only supports UTF-8 encoded JSON" } {}
};

/**
 * @brief Exception for unexpected UTF-16 encoding.
 */
class unexpected_utf16_BOM_error : public std::runtime_error {
public:
    unexpected_utf16_BOM_error()
        : std::runtime_error{ "UTF-16 BOM detected: this parser only supports UTF-8 encoded JSON" } {}
};

/**
 * @brief Exception for invalid UTF-8 sequence.
 */
class invalid_utf8_sequence_error : public std::runtime_error {
public:
    invalid_utf8_sequence_error()
        : std::runtime_error{ "Invalid UTF-8 sequence: 0xF8-0xFD bytes are never valid in UTF-8" } {}
};

/**
 * @brief Exception for unexpected UTF-8 continuation byte at start position.
 */
class invalid_utf8_continuation_error : public std::runtime_error {
public:
    invalid_utf8_continuation_error()
        : std::runtime_error{ "Invalid UTF-8 sequence: continuation byte detected at start position" } {}
};

/**
 * @brief Exception for unexpected end of text.
 */
class unexpected_end_of_text_error : public std::runtime_error {
public:
    explicit unexpected_end_of_text_error(std::string_view where = {})
        : std::runtime_error{ where.empty()
            ? "unexpected 'eot'"
            : std::string{"unexpected 'eot': "}.append(where) } {}
};

/**
 * @brief Exception for unexpected symbols.
 */
class unexpected_symbol_error : public std::runtime_error {
public:
    unexpected_symbol_error(std::string_view err, size_t pos)
        : std::runtime_error{ std::string{"'"}
            .append(detail::translate_char(err))
            .append("' unexpected at position ")
            .append(std::to_string(pos)) } {}
};

/**
 * @brief Exception for expected symbols.
 */
class expected_symbol_error : public std::runtime_error {
public:
    expected_symbol_error(std::string_view err, size_t pos)
        : std::runtime_error{ std::string{"'"}
            .append(detail::translate_char(err))
            .append("' expected at position ")
            .append(std::to_string(pos)) } {}
};

/*EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
 * @section Parser Exceptions
 *EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/

/**
 * @brief Exception for invalid base64 encoding.
 */
class invalid_base64_error : public std::runtime_error {
public:
    invalid_base64_error()
        : std::runtime_error{ std::string{"invalid base64"} } {}
};

/**
 * @brief Exception for blob not realized.
 */
class blob_not_realized_error : public std::runtime_error {
public:
    blob_not_realized_error()
        : std::runtime_error{ "attempted to access a blob that is not yet decoded (blob_string_t)" } {}
};

/**
 * @brief Exception for number not converted.
 */
class number_not_converted_error : public std::runtime_error {
public:
    number_not_converted_error()
        : std::runtime_error{ "number not yet converted in const context" } {}
};

/*EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
 * @section Visitor Exceptions
 *EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/

/**
 * @brief Exception for null values.
 */
class null_value_error : public std::runtime_error {
public:
    explicit null_value_error(std::string_view err = {})
        : std::runtime_error{ err.empty()
            ? "'null' value"
            : std::string{"'null' value: "}.append(err) } {}
};

/**
 * @brief Exception for type mismatches.
 */
class type_mismatch_error : public std::runtime_error {
public:
    explicit type_mismatch_error(std::string_view type = {})
        : std::runtime_error{ type.empty()
            ? "type mismatch"
            : std::string{"type mismatch: "}.append(type) } {}
};

/**
 * @brief Exception for missing members.
 */
class member_not_found_error : public std::runtime_error {
public:
    explicit member_not_found_error(std::string_view member = {})
        : std::runtime_error{ member.empty()
            ? "member not found"
            : std::string{"member not found: "}.append(member) } {}
};

/**
 * @brief Exception for bad array indices.
 */
class bad_array_index_error : public std::out_of_range {
public:
    explicit bad_array_index_error(std::string_view index = {})
        : std::out_of_range{ index.empty()
            ? "bad array index"
            : std::string{"bad array index: "}.append(index) } {}
};

/**
 * @brief Exception for invalid path segments.
 */
class invalid_path_segment_error : public std::runtime_error {
public:
    explicit invalid_path_segment_error(std::string_view segment = {})
        : std::runtime_error{ segment.empty()
            ? "invalid path segment"
            : std::string{"invalid path segment: "}.append(segment) } {}
};

/**
 * @brief Exception for invalid path value (whole path format).
 */
class invalid_path_error : public std::runtime_error {
public:
    explicit invalid_path_error(std::string_view detail = {})
        : std::runtime_error{ detail.empty()
            ? "invalid path"
            : std::string{"invalid path: "}.append(detail) } {}
};

/**
 * @brief Exception for excessive array resizing.
 */
class excessive_array_resize_error : public std::runtime_error {
public:
    explicit excessive_array_resize_error(std::string_view detail = {})
        : std::runtime_error{ detail.empty()
            ? "excessive array resize"
            : std::string{"excessive array resize: "}.append(detail) } {}
};

/*EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
 * @section Bad Logic Exceptions
 *EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/

 /**
  * @brief Exception for unsafe pointer assignment.
  */
class unsafe_pointer_assignment_error : public std::logic_error {
public:
    explicit unsafe_pointer_assignment_error(std::string_view detail = {})
        : std::logic_error{ detail.empty()
            ? "unsafe pointer assignment"
            : std::string{ "unsafe pointer assignment: " }.append(detail) } {}
};

/*EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
 * @section Printer Exceptions
 *EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/

/**
 * @brief Exception for bad options.
 */
class bad_option_error : public std::runtime_error {
public:
    explicit bad_option_error(std::string_view err)
        : std::runtime_error{ std::string{"bad "}.append(err) } {}
};

/**
 * @brief Exception for JSON compatibility issues.
 */
class json_compatibility_error : public std::runtime_error {
public:
    explicit json_compatibility_error(std::string_view detail)
        : std::runtime_error{ std::string{"JSON compatibility error: "}.append(detail) } {}
};

} // namespace ch5

#endif // CPPON_EXCEPTIONS_H
