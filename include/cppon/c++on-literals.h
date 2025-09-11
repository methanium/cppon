/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-literals.h : C++ON User-defined literals
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_LITERALS_H
#define CPPON_LITERALS_H

#include "c++on-parser.h"

// Inputs used by the parser must be NUL-terminated (sentinel '\0' readable at data()[size()]).
// UDLs below rely on const char* (no size_t) to preserve that invariant. For raw binary blobs,
// use (const char*, size_t) because embedded 0x00 are expected.

namespace ch5 {
namespace literals {

// Parse a JSON string literal to a cppon (full eval, blobs not decoded)
inline cppon operator"" _cpponf(const char* s, size_t n) {
    return eval(string_view_t{s, n}, options::eval);
}

// Parse a JSON string literal to a cppon (quick: lazy numbers)
inline cppon operator"" _cppon(const char* s, size_t n) {
    return eval(string_view_t{s, n}, options::quick);
}

// Parse an options object literal for to_string(...)
inline cppon operator"" _options(const char* s, size_t n) {
    return eval(string_view_t{s, n}, options::eval);
}

// Build a path_t from "/..."
inline path_t operator"" _path(const char* s, size_t n) {
#ifndef NDEBUG
    CPPON_ASSERT(s && s[0] == '/'); // path literals must start with '/'
#endif
    return path_t{ string_view_t{s, n} };
}

// Build a blob_string_t from base64 text (undecoded)
inline blob_string_t operator"" _blob64(const char* s, size_t n) {
    return blob_string_t{ string_view_t{s, n} };
}

// Build a raw binary blob (blob_t) directly from the literal bytes
inline blob_t operator"" _blob(const char* s, size_t n) {
    return blob_t{ reinterpret_cast<const uint8_t*>(s), reinterpret_cast<const uint8_t*>(s) + n };
}

// ------------------------------------------------------------
// Optional aliases (more idiomatic names, forward to above):
//   _jsonf -> _cpponf
//   _json  -> _cppon
//   _opts  -> _options
//   _b64   -> _blob64
// Keep both for ergonomics and doc alignment.
// ------------------------------------------------------------
inline cppon operator"" _jsonf(const char* s, size_t n) { return operator"" _cpponf(s, n); }
inline cppon operator"" _json(const char* s, size_t n) { return operator"" _cppon(s, n); }
inline cppon operator"" _opts(const char* s, size_t n) { return operator"" _options(s, n); }
inline blob_string_t operator"" _b64(const char* s, size_t n) { return operator"" _blob64(s, n); }

} // namespace literals
} // namespace ch5


#endif // CPPON_LITERALS_H
