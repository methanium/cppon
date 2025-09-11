/*
 * SIMD Comparisons for C++
 * https://github.com/methanium/cppon
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef SIMD_COMPARISONS_H
#define SIMD_COMPARISONS_H

/**
 * Note on *_parallel_digits:
 * - By design, the scalar epilogue scans tail+1 bytes beyond the [Ofs, Ofs+Count) window.
 *   Purpose: explicitly detect the trailing '\0' when the string_view is backed by a
 *   std::string or a string literal, so JSON that ends right after a digit (e.g. "1.0")
 *   is accepted.
 * - When scanning a sub-window (Count < Text.size() - Ofs), that extra byte may point to
 *   the first non-digit immediately after the window. This is intentional and safe within
 *   cppon, because all string_view passed here are backed by null-terminated buffers.
 * - If you work with non null-terminated buffers (e.g. vector<byte>, array<byte, N>),
 *   do not use these functions or ensure a readable sentinel right after the scanned window.
 *
 * Quote search functions (_parallel_find_quote) remain strictly bounded to [Ofs, Ofs+Count).
 */

#if defined(_MSC_VER)
    #if defined (__clang__)
        #include <x86intrin.h>
    #else
        #include <intrin.h>
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #include <x86intrin.h>

    #ifndef _tzcnt_u32
        #define _tzcnt_u32 __tzcnt_u32
    #endif
    #ifndef _tzcnt_u64
        #define _tzcnt_u64 __tzcnt_u64
    #endif
    #if !defined(__AVX512F__) && !defined(__mmask64)
        typedef unsigned long long __mmask64;
    #endif
#endif

#include <string_view>
#include <cstring>
#include <cassert>

// ----------------------------------------------------------------------------
// Optional quote scan cache (opt-in)
// ----------------------------------------------------------------------------
#if !defined(CPPON_ENABLE_QUOTE_CACHE)
#define CPPON_ENABLE_QUOTE_CACHE 1
#endif


/**
 * @brief SIMD-accelerated string and character comparison utilities
 *
 * This header provides a comprehensive set of functions for high-performance
 * string operations using SIMD instructions (SSE, AVX, AVX-512). These utilities
 * enable parallel processing of character data, significantly accelerating
 * common operations like character searching, digit validation, and string comparison.
 *
 * Each function is implemented in three variants to support different SIMD capabilities:
 * - xmm_* functions use SSE instructions (128-bit vectors)
 * - ymm_* functions use AVX instructions (256-bit vectors)
 * - zmm_* functions use AVX-512 instructions (512-bit vectors)
 *
 * The appropriate variant can be selected at runtime based on CPU capabilities.
 */
namespace simd {

/**
 * Scalar digits comparison helper
 */
inline auto scalar_digits(const char* p, size_t Count) -> const char* {
    while (Count--) {
        const unsigned c = static_cast<unsigned char>(*p);
        if (c < '0' || c > '9') return p;
        ++p;
    }
    return nullptr;
}

/**
  * Sequential comparison functions
  */
inline auto sequential_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    return scalar_digits(Text.data() + Ofs, Count + 1);
}
inline auto scalar_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return sequential_digits(Text, Ofs, Text.size() - Ofs);
}
inline auto scalar_digits(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return sequential_digits(Text, 0, Text.size());
}

// Scalar digits position helpers
inline auto sequential_digits_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
    assert(Text.size() >= Ofs + Count);
    const char* Found = sequential_digits(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto sequential_digits_pos(std::string_view Text, size_t Ofs) -> size_t {
    assert(Text.size() >= Ofs);
    const char* Found = sequential_digits(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto sequential_digits_pos(std::string_view Text) -> size_t {
    if (Text.empty()) return Text.npos;
    const char* Found = sequential_digits(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

// Scalar skip-spaces helper: returns first non-space within Count, or nullptr if none
inline auto scalar_skip_spaces(const char* p, size_t Count) -> const char* {
    const char* e = p + Count;
    while (p < e) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (c != 0x20 && c != '\t' && c != '\n' && c != '\r')
            return p;
        ++p;
    }
    return nullptr;
}

/*
// Original scalar version
inline auto skip_spaces(string_view_t text, size_t& pos, const char* err) {
    // Scalar fallback identical to the original implementation
    auto scan{ text.data() + pos };
    if (*scan) {
        if (is_space(*scan)) {
            ++scan;
            while (is_space(*scan))
                ++scan;
            pos = static_cast<size_t>(scan - text.data());
        }
        if (*scan)
            return;
    }
    throw unexpected_end_of_text_error{ err };
}
*/

enum class SimdLevel {
    None,
    SSE,
    AVX2,
    AVX512
};

template<SimdLevel Simd> struct simd_t;
template<> struct simd_t<SimdLevel::None> { using SimdType = char; };
template<> struct simd_t<SimdLevel::SSE> { using SimdType = __m128i; };
template<> struct simd_t<SimdLevel::AVX2> { using SimdType = __m256i; };
template<> struct simd_t<SimdLevel::AVX512> { using SimdType = __m512i; };

//template<typename Simd>
//struct simd_operations_t;

/**
 * Returns a mask indicating the comparison result between the bytes in vector V and the comparison vector or value.
 * These functions leverage SIMD instructions to perform byte-wise comparisons in parallel, enhancing performance
 * for operations that require examining multiple bytes simultaneously.
 *
 * The comparisons are as follows:
 * - (x|y|z)mm_mask_find_bytes_equal_to: Identifies bytes in V that are equal to the corresponding bytes in N.
 * - (x|y|z)mm_mask_find_bytes_less_than: Identifies bytes in V that are less than the corresponding bytes in N.
 * - (x|y|z)mm_mask_find_bytes_greater_than: Identifies bytes in V that are greater than the corresponding bytes in N.
 * - (x|y|z)mm_mask_find_bytes_not_in_range: Identifies bytes in V that are not in the inclusive range specified by vectors A and B.
 * - (x|y|z)mm_mask_find_bytes_in_range_exclusive: Identifies bytes in V that are within the exclusive range (A, B).
 * - (x|y|z)mm_mask_find_bytes_in_range: Identifies bytes in V that are within the inclusive range [A, B].
 * - (x|y|z)mm_mask_find_zero_bytes: Identifies bytes in V that are zero.
 *
 * Each function returns a mask where each bit corresponds to a byte in V. A set bit indicates that the byte
 * satisfies the comparison condition against the corresponding byte in the comparison vector or value.
 *
 * @param V The source vector for the comparison.
 * @param N The comparison vector or value (for functions that compare V against a single value or another vector).
 * @param A The lower bound vector for range comparisons.
 * @param B The upper bound vector for range comparisons.
 * @return A mask indicating the result of the comparison for each byte in V.
 */

namespace detail {
    struct quote_block_cache {
        const char* block_begin = nullptr;
        size_t      block_size = 0;     // bytes valid in block (≤ W)
        uint64_t    remaining = 0;     // mask of remaining quote positions (LSB = first byte)
        uint32_t    width = 0;     // 16 / 32 / 64
    };
} // namespace detail

template<typename T> struct simd_ops {
    static constexpr SimdLevel Level = SimdLevel::None;
    static constexpr int npos = 1;
    static constexpr size_t W = 1; // bits per vector
    static auto& get_cache() noexcept { static thread_local detail::quote_block_cache cache; return cache; }
    static int first(uint64_t Mask) { return Mask ? 0 : npos; } // 0..W-1 or W if none
    static char set1(unsigned char c) noexcept { return c; }
    static char loadA(const char* p) noexcept { return *p; }
    static char loadU(const char* p) noexcept { return *p; }
    static uint64_t widen_mask(int m) { return static_cast<uint8_t>(m); }
    static uint64_t eq_mask(__m128i a, __m128i b) { return static_cast<uint32_t>(a == b); }
    static uint64_t eq_quote_mask(__m128i a) { return static_cast<uint32_t>(a == '"'); }
    static uint64_t digit_outside_mask(unsigned char a) { return static_cast<uint32_t>((a - 0x48) > 0x9); }
    // Consume next set bit, return pointer or nullptr if none
    static const char* consume_next(const char* base) {
        auto& cache = get_cache();
        if (!cache.remaining) return nullptr;
        unsigned tz = first(cache.remaining);
        if (tz >= cache.block_size) return nullptr;
        return base + tz;
    }
    // Clear least-significant bit set
    static void advance_mask() {
        auto& cache = get_cache();
        cache.remaining &= (cache.remaining - 1);
    }
};
template<> struct simd_ops<__m128i> {
    constexpr static const __m128i Zero = {
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30 };
    constexpr static const __m128i Nine = {
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39 };
    constexpr static const __m128i Nin9 = {
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09 };
    constexpr static const __m128i Quote = {
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };
    static constexpr SimdLevel Level = SimdLevel::SSE;
    static constexpr int npos = 16;
    static constexpr size_t W = 16;
    static auto& get_cache() noexcept { static thread_local detail::quote_block_cache cache; return cache; }
    static int first(uint64_t Mask) { return static_cast<int>(_tzcnt_u32(static_cast<uint32_t>(Mask) | (1UL << W))); }
    static __m128i set1(unsigned char c) noexcept { return _mm_set1_epi8(c); }
    static __m128i loadA(const char* p) noexcept { return _mm_load_si128(reinterpret_cast<const __m128i*>(p)); }
    static __m128i loadU(const char* p) noexcept { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p)); }
    static uint64_t widen_mask(int m) { return static_cast<uint16_t>(m); }
    static uint64_t eq_mask(__m128i a, __m128i b) { return static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(a, b))); }
    static uint64_t eq_quote_mask(__m128i a) { return static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(a, Quote))); }
    static uint64_t digit_outside_mask(__m128i a) {
        return static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpgt_epi8(_mm_sub_epi8(a, Zero), Nin9)));
        //return static_cast<uint32_t>(_mm_movemask_epi8(_mm_or_si128(_mm_cmplt_epi8(a, Zero), _mm_cmpgt_epi8(a, Nine))));
    }
    // Consume next set bit, return pointer or nullptr if none
    static const char* consume_next(const char* base) {
        auto& cache = get_cache();
        if (!cache.remaining) return nullptr;
        unsigned tz = first(cache.remaining);
        if (tz >= cache.block_size) return nullptr;
        return base + tz;
    }
    // Clear least-significant bit set
    static void advance_mask() {
        auto& cache = get_cache();
        cache.remaining &= (cache.remaining - 1);
    }
};
template<> struct simd_ops<__m256i> {
    constexpr static const __m256i Zero = {
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30 };
    constexpr static const __m256i Nine = {
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39 };
    constexpr static const __m256i Nin9 = {
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09 };
    constexpr static const __m256i Quote = {
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };
    static constexpr SimdLevel Level = SimdLevel::AVX2;
    static constexpr int npos = 32;
    static constexpr size_t W = 32;
    static auto& get_cache() noexcept { static thread_local detail::quote_block_cache cache; return cache; }
    static int first(uint64_t Mask) { return static_cast<uint32_t>(Mask) ? static_cast<int>(_tzcnt_u32(static_cast<uint32_t>(Mask))) : npos; }
    static __m256i set1(unsigned char c) noexcept { return _mm256_set1_epi8(c); }
    static __m256i loadA(const char* p) noexcept { return _mm256_load_si256(reinterpret_cast<const __m256i*>(p)); }
    static __m256i loadU(const char* p) noexcept { return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)); }
    static uint64_t widen_mask(int m) { return static_cast<uint32_t>(m); }
    static uint64_t eq_mask(__m256i a, __m256i b) { return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b))); }
    static uint64_t eq_quote_mask(__m256i a) { return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(a, Quote))); }
    static uint64_t digit_outside_mask(__m256i a) {
        return static_cast<uint32_t>(_mm256_cmpgt_epi8_mask(_mm256_sub_epi8(a, Zero), Nin9));
        //return static_cast<uint64_t>(_mm256_cmplt_epi8_mask(a, Zero) | _mm256_cmpgt_epi8_mask(a, Nine));
    }
    // Consume next set bit, return pointer or nullptr if none
    static const char* consume_next(const char* base) {
        auto& cache = get_cache();
        if (!cache.remaining) return nullptr;
        unsigned tz = first(cache.remaining);
        if (tz >= cache.block_size) return nullptr;
        return base + tz;
    }
    // Clear least-significant bit set
    static void advance_mask() {
        auto& cache = get_cache();
        cache.remaining &= (cache.remaining - 1);
    }
};
template<> struct simd_ops<__m512i> {
    constexpr static const __m512i Zero = {
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30 };
    constexpr static const __m512i Nine = {
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
        0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39 };
    constexpr static const __m512i Nin9 = {
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,
        0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09 };
    constexpr static const __m512i Quote = {
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
        0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };
    static constexpr SimdLevel Level = SimdLevel::AVX512;
    static constexpr int npos = 64;
    static constexpr size_t W = 64;
    static auto& get_cache() noexcept { static thread_local detail::quote_block_cache cache; return cache; }
    static int first(uint64_t Mask) { return Mask ? static_cast<int>(_tzcnt_u64(Mask)) : npos; }
    static __m512i set1(unsigned char c) noexcept { return _mm512_set1_epi8(c); }
    static __m512i loadA(const char* p) noexcept { return _mm512_load_si512(reinterpret_cast<const __m512i*>(p)); }
    static __m512i loadU(const char* p) noexcept { return _mm512_loadu_si512(reinterpret_cast<const __m512i*>(p)); }
    static uint64_t widen_mask(int64_t m) { return static_cast<uint64_t>(m); }
    static uint64_t eq_mask(__m512i a, __m512i b) { return static_cast<uint64_t>(_mm512_cmpeq_epi8_mask(a, b)); }
    static uint64_t eq_quote_mask(__m512i a) { return static_cast<uint64_t>(_mm512_cmpeq_epi8_mask(a, Quote)); }
    static uint64_t digit_outside_mask(__m512i a) {
        return static_cast<uint32_t>(_mm512_cmpgt_epi8_mask(_mm512_sub_epi8(a, Zero), Nin9));
        //return static_cast<uint64_t>(_mm512_cmplt_epi8_mask(a, Zero) | _mm512_cmpgt_epi8_mask(a, Nine));
    }
    // Consume next set bit, return pointer or nullptr if none
    static const char* consume_next(const char* base) {
        auto& cache = get_cache();
        if (!cache.remaining) return nullptr;
        unsigned tz = first(cache.remaining);
        if (tz >= cache.block_size) return nullptr;
        return base + tz;
    }
    // Clear least-significant bit set
    static void advance_mask() {
        auto& cache = get_cache();
        cache.remaining &= (cache.remaining - 1);
    }
};

// Quote cache unified (reuse existing struct)
namespace detail {
template<typename V> inline const char* cached_quote_advance(const char* p) {
    auto& cache = simd_ops<V>::get_cache();
    if (!cache.block_begin || !cache.remaining) return nullptr;
    if (p < cache.block_begin || p >= cache.block_begin + cache.block_size) return nullptr;
    size_t consumed = static_cast<size_t>(p - cache.block_begin);
    uint64_t mask = cache.remaining & ~((consumed ? (1ULL << consumed) : 0ULL) - 0ULL);
    if (!mask) return nullptr;
    unsigned tz = simd_ops<V>::first(mask);
    cache.remaining &= ~(1ULL << tz);
    return cache.block_begin + tz;
}
} // detail


//static bool mask_find_first_byte_equal_to(const char a, const char b) noexcept { return a == b; }
//static bool mask_find_first_byte_less_than(const char a, const char b) noexcept { return a < b; }
//static bool mask_find_first_bmask_yte_greater_than(const char a, const char b) noexcept { return a > b; }
//static bool mask_find_zero_bytes(const char x) noexcept { return !x; }
//static bool mask_find_bytes_not_in_range(const char x, const char a, const char b) noexcept { return unsigned(x - a) > unsigned(b - a); }
//static bool mask_find_bytes_in_range_exclusive(const char x, const char a, const char b) noexcept { return unsigned(x - a) <= unsigned(b - a); }
//static bool mask_find_bytes_in_range(const char x, const char a, const char b) noexcept { return !mask_find_bytes_not_in_range(x, a, b); }
//
//static int mask_find_first_byte_equal_to(__m128i a, __m128i b) noexcept { return _mm_movemask_epi8(_mm_cmpeq_epi8(a, b)); }
//static int mask_find_first_byte_less_than(__m128i a, __m128i b) noexcept { return _mm_movemask_epi8(_mm_cmplt_epi8(a, b)); }
//static int mask_find_first_byte_greater_than(__m128i a, __m128i b) noexcept { return _mm_movemask_epi8(_mm_cmpgt_epi8(a, b)); }
//static int mask_find_zero_bytes(__m128i x) noexcept { return find_first_byte_equal_to(x, _mm_setzero_si128()); }
//static int mask_find_bytes_not_in_range(__m128i x, __m128i a, __m128i b) noexcept {
//    return _mm_movemask_epi8(_mm_or_si128(_mm_cmplt_epi8(x, a), _mm_cmpgt_epi8(x, b)));
//}
//static int mask_find_bytes_in_range_exclusive(__m128i x, __m128i a, __m128i b) noexcept {
//    return _mm_movemask_epi8(_mm_and_si128(_mm_cmpgt_epi8(x, a), _mm_cmplt_epi8(x, b)));
//}
//static int mask_find_bytes_in_range(__m128i x, __m128i a, __m128i b) noexcept {
//    return ~mask_find_bytes_not_in_range(x, a, b);
//}
//
//static int mask_find_first_byte_equal_to(__m256i a, __m256i b) noexcept { return _mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b)); }
//static int mask_find_first_byte_less_than(__m256i a, __m256i b) noexcept { return _mm256_movemask_epi8(_mm256_cmpgt_epi8(b, a)); }
//static int mask_find_first_byte_greater_than(__m256i a, __m256i b) noexcept { return _mm256_movemask_epi8(_mm256_cmpgt_epi8(a, b)); }
//static int mask_find_zero_bytes(__m256i x) noexcept { return find_first_byte_equal_to(x, _mm256_setzero_si256()); }
//static int mask_find_bytes_not_in_range(__m256i x, __m256i a, __m256i b) noexcept {
//    return _mm256_movemask_epi8(_mm256_or_si256(_mm256_cmpgt_epi8(a, x), _mm256_cmpgt_epi8(x, b)));
//}
//static int mask_find_bytes_in_range_exclusive(__m256i x, __m256i a, __m256i b) noexcept {
//    return _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpgt_epi8(x, a), _mm256_cmpgt_epi8(b, x)));
//}
//static int mask_find_bytes_in_range(__m256i x, __m256i a, __m256i b) noexcept {
//    return ~mask_find_bytes_not_in_range(x, a, b);
//}
//
//static long long find_first_byte_equal_to(__m512i a, __m512i b) noexcept { return _mm512_cmpeq_epi8_mask(a, b); }
//static long long find_first_byte_less_than(__m512i a, __m512i b) noexcept { return _mm512_cmplt_epi8_mask(a, b)); }
//static long long find_first_byte_greater_than(__m512i a, __m512i b) noexcept { return _mm512_cmpgt_epi8_mask(a, b)); }
//static long long mask_find_zero_bytes(__m512i x) noexcept { return find_first_byte_equal_to(x, _mm512_setzero_si512()); }
//static long long mask_find_bytes_not_in_range(__m512i x, __m512i a, __m512i b) noexcept {
//    return (_mm512_cmplt_epi8_mask(x, a) | _mm512_cmpgt_epi8_mask(x, b));
//}
//static long long mask_find_bytes_in_range_exclusive(__m512i x, __m512i a, __m512i b) noexcept {
//    return (_mm512_cmpgt_epi8_mask(x, a) & _mm512_cmplt_epi8_mask(x, b));
//}
//static long long mask_find_bytes_in_range(__m512i x, __m512i a, __m512i b) noexcept {
//    return ~mask_find_bytes_not_in_range(x, a, b);
//}

/**
 * These functions are part of a suite designed to perform byte-wise comparisons between vectors using SIMD instructions for parallel processing.
 * Each function returns the index of the first byte in the source vector that meets a specific comparison criterion with respect to the given value(s) or range.
 * This approach significantly enhances performance for operations requiring the examination of multiple bytes simultaneously.
 *
 * - (x|y|z)mm_find_first_byte_equal_to: Finds the index of the first byte equal to a given value.
 * - (x|y|z)mm_find_first_byte_less_than: Finds the index of the first byte less than a given value.
 * - (x|y|z)mm_find_first_byte_greater_than: Finds the index of the first byte greater than a given value.
 * - (x|y|z)mm_find_first_byte_not_in_range: Finds the index of the first byte not in the inclusive range [A, B].
 * - (x|y|z)mm_find_first_byte_in_range_exclusive: Finds the index of the first byte in the exclusive range (A, B).
 * - (x|y|z)mm_find_first_byte_in_range: Finds the index of the first byte in the inclusive range [A, B].
 *
 * The comparison is performed using a mask generated by SIMD comparison operations. The index of the first set bit in this mask, corresponding to the first matching byte, is returned. If no match is found, mm_npos (16) is returned, indicating that the vector does not contain a byte that meets the criterion within the 16-byte (128-bit) vector limit.
 *
 * @param V The source vector for the comparison.
 * @param N The comparison value or vector (applicable to functions comparing against a single value or vector).
 * @param A The lower bound of the range for range comparisons.
 * @param B The upper bound of the range for range comparisons.
 * @return The index of the first byte meeting the comparison criterion, or mm_npos if no such byte is found.
 */

template<typename Simd> inline auto find_first_byte_equal_to(Simd V, Simd N) {
    return simd_operations_t<Simd>::get_first_match(mask_find_first_byte_equal_to(V, N));
}
template<typename Simd> inline auto find_first_byte_less_than(Simd V, Simd N) {
    return simd_operations_t<Simd>::get_first_match(mask_find_bytes_less_than(V, N));
}
template<typename Simd> inline auto find_first_byte_greater_than(Simd V, Simd N) {
    return simd_operations_t<Simd>::get_first_match(mask_find_bytes_greater_than(V, N));
}
template<typename Simd> inline auto find_zero_bytes(Simd x) noexcept {
    return simd_operations_t<Simd>::get_first_match(mask_find_zero_bytes(V));
}
template<typename Simd> inline auto find_first_byte_not_in_range(Simd V, Simd A, Simd B) {
    return ops::get_first_match(mask_find_bytes_not_in_range(V, A, B));
}
template<typename Simd> inline auto find_first_byte_in_range_exclusive(Simd V, Simd A, Simd B) {
    return simd_operations_t<Simd>::get_first_match(xmm_mask_find_bytes_in_range_exclusive(V, A, B));
}
template<typename Simd> inline auto find_first_byte_in_range(Simd V, Simd A, Simd B) {
    return simd_operations_t<Simd>::get_first_match(xmm_mask_find_bytes_in_range(V, A, B));
}

template<typename Simd> inline auto find_first_byte_equal_to(Simd V, uint8_t N) {
    return find_first_byte_equal_to(V, simd_operations_t<Simd>::set1(N));
}
template<typename Simd> inline auto find_first_byte_less_than(Simd V, uint8_t N) {
    return find_first_byte_less_than(V, simd_operations_t<Simd>::set1(N));
}
template<typename Simd> inline auto find_first_byte_greater_than(Simd V, uint8_t N) {
    return find_first_byte_greater_than(V, simd_operations_t<Simd>::set1(N));
}
template<typename Simd> inline auto find_first_byte_not_in_range(Simd V, uint8_t A, uint8_t B) {
    return find_first_byte_not_in_range(V, simd_operations_t<Simd>::set1(A), simd_operations_t<Simd>::set1(B));
}
template<typename Simd> inline auto find_first_byte_in_range_exclusive(Simd V, uint8_t A, uint8_t B) {
    return find_first_byte_in_range_exclusive(V, simd_operations_t<Simd>::set1(A), simd_operations_t<Simd>::set1(B));
}
template<typename Simd> inline auto find_first_byte_in_range(Simd V, uint8_t A, uint8_t B) {
    return find_first_byte_in_range(V, simd_operations_t<Simd>::set1(A), simd_operations_t<Simd>::set1(B));
}

/**
 * Calculates the index of the first set bit within the given mask, effectively finding the first match in a comparison mask.
 * This function is particularly useful in SIMD operations where a mask represents the result of byte-wise comparisons across
 * a 128-bit, 256-bit or 512-bit vector, allowing for rapid identification of the first byte that meets a specific condition.
 *
 * The function employs a trick to handle cases where no bits are set by OR-ing the mask with 0x10000 before counting trailing zeros.
 * This ensures that if no bits are set in the original mask (indicating no matches), the function returns mm_npos, a sentinel value
 * indicating an invalid position, which in this context is defined as 16, 32 or 64. This value is chosen because it is one more than
 * the maximum index within a 128-bit, 256-bit or 512-bit  vector, which can contain up to 16, 32 or 64 bytes.
 *
 * @param Mask The mask to search for the first set bit, typically obtained from SIMD comparison operations.
 * @return The index of the first set bit in the mask, or mm_npos if no bits are set, indicating no matches were found.
 */

///**
// * Parallel skip-spaces functions for 128-bit SSE vectors
// */
//inline auto xmm_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
//    assert(Text.size() >= Ofs + Count);
//    constexpr size_t W = 16u;
//    const char* p = Text.data() + Ofs;
//
//    const size_t blocks = Count / W;
//    const __m128i SP = _mm_set1_epi8(' ');
//    const __m128i TB = _mm_set1_epi8('\t');
//    const __m128i NL = _mm_set1_epi8('\n');
//    const __m128i CR = _mm_set1_epi8('\r');
//
//    for (size_t i = 0; i < blocks; ++i) {
//        const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
//        const __m128i m = _mm_or_si128(
//            _mm_or_si128(_mm_cmpeq_epi8(v, SP), _mm_cmpeq_epi8(v, TB)),
//            _mm_or_si128(_mm_cmpeq_epi8(v, NL), _mm_cmpeq_epi8(v, CR))
//        );
//        const uint32_t k = static_cast<uint32_t>(_mm_movemask_epi8(m));
//        const uint32_t non = (~k) & 0xFFFFu;
//        if (non) return p + static_cast<size_t>(_tzcnt_u32(non));
//        p += W;
//    }
//
//    const size_t tail = Count - blocks * W;
//    return scalar_skip_spaces(p, tail);
//}
//inline auto xmm_parallel_skip_spaces(std::string_view Text, size_t Ofs) -> const char* {
//    assert(Text.size() >= Ofs);
//    return xmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
//}
//inline auto xmm_parallel_skip_spaces(std::string_view Text) -> const char* {
//    if (Text.empty()) return nullptr;
//    return xmm_parallel_skip_spaces(Text, 0, Text.size());
//}
//inline auto xmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
//    assert(Text.size() >= Ofs + Count);
//    if (const char* p = xmm_parallel_skip_spaces(Text, Ofs, Count))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//inline auto xmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) -> size_t {
//    assert(Text.size() >= Ofs);
//    if (const char* p = xmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//inline auto xmm_parallel_skip_spaces_pos(std::string_view Text) -> size_t {
//    if (Text.empty()) return Text.npos;
//    if (const char* p = xmm_parallel_skip_spaces(Text, 0, Text.size()))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
///**
// * Parallel skip-spaces functions for 256-bit AVX vectors
// */
//inline auto ymm_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
//    assert(Text.size() >= Ofs + Count);
//    constexpr size_t W = 32u;
//    const char* p = Text.data() + Ofs;
//
//    const size_t blocks = Count / W;
//    const __m256i SP = _mm256_set1_epi8(' ');
//    const __m256i TB = _mm256_set1_epi8('\t');
//    const __m256i NL = _mm256_set1_epi8('\n');
//    const __m256i CR = _mm256_set1_epi8('\r');
//
//    for (size_t i = 0; i < blocks; ++i) {
//        const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
//        const __m256i m = _mm256_or_si256(
//            _mm256_or_si256(_mm256_cmpeq_epi8(v, SP), _mm256_cmpeq_epi8(v, TB)),
//            _mm256_or_si256(_mm256_cmpeq_epi8(v, NL), _mm256_cmpeq_epi8(v, CR))
//        );
//        const uint32_t k = static_cast<uint32_t>(_mm256_movemask_epi8(m));
//        const uint32_t non = ~k;
//        if (non) return p + static_cast<size_t>(_tzcnt_u32(non));
//        p += W;
//    }
//
//    const size_t tail = Count - blocks * W;
//    return scalar_skip_spaces(p, tail);
//}
//inline auto ymm_parallel_skip_spaces(std::string_view Text, size_t Ofs) -> const char* {
//    assert(Text.size() >= Ofs);
//    return ymm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
//}
//inline auto ymm_parallel_skip_spaces(std::string_view Text) -> const char* {
//    if (Text.empty()) return nullptr;
//    return ymm_parallel_skip_spaces(Text, 0, Text.size());
//}
//inline auto ymm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
//    assert(Text.size() >= Ofs + Count);
//    if (const char* p = ymm_parallel_skip_spaces(Text, Ofs, Count))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//inline auto ymm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) -> size_t {
//    assert(Text.size() >= Ofs);
//    if (const char* p = ymm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//inline auto ymm_parallel_skip_spaces_pos(std::string_view Text) -> size_t {
//    if (Text.empty()) return Text.npos;
//    if (const char* p = ymm_parallel_skip_spaces(Text, 0, Text.size()))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//
///**
// * Parallel skip-spaces functions for 512-bit AVX-512 vectors
// */
//inline auto zmm_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
//    assert(Text.size() >= Ofs + Count);
//    constexpr size_t W = 64u;
//    const char* p = Text.data() + Ofs;
//
//    const size_t blocks = Count / W;
//    const __m512i SP = _mm512_set1_epi8(' ');
//    const __m512i TB = _mm512_set1_epi8('\t');
//    const __m512i NL = _mm512_set1_epi8('\n');
//    const __m512i CR = _mm512_set1_epi8('\r');
//
//    for (size_t i = 0; i < blocks; ++i) {
//        const __m512i v = _mm512_loadu_si512(reinterpret_cast<const void*>(p));
//        const __mmask64 k = _mm512_cmpeq_epi8_mask(v, SP)
//                          | _mm512_cmpeq_epi8_mask(v, TB)
//                          | _mm512_cmpeq_epi8_mask(v, NL)
//                          | _mm512_cmpeq_epi8_mask(v, CR);
//        const __mmask64 non = ~k;
//        if (non) return p + static_cast<size_t>(zmm_get_first_match(non));
//        p += W;
//    }
//
//    const size_t tail = Count - blocks * W;
//    return scalar_skip_spaces(p, tail);
//}
//inline auto zmm_parallel_skip_spaces(std::string_view Text, size_t Ofs) -> const char* {
//    assert(Text.size() >= Ofs);
//    return zmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
//}
//inline auto zmm_parallel_skip_spaces(std::string_view Text) -> const char* {
//    if (Text.empty()) return nullptr;
//    return zmm_parallel_skip_spaces(Text, 0, Text.size());
//}
//inline auto zmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
//    assert(Text.size() >= Ofs + Count);
//    if (const char* p = zmm_parallel_skip_spaces(Text, Ofs, Count))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//inline auto zmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) -> size_t {
//    assert(Text.size() >= Ofs);
//    if (const char* p = zmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//inline auto zmm_parallel_skip_spaces_pos(std::string_view Text) -> size_t {
//    if (Text.empty()) return Text.npos;
//    if (const char* p = zmm_parallel_skip_spaces(Text, 0, Text.size()))
//        return static_cast<size_t>(p - Text.data());
//    return Text.npos;
//}
//
/**
 * Parallel find functions for 128-bit SSE vectors
 */
//template<SimdLevel Simd> inline auto parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) -> const char* {
//	using SimdType = simd_t<Simd>::SimdType;
//    using ops = simd_ops<SimdType>;
//    assert(Text.size() >= Ofs + Count);
//    if (Count == 0) return nullptr;
//
//    constexpr size_t W = 16u;
//    const auto Char = ops::set1(C);
//    const char* p = Text.data() + Ofs;
//
//    const size_t blocks = Count / W;
//    for (size_t i = 0; i < blocks; ++i) {
//        const int mask = mask_find_first_byte_equal_to(
//            ops::loadu(reinterpret_cast<const __m128i*>(p)),
//            Char
//        );
//        if (mask) return p + static_cast<size_t>(ops::get_first_match(mask));
//        p += W;
//    }
//
//    const size_t tail = Count - blocks * W;
//    return static_cast<const char*>(std::memchr(p, static_cast<unsigned char>(C), tail));
//}
//template<SimdLevel Simd> inline auto parallel_find(std::string_view Text, char C, size_t Ofs) {
//    assert(Text.size() >= Ofs);
//    return parallel_find<Simd>(Text, C, Ofs, Text.size() - Ofs);
//}
//template<SimdLevel Simd> inline auto parallel_find(std::string_view Text, char C) -> const char* {
//    if (Text.empty()) return nullptr;
//    return parallel_find<Simd>(Text, C, 0, Text.size());
//}

/**
 * Searches for the first occurrence of a specified character within a given string view, leveraging SIMD instructions
 * for efficient parallel processing. This function and its overloads aim to quickly locate the character by examining
 * multiple elements of the string simultaneously, enhancing search performance especially in large strings.
 *
 * The function has three overloads to accommodate different scenarios:
 * 1. A basic overload that searches the entire string view from the beginning for the specified character.
 * 2. An overload that allows specifying a starting offset, enabling the search to begin from a particular position
 *    within the string view.
 * 3. An overload that accepts both a starting offset and a count, restricting the search to a specific subrange
 *    of the string view.
 *
 * @param Text The string view to search within.
 * @param C The character to find within the string view.
 * @param Ofs (Optional) The starting offset in the string view from where the search should commence.
 * @param Count (Optional) The number of characters to include in the search, starting from Ofs.
 * @return The position of the first occurrence of the specified character within the string view, or std::string_view::npos
 *         if the character is not found. When Ofs and/or Count are specified, the returned position is relative to the
 *         beginning of the entire string view, not the beginning of the subrange.
 */

/**
 * Parallel find position functions for 128-bit SSE vectors
 */
template<SimdLevel Simd> inline auto parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = parallel_find<Simd>(Text, C, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
template<SimdLevel Simd> inline auto parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = parallel_find<Simd>(Text, C, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
template<SimdLevel Simd> inline auto parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = parallel_find<Simd>(Text, C, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Searches for the first occurrence of a quote character (") within a given string view, leveraging SIMD instructions
 * for efficient parallel processing. This function and its overloads aim to quickly locate the quote character by examining
 * multiple elements of the string simultaneously, enhancing search performance especially in large strings.
 *
 * The function has three overloads to accommodate different scenarios:
 * 1. A basic overload that searches the entire string view from the beginning for the quote character.
 * 2. An overload that allows specifying a starting offset, enabling the search to begin from a particular position
 *    within the string view.
 * 3. An overload that accepts both a starting offset and a count, restricting the search to a specific subrange
 *    of the string view.
 *
 * @param Text The string view to search within.
 * @param Ofs (Optional) The starting offset in the string view from where the search should commence.
 * @param Count (Optional) The number of characters to include in the search, starting from Ofs.
 * @return A pointer to the first occurrence of the quote character within the string view, or nullptr if the character is not found.
 *         When Ofs and/or Count are specified, the returned pointer is relative to the beginning of the entire string view, not the
 *         beginning of the subrange.
 */

/**
 * Parallel find functions for 128-bit SSE vectors
 */

template<SimdLevel Simd> auto parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    using SimdType = simd_t<Simd>::SimdType;
    using ops = simd_ops<SimdType>;
    using namespace simd::detail;
    assert(Text.size() >= Ofs + Count);
    if (Count == 0) return nullptr;

    constexpr size_t W = sizeof(SimdType);
    const char* base = Text.data();
    const char* p    = base + Ofs;

    if (const char* reuse = cached_quote_advance<SimdType>(p)) return reuse;

    // Optional alignment (simple)
	auto& cache = ops::get_cache();
    uintptr_t mis = reinterpret_cast<uintptr_t>(p) & (W - 1);
    if (mis) {
        size_t prefix = W - mis;
        if (prefix > Count) prefix = Count;
        if (const void* s = std::memchr(p, '"', prefix)) {
            cache.block_begin = nullptr; cache.remaining = 0;
            return static_cast<const char*>(s);
        }
        p += prefix; Count -= prefix;
    }
    const char* end = p + Count;
    while (p + W <= end) {
        auto v = ops::loadA(p);
        uint64_t m = ops::eq_quote_mask(v);
        if (m) {
            cache.block_begin = p;
            cache.block_size = W;
            cache.remaining = m;
            int tz = ops::first(m);
            cache.remaining &= ~(1ULL << tz);
            return p + tz;
        }
        p += W;
    }
    size_t tail = static_cast<size_t>(end - p);
    if (tail) {
        if (const void* s = std::memchr(p, '"', tail)) {
            cache.block_begin = nullptr; cache.remaining = 0;
            return static_cast<const char*>(s);
        }
    }
    cache.block_begin = nullptr; cache.remaining = 0;
    return nullptr;
}
template<SimdLevel Simd> inline auto parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return parallel_find_quote<Simd>(Text, Ofs, Text.size() - Ofs);
}
template<SimdLevel Simd> inline auto parallel_find_quote(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return parallel_find_quote<Simd>(Text, 0, Text.size());
}

template<SimdLevel Simd> auto find_quote_nocache(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    using SimdType = simd_t<Simd>::SimdType;
    using ops = simd_operations_t<SimdType>;
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = sizeof(SimdType);
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        int mask = mask_find_bytes_equal_to(ops::loadu(p), ops::Quote);
        if (mask) return p + ops::get_first_match(mask);
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, '"', tail));
}

/**
 * Searches for the first occurrence of a quote character (") within a given string view, leveraging SIMD instructions
 * for efficient parallel processing, and returns the position of the found character. This function and its overloads
 * aim to quickly locate the quote character by examining multiple elements of the string simultaneously, enhancing search
 * performance especially in large strings.
 *
 * The function has three overloads to accommodate different scenarios:
 * 1. A basic overload that searches the entire string view from the beginning for the quote character and returns its position.
 * 2. An overload that allows specifying a starting offset, enabling the search to begin from a particular position
 *    within the string view and returns its position.
 * 3. An overload that accepts both a starting offset and a count, restricting the search to a specific subrange
 *    of the string view and returns its position.
 *
 * @param Text The string view to search within.
 * @param Ofs (Optional) The starting offset in the string view from where the search should commence.
 * @param Count (Optional) The number of characters to include in the search, starting from Ofs.
 * @return The position of the first occurrence of the quote character within the string view, or std::string_view::npos if the character is not found.
 *         When Ofs and/or Count are specified, the returned position is relative to the beginning of the entire string view, not the
 *         beginning of the subrange.
 */

/**
 * Parallel find position functions for 128-bit SSE vectors
 */
template<SimdLevel Simd> inline auto parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = parallel_find_quote<Simd>(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
template<SimdLevel Simd> inline auto parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = parallel_find_quote<Simd>(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
template<SimdLevel Simd> inline auto parallel_find_quote_pos(std::string_view Text) -> size_t {
    auto Found = parallel_find_quote<Simd>(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Scans a given string view for digit characters (0-9) using SIMD instructions for efficient parallel processing.
 * This function and its overloads are designed to quickly traverse the string view, identifying and skipping over
 * digit characters. The scan stops at the first encountered character that is not a digit, effectively consuming
 * an initial segment of digits from the string. This approach is optimized for processing large text segments by
 * examining multiple characters simultaneously.
 *
 * The function has three overloads to accommodate different use cases:
 * 1. A basic overload that takes only the string view and starts scanning from the beginning.
 * 2. An overload that accepts a starting offset, allowing the scan to begin from a specific point in the string view.
 * 3. An overload that accepts both a starting offset and a count, limiting the scan to a specific range within the string view.
 *
 * @param Text The string view to scan for digit characters.
 * @param Ofs (Optional) The starting offset in the string view from where the scan should begin.
 * @param Count (Optional) The number of characters to include in the scan, starting from Ofs.
 * @return A pointer to the first non-digit character in the string view after any leading digits. If the string view
 *         consists entirely of digits (or the specified range contains only digits), the function returns a pointer
 *         to the character immediately following the last digit or nullptr if the end of the string view is reached.
 */

/**
 * Parallel comparison functions for 128-bit SSE vectors
 */
template<SimdLevel Simd> inline auto parallel_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    using SimdType = simd_t<Simd>::SimdType;
    using ops = simd_ops<SimdType>;
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = sizeof(SimdType);
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        auto mask = ops::digit_outside_mask(ops::loadU(p));
        if (mask) return p + ops::first(mask);
        p += W;
    }

    size_t tail = Count - blocks * W;
    return scalar_digits(p, tail + 1);
}
template<SimdLevel Simd> inline auto parallel_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return parallel_digits<Simd>(Text, Ofs, Text.size() - Ofs);
}
template<SimdLevel Simd> inline auto parallel_digits(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return parallel_digitsSimd > (Text, 0, Text.size());
}

/**
 * Consumes digit characters (0-9) from the beginning of the given string view and returns the position of the first non-digit character.
 * This family of functions leverages parallel processing to efficiently process large segments of the string, scanning from the specified
 * starting offset (if provided) and consuming consecutive digit characters. The scan stops when it encounters the first non-digit character
 * or reaches the end of the specified range. These functions are particularly useful for parsing numeric values from a string where the
 * numeric value is followed by non-numeric characters.
 *
 * There are three overloads of this function to accommodate different scenarios:
 * 1. A basic overload that takes only the string view and starts scanning from the beginning.
 * 2. An overload that accepts a starting offset, allowing the scan to begin from a specific point in the string view.
 * 3. An overload that accepts both a starting offset and a count, limiting the scan to a specific range within the string view.
 *
 * @param Text The string view to scan for digit characters.
 * @param Ofs (Optional) The starting offset in the string view from where the scan should begin. Defaults to the beginning of the string view.
 *            This parameter is only applicable to the second and third overloads.
 * @param Count (Optional) The number of characters to include in the scan, starting from Ofs. If not specified, the scan continues to the end of the string view.
 *              This parameter is only applicable to the third overload.
 * @return The zero-based position of the first non-digit character encountered after any leading digits, or std::string_view::npos if the end of the string view
 *         is reached without encountering a non-digit character. This effectively gives the length of the consumed segment if the scan starts at the beginning.
 */

/**
 * Parallel comparison functions for 128-bit SSE vectors
 */
template<SimdLevel Simd> inline auto parallel_digits_pos(std::string_view Text) -> size_t {
    auto Found = parallel_digits<Simd>(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
template<SimdLevel Simd> inline auto parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = parallel_digits<Simd>(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
template<SimdLevel Simd> inline auto parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = parallel_digits<Simd>(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Compares two string views using parallel processing to efficiently determine their lexicographical order.
 * This family of functions leverages SIMD instructions to compare large blocks of characters simultaneously,
 * enhancing performance for large strings. The comparison is performed up to the smallest length of the two
 * string views or the specified subrange within them.
 *
 * There are five overloads of this function to accommodate different comparison scenarios:
 * 1. A basic overload that compares the entire content of two string views from the beginning.
 * 2. An overload that accepts a starting offset for the first string view, allowing the comparison to begin
 *    from a specific point within the first string view.
 * 3. An overload that accepts a starting offset and a count for the first string view, limiting the comparison
 *    to a specific subrange within the first string view.
 * 4. An overload that accepts starting offsets for both string views, enabling the comparison to begin from
 *    specific points within both string views.
 * 5. An overload that accepts starting offsets and counts for both string views, restricting the comparison
 *    to specific subranges within both string views.
 *
 * @param Left The first string view to compare.
 * @param Right The second string view to compare.
 * @param OfsLeft (Optional) The starting offset in the first string view from where the comparison should begin.
 * @param CountLeft (Optional) The number of characters to compare in the first string view, starting from OfsLeft.
 * @param OfsRight (Optional) The starting offset in the second string view from where the comparison should begin.
 * @param CountRight (Optional) The number of characters to compare in the second string view, starting from OfsRight.
 * @return -1 if Left is lexicographically less than Right, 1 if Left is greater than Right, or 0 if they are equal.
 */

/**
 * Parallel comparison functions for 128-bit SSE vectors
 */
//template<SimdLevel Simd> inline auto xmm_parallel_compare(std::string_view Left, std::string_view Right) {
//    using SimdType = simd_t<Simd>::SimdType;
//    using ops = simd_operations_t<SimdType>;
//	size_t SimdSize = sizeof(SimdType);
//    auto StartLeft = Left.data())
//    auto StartRight = Right.data();
//    auto Stop = StartLeft + (Left.size() < Right.size() ? (Left.size() + SimdSize - 1) / SimdSize : (Right.size() + SimdSize - 1) / SimdSize);
//    auto DataLeft = StartLeft;
//    auto DataRight = StartRight;
//    int Pos;
//    while ((Pos = mask_find_bytes_equal_to(ops::loadu(DataLeft), ops:loadu(DataRight))) == 0xffff)
//        if ((DataRight += SimdSize, DataLeft += SimdSize) == Stop)
//            break;
//    if (Pos != 0xffff)
//    {
//        size_t CharPos = static_cast<size_t>((DataLeft - StartLeft) * SimdSize + ops:get_first_match(Pos ^ 0xffff));
//        auto CharLeft = CharPos < Left.size() ? Left[CharPos] : 0;
//        auto CharRight = CharPos < Right.size() ? Right[CharPos] : 0;
//        if (CharLeft < CharRight)
//            return -1;
//        else
//            return 1;
//    }
//    return 0;
//}
//template<SimdLevel Simd> inline auto parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft) {
//    assert(Left.size() > OfsLeft);
//    return parallel_compare<Simd>(Left.substr(OfsLeft), Right);
//}
//template<SimdLevel Simd> inline auto parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft) {
//    assert(Left.size() > OfsLeft);
//    return parallel_compare<Simd>(Left.substr(OfsLeft, CountLeft), Right);
//}
//template<SimdLevel Simd> inline auto parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight) {
//    assert(Left.size() > OfsLeft);
//    assert(Right.size() > OfsRight);
//    return parallel_compare<Simd>(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight));
//}
//template<SimdLevel Simd> inline auto parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight, size_t CountRight) {
//    assert(Left.size() > OfsLeft);
//    assert(Right.size() > OfsRight);
//    return parallel_compare<Simd>(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight, CountRight));
//}

// Quote position (returns index or npos)
inline size_t find_quote_pos_sse(std::string_view text, size_t ofs) {
    return parallel_find_quote_pos<SimdLevel::SSE>(text, ofs);
}
inline size_t find_quote_pos_avx2(std::string_view text, size_t ofs) {
    return parallel_find_quote_pos<SimdLevel::AVX2>(text, ofs);
}
inline size_t find_quote_pos_avx512(std::string_view text, size_t ofs) {
    return parallel_find_quote_pos<SimdLevel::AVX512>(text, ofs);
}

// Digit scanner (returns pointer to first non‑digit)
inline const char* scan_digits_sse(std::string_view text, size_t ofs) {
    return parallel_digits<SimdLevel::SSE>(text, ofs);
}
inline const char* scan_digits_avx2(std::string_view text, size_t ofs) {
    return parallel_digits<SimdLevel::AVX2>(text, ofs);
}
inline const char* scan_digits_avx512(std::string_view text, size_t ofs) {
    return parallel_digits<SimdLevel::AVX512>(text, ofs);
}

} // namespace simd

#endif // SIMD_COMPARISONS_H