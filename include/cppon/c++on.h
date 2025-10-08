/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_H
#define CPPON_H

// -----------------------------------------------------------------------------
// C++ON version (Semantic Versioning: MAJOR.MINOR.PATCH)
// -----------------------------------------------------------------------------
#ifndef CPPON_VERSION_MAJOR
#define CPPON_VERSION_MAJOR 0
#endif
#ifndef CPPON_VERSION_MINOR
#define CPPON_VERSION_MINOR 1
#endif
#ifndef CPPON_VERSION_PATCH
#define CPPON_VERSION_PATCH 0
#endif

#ifndef CPPON_VERSION_STRING
#define CPPON_STR_IMPL(x) #x
#define CPPON_STR(x) CPPON_STR_IMPL(x)
#define CPPON_VERSION_STRING CPPON_STR(CPPON_VERSION_MAJOR) "." CPPON_STR(CPPON_VERSION_MINOR) "." CPPON_STR(CPPON_VERSION_PATCH)
#endif

#ifndef CPPON_VERSION_HEX
// 0x00MMmmpp (M=major, m=minor, p=patch) for simple numeric compares
#define CPPON_VERSION_HEX ((CPPON_VERSION_MAJOR << 16) | (CPPON_VERSION_MINOR << 8) | (CPPON_VERSION_PATCH))
#endif

namespace ch5 {
inline constexpr int cppon_version_major() noexcept { return CPPON_VERSION_MAJOR; }
inline constexpr int cppon_version_minor() noexcept { return CPPON_VERSION_MINOR; }
inline constexpr int cppon_version_patch() noexcept { return CPPON_VERSION_PATCH; }
inline constexpr const char* cppon_version_string() noexcept { return CPPON_VERSION_STRING; }
inline constexpr unsigned cppon_version_hex() noexcept { return CPPON_VERSION_HEX; }
}

/*
 * @brief Configuration macros
 *
 * #define CPPON_ENABLE_SIMD              - Opt-in: enable SIMD paths (SSE/AVX2/AVX512 if available)
 * #define CPPON_TRUSTED_INPUT            - Enable fast path for skip_spaces on trusted input
 * #define CPPON_ENABLE_STD_GET_INJECTION - Enable std::get injection into ch5 namespace
 * #define CPPON_IMPLICIT_CONVERSION      - Enable implicit conversion operators
 */

#ifndef CPPON_PATH_PREFIX
#define CPPON_PATH_PREFIX "$cppon-path:"
#endif

#ifndef CPPON_BLOB_PREFIX
#define CPPON_BLOB_PREFIX "$cppon-blob:"
#endif

#ifndef CPPON_NUMBER_PREFIX
#define CPPON_NUMBER_PREFIX "$cppon-number:"
#endif

#ifndef CPPON_MAX_ARRAY_DELTA
#define CPPON_MAX_ARRAY_DELTA 256
#endif

#ifndef CPPON_PRINTER_RESERVE_PER_ELEMENT
#define CPPON_PRINTER_RESERVE_PER_ELEMENT 16
#endif

#ifndef CPPON_OBJECT_MIN_RESERVE
#define CPPON_OBJECT_MIN_RESERVE 8
#endif

#ifndef CPPON_ARRAY_MIN_RESERVE
#define CPPON_ARRAY_MIN_RESERVE 8
#endif

#ifndef CPPON_OBJECT_SAFE_RESERVE
#define CPPON_OBJECT_SAFE_RESERVE 8
#endif

#ifndef CPPON_ARRAY_SAFE_RESERVE
#define CPPON_ARRAY_SAFE_RESERVE 8
#endif

#ifndef CPPON_ENABLE_SIMD
#define CPPON_USE_SIMD 0
#else
#define CPPON_USE_SIMD 1
#endif

#if defined(_MSC_VER)
  #if CPPON_USE_SIMD
    #pragma detect_mismatch("cppon_use_simd", "1")
  #else
    #pragma detect_mismatch("cppon_use_simd", "0")
  #endif
#endif

// -----------------------------------------------------------------------------
// SECTION: Platform & SIMD Utilities
// -----------------------------------------------------------------------------
// Platform-adaptive includes and type definitions for CPU feature detection
// and SIMD acceleration. This section provides the necessary low-level
// utilities for high-performance parsing and processing, and ensures
// portability across compilers and architectures.
// Namespaces: platform, simd
// -----------------------------------------------------------------------------

#if CPPON_USE_SIMD
#include "../platform/processor_features_info.h"

#include "../simd/simd_comparisons.h"
#endif

// -----------------------------------------------------------------------------
// SECTION: C++ON Core Headers
// -----------------------------------------------------------------------------
// Main C++ON components: exceptions, type alternatives, core types, parser,
// visitors, printer, and references. These headers define the public API and
// all core features of the library.
// -----------------------------------------------------------------------------

#include "c++on-exceptions.h"
#include "c++on-alternatives.h"
#include "c++on-printer-state.h"
#include "c++on-swar.h"
#include "c++on-types.h"
#include "c++on-thread.h"
#include "c++on-references.h"
#include "c++on-scanner.h"
#include "c++on-parser.h"
#include "c++on-printer.h"
#include "c++on-roots.h"
#include "c++on-visitors.h"
#include "c++on-literals.h"
#include "c++on-document.h"
#include "c++on-config.h"

#endif // CPPON_H