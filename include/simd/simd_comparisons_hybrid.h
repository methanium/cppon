/*
 * SIMD Comparisons for C++
 * https://github.com/methanium/cppon
 *
 * MIT License
 * Copyright (c) 2023 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef SIMD_COMPARISONS_H
#define SIMD_COMPARISONS_H

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
#include <algorithm>
#include <cstdint> 
#include <cstring>
#include <cassert>

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

constexpr const __m128i xmm_Zero =  {
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30 };
constexpr const __m128i xmm_Nine =  {
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39 };
constexpr const __m128i xmm_Quote = {
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };

constexpr const __m256i ymm_Zero = {
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30 };
constexpr const __m256i ymm_Nine = {
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39 };
constexpr const __m256i ymm_Quote = {
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };

constexpr const __m512i zmm_Zero = {
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30 };
constexpr const __m512i zmm_Nine = {
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
    0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39 };
constexpr const __m512i zmm_Quote = {
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
    0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22 };

constexpr int xmm_npos = 16;
constexpr int ymm_npos = 32;
constexpr int zmm_npos = 64;

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
inline auto xmm_get_first_match(int Mask) {
    return _tzcnt_u32(Mask | (1UL << 16));
    }

inline auto ymm_get_first_match(int Mask) {
    if (Mask == 0)
        return ymm_npos;
    return (int)_tzcnt_u32(Mask);
    }

inline auto zmm_get_first_match(__mmask64 Mask) {
    if (Mask == 0)
        return zmm_npos;
    return (int)_tzcnt_u64(Mask);
    }

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

 /**
  * Comparison functions for 128-bit SSE vectors
  */
inline auto xmm_mask_find_bytes_equal_to(__m128i V, __m128i N) {
    return _mm_movemask_epi8(_mm_cmpeq_epi8(V, N));
}
inline auto xmm_mask_find_bytes_less_than(__m128i V, __m128i N) {
    return _mm_movemask_epi8(_mm_cmplt_epi8(V, N));
}
inline auto xmm_mask_find_bytes_greater_than(__m128i V, __m128i N) {
    return _mm_movemask_epi8(_mm_cmpgt_epi8(V, N));
}
inline auto xmm_mask_find_bytes_not_in_range(__m128i V, __m128i A, __m128i B) {
    return _mm_movemask_epi8(_mm_or_si128(_mm_cmplt_epi8(V, A), _mm_cmpgt_epi8(V, B)));
}
inline auto xmm_mask_find_bytes_in_range_exclusive(__m128i V, __m128i A, __m128i B) {
    return _mm_movemask_epi8(_mm_and_si128(_mm_cmpgt_epi8(V, A), _mm_cmplt_epi8(V, B)));
}
inline auto xmm_mask_find_bytes_in_range(__m128i V, __m128i A, __m128i B) {
    return ~xmm_mask_find_bytes_not_in_range(V, A, B);
}
inline auto xmm_mask_find_zero_bytes(__m128i V) {
    return xmm_mask_find_bytes_equal_to(V, _mm_setzero_si128());
}

/**
 * Comparison functions for 256-bit AVX vectors
 */
inline auto ymm_mask_find_bytes_equal_to(__m256i V, __m256i N) {
    return _mm256_movemask_epi8(_mm256_cmpeq_epi8(V, N));
}
inline auto ymm_mask_find_bytes_less_than(__m256i V, __m256i N) {
    return _mm256_movemask_epi8(_mm256_cmpgt_epi8(N, V)); // inversion for epi8
}
inline auto ymm_mask_find_bytes_greater_than(__m256i V, __m256i N) {
    return _mm256_movemask_epi8(_mm256_cmpgt_epi8(V, N));
}
inline auto ymm_mask_find_bytes_not_in_range(__m256i V, __m256i A, __m256i B) {
    return _mm256_movemask_epi8(_mm256_or_si256(_mm256_cmpgt_epi8(A, V), _mm256_cmpgt_epi8(V, B)));
}
inline auto ymm_mask_find_bytes_in_range_exclusive(__m256i V, __m256i A, __m256i B) {
    return _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpgt_epi8(V, A), _mm256_cmpgt_epi8(B, V)));
}
inline auto ymm_mask_find_bytes_in_range(__m256i V, __m256i A, __m256i B) {
    return ~ymm_mask_find_bytes_not_in_range(V, A, B);
}
inline auto ymm_mask_find_zero_bytes(__m256i V) {
    return ymm_mask_find_bytes_equal_to(V, _mm256_setzero_si256());
}

/**
 * Comparison functions for 512-bit AVX-512 vectors
 */
inline auto zmm_mask_find_bytes_equal_to(__m512i V, __m512i N) {
    return _mm512_cmpeq_epi8_mask(V, N);
}
inline auto zmm_mask_find_bytes_less_than(__m512i V, __m512i N) {
    return _mm512_cmplt_epi8_mask(V, N);
}
inline auto zmm_mask_find_bytes_greater_than(__m512i V, __m512i N) {
    return _mm512_cmpgt_epi8_mask(V, N);
}
inline auto zmm_mask_find_bytes_not_in_range(__m512i V, __m512i A, __m512i B) {
    return _mm512_cmplt_epi8_mask(V, A) | _mm512_cmpgt_epi8_mask(V, B);
}
inline auto zmm_mask_find_bytes_in_range_exclusive(__m512i V, __m512i A, __m512i B) {
    return _mm512_cmpgt_epi8_mask(V, A) & _mm512_cmplt_epi8_mask(V, B);
}
inline auto zmm_mask_find_bytes_in_range(__m512i V, __m512i A, __m512i B) {
    return ~zmm_mask_find_bytes_not_in_range(V, A, B);
}
inline auto zmm_mask_find_zero_bytes(__m512i V) {
    return zmm_mask_find_bytes_equal_to(V, _mm512_setzero_si512());
}

/**
 * Generates masks indicating the comparison results between the bytes in vector V and a single comparison value N, or ranges defined by A and B.
 * These functions utilize SIMD instructions to perform byte-wise comparisons in parallel, significantly enhancing performance for operations
 * that require examining multiple bytes simultaneously. Each function returns a mask where each bit corresponds to a byte in V, with a set bit
 * indicating that the byte meets the specified comparison condition.
 *
 * - (x|y|z)mm_mask_find_bytes_equal_to: Generates a mask for bytes in V equal to N.
 * - (x|y|z)mm_mask_find_bytes_less_than: Generates a mask for bytes in V less than N.
 * - (x|y|z)mm_mask_find_bytes_greater_than: Generates a mask for bytes in V greater than N.
 * - (x|y|z)mm_mask_find_bytes_not_in_range: Generates a mask for bytes in V not in the inclusive range [A, B].
 * - (x|y|z)mm_mask_find_bytes_in_range_exclusive: Generates a mask for bytes in V within the exclusive range (A, B).
 * - (x|y|z)mm_mask_find_bytes_in_range: Generates a mask for bytes in V within the inclusive range [A, B].
 *
 * These functions are designed for efficient data processing and analysis, allowing for rapid identification of bytes that match specific criteria.
 *
 * @param V The source vector for the comparison.
 * @param N The comparison value for single-value comparisons.
 * @param A The lower bound value for range comparisons.
 * @param B The upper bound value for range comparisons.
 * @return A mask indicating the comparison results for each byte in V.
 */

 /**
  * Comparison functions for 128-bit SSE vectors with single byte values
  */
inline auto xmm_mask_find_bytes_equal_to(__m128i V, uint8_t N) {
    return xmm_mask_find_bytes_equal_to(V, _mm_set1_epi8(N));
}
inline auto xmm_mask_find_bytes_less_than(__m128i V, uint8_t N) {
    return xmm_mask_find_bytes_less_than(V, _mm_set1_epi8(N));
}
inline auto xmm_mask_find_bytes_greater_than(__m128i V, uint8_t N) {
    return xmm_mask_find_bytes_greater_than(V, _mm_set1_epi8(N));
}
inline auto xmm_mask_find_bytes_not_in_range(__m128i V, uint8_t A, uint8_t B) {
    return xmm_mask_find_bytes_not_in_range(V, _mm_set1_epi8(A), _mm_set1_epi8(B));
}
inline auto xmm_mask_find_bytes_in_range_exclusive(__m128i V, uint8_t A, uint8_t B) {
    return xmm_mask_find_bytes_in_range_exclusive(V, _mm_set1_epi8(A), _mm_set1_epi8(B));
}
inline auto xmm_mask_find_bytes_in_range(__m128i V, uint8_t A, uint8_t B) {
    return xmm_mask_find_bytes_in_range(V, _mm_set1_epi8(A), _mm_set1_epi8(B));
}

/**
 * Comparison functions for 256-bit AVX vectors with single byte values
 */
inline auto ymm_mask_find_bytes_equal_to(__m256i V, uint8_t N) {
    return ymm_mask_find_bytes_equal_to(V, _mm256_set1_epi8(N));
}
inline auto ymm_mask_find_bytes_less_than(__m256i V, uint8_t N) {
    return ymm_mask_find_bytes_less_than(V, _mm256_set1_epi8(N));
}
inline auto ymm_mask_find_bytes_greater_than(__m256i V, uint8_t N) {
    return ymm_mask_find_bytes_greater_than(V, _mm256_set1_epi8(N));
}
inline auto ymm_mask_find_bytes_not_in_range(__m256i V, uint8_t A, uint8_t B) {
    return ymm_mask_find_bytes_not_in_range(V, _mm256_set1_epi8(A), _mm256_set1_epi8(B));
}
inline auto ymm_mask_find_bytes_in_range_exclusive(__m256i V, uint8_t A, uint8_t B) {
    return ymm_mask_find_bytes_in_range_exclusive(V, _mm256_set1_epi8(A), _mm256_set1_epi8(B));
}
inline auto ymm_mask_find_bytes_in_range(__m256i V, uint8_t A, uint8_t B) {
    return ymm_mask_find_bytes_in_range(V, _mm256_set1_epi8(A), _mm256_set1_epi8(B));
}

/**
 * Comparison functions for 512-bit AVX-512 vectors with single byte values
 */
inline auto zmm_mask_find_bytes_equal_to(__m512i V, uint8_t N) {
    return zmm_mask_find_bytes_equal_to(V, _mm512_set1_epi8(N));
}
inline auto zmm_mask_find_bytes_less_than(__m512i V, uint8_t N) {
    return zmm_mask_find_bytes_less_than(V, _mm512_set1_epi8(N));
}
inline auto zmm_mask_find_bytes_greater_than(__m512i V, uint8_t N) {
    return zmm_mask_find_bytes_greater_than(V, _mm512_set1_epi8(N));
}
inline auto zmm_mask_find_bytes_not_in_range(__m512i V, uint8_t A, uint8_t B) {
    return zmm_mask_find_bytes_not_in_range(V, _mm512_set1_epi8(A), _mm512_set1_epi8(B));
}
inline auto zmm_mask_find_bytes_in_range_exclusive(__m512i V, uint8_t A, uint8_t B) {
    return zmm_mask_find_bytes_in_range_exclusive(V, _mm512_set1_epi8(A), _mm512_set1_epi8(B));
}
inline auto zmm_mask_find_bytes_in_range(__m512i V, uint8_t A, uint8_t B) {
    return zmm_mask_find_bytes_in_range(V, _mm512_set1_epi8(A), _mm512_set1_epi8(B));
}

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

 /**
  * Comparison functions for 128-bit SSE vectors
  */
inline auto xmm_find_first_byte_equal_to(__m128i V, __m128i N) {
    return xmm_get_first_match(xmm_mask_find_bytes_equal_to(V, N));
}
inline auto xmm_find_first_byte_less_than(__m128i V, __m128i N) {
    return xmm_get_first_match(xmm_mask_find_bytes_less_than(V, N));
}
inline auto xmm_find_first_byte_greater_than(__m128i V, __m128i N) {
    return xmm_get_first_match(xmm_mask_find_bytes_greater_than(V, N));
}
inline auto xmm_find_first_byte_not_in_range(__m128i V, __m128i A, __m128i B) {
    return xmm_get_first_match(xmm_mask_find_bytes_not_in_range(V, A, B));
}
inline auto xmm_find_first_byte_in_range_exclusive(__m128i V, __m128i A, __m128i B) {
    return xmm_get_first_match(xmm_mask_find_bytes_in_range_exclusive(V, A, B));
}
inline auto xmm_find_first_byte_in_range(__m128i V, __m128i A, __m128i B) {
    return xmm_get_first_match(xmm_mask_find_bytes_in_range(V, A, B));
}

/**
 * Comparison functions for 256-bit AVX vectors
 */
inline auto ymm_find_first_byte_equal_to(__m256i V, __m256i N) {
    return ymm_get_first_match(ymm_mask_find_bytes_equal_to(V, N));
}
inline auto ymm_find_first_byte_less_than(__m256i V, __m256i N) {
    return ymm_get_first_match(ymm_mask_find_bytes_less_than(V, N));
}
inline auto ymm_find_first_byte_greater_than(__m256i V, __m256i N) {
    return ymm_get_first_match(ymm_mask_find_bytes_greater_than(V, N));
}
inline auto ymm_find_first_byte_not_in_range(__m256i V, __m256i A, __m256i B) {
    return ymm_get_first_match(ymm_mask_find_bytes_not_in_range(V, A, B));
}
inline auto ymm_find_first_byte_in_range_exclusive(__m256i V, __m256i A, __m256i B) {
    return ymm_get_first_match(ymm_mask_find_bytes_in_range_exclusive(V, A, B));
}
inline auto ymm_find_first_byte_in_range(__m256i V, __m256i A, __m256i B) {
    return ymm_get_first_match(ymm_mask_find_bytes_in_range(V, A, B));
}

/**
 * Comparison functions for 512-bit AVX-512 vectors
 */
inline auto zmm_find_first_byte_equal_to(__m512i V, __m512i N) {
    return zmm_get_first_match(zmm_mask_find_bytes_equal_to(V, N));
}
inline auto zmm_find_first_byte_less_than(__m512i V, __m512i N) {
    return zmm_get_first_match(zmm_mask_find_bytes_less_than(V, N));
}
inline auto zmm_find_first_byte_greater_than(__m512i V, __m512i N) {
    return zmm_get_first_match(zmm_mask_find_bytes_greater_than(V, N));
}
inline auto zmm_find_first_byte_not_in_range(__m512i V, __m512i A, __m512i B) {
    return zmm_get_first_match(zmm_mask_find_bytes_not_in_range(V, A, B));
}
inline auto zmm_find_first_byte_in_range_exclusive(__m512i V, __m512i A, __m512i B) {
    return zmm_get_first_match(zmm_mask_find_bytes_in_range_exclusive(V, A, B));
}
inline auto zmm_find_first_byte_in_range(__m512i V, __m512i A, __m512i B) {
    return zmm_get_first_match(zmm_mask_find_bytes_in_range(V, A, B));
}

/**
 * Identifies the index of the first byte in the source vector V that meets specific comparison criteria with respect to a given value or range.
 * These functions extend the capabilities of SIMD instructions for parallel processing to efficiently determine the position of bytes that match
 * the specified conditions, significantly enhancing performance for operations requiring the examination of multiple bytes.
 *
 * The functions and their criteria are as follows:
 * - (x|y|z)mm_find_first_byte_equal_to: Returns the index of the first byte in V that is equal to the given value N.
 * - (x|y|z)mm_find_first_byte_less_than: Returns the index of the first byte in V that is less than the given value N.
 * - (x|y|z)mm_find_first_byte_greater_than: Returns the index of the first byte in V that is greater than the given value N.
 * - (x|y|z)mm_find_first_byte_not_in_range: Returns the index of the first byte in V that is not within the inclusive range defined by [A, B].
 * - (x|y|z)mm_find_first_byte_in_range_exclusive: Returns the index of the first byte in V that is within the inclusive range defined by (A, B).
 * - (x|y|z)mm_find_first_byte_in_range: Returns the index of the first byte in V that is within the inclusive range defined by [A, B].
 * - (x|y|z)mm_find_first_zero_byte: Returns the index of the first zero byte in V.
 *
 * These functions utilize a mask generated by SIMD comparison operations to identify the first byte that meets the condition. The index of this byte
 * is then calculated and returned. If no byte meets the condition, mm_npos is returned, indicating that no matching byte was found within the 128-bit
 * vector limit.
 *
 * @param V The source vector for the comparison.
 * @param N The comparison value for functions that compare against a single value.
 * @param A The lower bound value for range comparisons.
 * @param B The upper bound value for range comparisons.
 * @return The index of the first byte meeting the comparison criterion, or mm_npos if no such byte is found.
 */

 /**
  * Comparison functions for 128-bit SSE vectors with single byte values
  */
inline auto xmm_find_first_byte_equal_to(__m128i V, uint8_t N) {
    return xmm_find_first_byte_equal_to(V, _mm_set1_epi8(N));
}
inline auto xmm_find_first_byte_less_than(__m128i V, uint8_t N) {
    return xmm_find_first_byte_less_than(V, _mm_set1_epi8(N));
}
inline auto xmm_find_first_byte_greater_than(__m128i V, uint8_t N) {
    return xmm_find_first_byte_greater_than(V, _mm_set1_epi8(N));
}
inline auto xmm_find_first_byte_not_in_range(__m128i V, uint8_t A, uint8_t B) {
    return xmm_find_first_byte_not_in_range(V, _mm_set1_epi8(A), _mm_set1_epi8(B));
}
inline auto xmm_find_first_byte_in_range_exclusive(__m128i V, uint8_t A, uint8_t B) {
    return xmm_find_first_byte_in_range_exclusive(V, _mm_set1_epi8(A), _mm_set1_epi8(B));
}
inline auto xmm_find_first_byte_in_range(__m128i V, uint8_t A, uint8_t B) {
    return xmm_find_first_byte_in_range(V, _mm_set1_epi8(A), _mm_set1_epi8(B));
}
inline auto xmm_find_first_zero_byte(__m128i V) {
    return xmm_get_first_match(xmm_mask_find_zero_bytes(V));
}

/**
 * Comparison functions for 256-bit AVX vectors with single byte values
 */
inline auto ymm_find_first_byte_equal_to(__m256i V, uint8_t N) {
    return ymm_find_first_byte_equal_to(V, _mm256_set1_epi8(N));
}
inline auto ymm_find_first_byte_less_than(__m256i V, uint8_t N) {
    return ymm_find_first_byte_less_than(V, _mm256_set1_epi8(N));
}
inline auto ymm_find_first_byte_greater_than(__m256i V, uint8_t N) {
    return ymm_find_first_byte_greater_than(V, _mm256_set1_epi8(N));
}
inline auto ymm_find_first_byte_not_in_range(__m256i V, uint8_t A, uint8_t B) {
    return ymm_find_first_byte_not_in_range(V, _mm256_set1_epi8(A), _mm256_set1_epi8(B));
}
inline auto ymm_find_first_byte_in_range_exclusive(__m256i V, uint8_t A, uint8_t B) {
    return ymm_find_first_byte_in_range_exclusive(V, _mm256_set1_epi8(A), _mm256_set1_epi8(B));
}
inline auto ymm_find_first_byte_in_range(__m256i V, uint8_t A, uint8_t B) {
    return ymm_find_first_byte_in_range(V, _mm256_set1_epi8(A), _mm256_set1_epi8(B));
}
inline auto ymm_find_first_zero_byte(__m256i V) {
    return ymm_get_first_match(ymm_mask_find_zero_bytes(V));
}

/**
 * Comparison functions for 512-bit AVX-512 vectors with single byte values
 */
inline auto zmm_find_first_byte_equal_to(__m512i V, uint8_t N) {
    return zmm_find_first_byte_equal_to(V, _mm512_set1_epi8(N));
}
inline auto zmm_find_first_byte_less_than(__m512i V, uint8_t N) {
    return zmm_find_first_byte_less_than(V, _mm512_set1_epi8(N));
}
inline auto zmm_find_first_byte_greater_than(__m512i V, uint8_t N) {
    return zmm_find_first_byte_greater_than(V, _mm512_set1_epi8(N));
}
inline auto zmm_find_first_byte_not_in_range(__m512i V, uint8_t A, uint8_t B) {
    return zmm_find_first_byte_not_in_range(V, _mm512_set1_epi8(A), _mm512_set1_epi8(B));
}
inline auto zmm_find_first_byte_in_range_exclusive(__m512i V, uint8_t A, uint8_t B) {
    return zmm_find_first_byte_in_range_exclusive(V, _mm512_set1_epi8(A), _mm512_set1_epi8(B));
}
inline auto zmm_find_first_byte_in_range(__m512i V, uint8_t A, uint8_t B) {
    return zmm_find_first_byte_in_range(V, _mm512_set1_epi8(A), _mm512_set1_epi8(B));
}
inline auto zmm_find_first_zero_byte(__m512i V) {
    return zmm_get_first_match(zmm_mask_find_zero_bytes(V));
}

/**
 * Searches for the first occurrence of a specified character within a given string view, leveraging SIMD instructions
 * for efficient parallel processing. This function and its overloads are designed to quickly locate the character by
 * examining multiple elements of the string simultaneously, enhancing search performance, especially in large strings.
 *
 * The function provides three overloads to accommodate different search scenarios:
 * 1. A basic overload that searches the entire string view from the beginning for the specified character.
 * 2. An overload that allows specifying a starting offset, enabling the search to begin from a particular position
 *    within the string view. This is useful for skipping a known prefix or previously searched segment.
 * 3. An overload that accepts both a starting offset and a count, restricting the search to a specific subrange
 *    of the string view. This is particularly useful for limiting the search to a known relevant section.
 *
 * @param Text The string view to search within.
 * @param C The character to find within the string view.
 * @param Ofs (Optional) The starting offset in the string view from where the search should commence. This parameter
 *            is only applicable to the second and third overloads.
 * @param Count (Optional) The number of characters to include in the search, starting from Ofs. This parameter is only
 *              applicable to the third overload.
 * @return A pointer to the first occurrence of the character in the string view, or nullptr if the character is not found.
 *         The search respects the bounds defined by Ofs and Count, when provided.
 */

 /**
  * Parallel find functions for 128-bit SSE vectors
  */
inline auto xmm_parallel_find(std::string_view Text, char C) -> const char* {
    auto Char = _mm_set1_epi8(C);
    auto Data = reinterpret_cast<const __m128i*>(Text.data());
    auto Stop = Data + (Text.size() + 15) / 16; int Pos;
    while (!(Pos = xmm_mask_find_bytes_equal_to(_mm_loadu_si128(Data), Char)))
        if (++Data == Stop)
            break;
    if (!Pos)
        return nullptr;
    return reinterpret_cast<const char*>(Data) + xmm_get_first_match(Pos);
}
inline auto xmm_parallel_find(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() > Ofs);
    return xmm_parallel_find(Text.substr(Ofs), C);
}
inline auto xmm_parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return xmm_parallel_find(Text.substr(Ofs, Count), C);
}

/**
 * Parallel find functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_find(std::string_view Text, char C) -> const char* {
    auto Char = _mm256_set1_epi8(C);
    auto Data = reinterpret_cast<const __m256i*>(Text.data());
    auto Stop = Data + (Text.size() + 31) / 32; int Pos;
    while (!(Pos = ymm_mask_find_bytes_equal_to(_mm256_loadu_si256(Data), Char)))
        if (++Data == Stop)
            break;
    if (!Pos)
        return nullptr;
    return reinterpret_cast<const char*>(Data) + ymm_get_first_match(Pos);
}
inline auto ymm_parallel_find(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() > Ofs);
    return ymm_parallel_find(Text.substr(Ofs), C);
}
inline auto ymm_parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return ymm_parallel_find(Text.substr(Ofs, Count), C);
}

/**
 * Parallel find functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find(std::string_view Text, char C) -> const char* {
    auto Char = _mm512_set1_epi8(C);
    auto Data = reinterpret_cast<const __m512i*>(Text.data());
    auto Stop = Data + (Text.size() + 63) / 64; __mmask64 Pos;
    while (!(Pos = zmm_mask_find_bytes_equal_to(_mm512_loadu_si512(Data), Char)))
        if (++Data == Stop)
            break;
    if (!Pos)
        return nullptr;
    return reinterpret_cast<const char*>(Data) + zmm_get_first_match(Pos);
}
inline auto zmm_parallel_find(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() > Ofs);
    return zmm_parallel_find(Text.substr(Ofs), C);
}
inline auto zmm_parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return zmm_parallel_find(Text.substr(Ofs, Count), C);
}

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
inline auto xmm_parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = xmm_parallel_find(Text, C);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto xmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() > Ofs);
    return xmm_parallel_find_pos(Text.substr(Ofs), C) + Ofs;
}
inline auto xmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return xmm_parallel_find_pos(Text.substr(Ofs, Count), C) + Ofs;
}

/**
 * Parallel find position functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = ymm_parallel_find(Text, C);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto ymm_parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() > Ofs);
    return ymm_parallel_find_pos(Text.substr(Ofs), C) + Ofs;
}
inline auto ymm_parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return ymm_parallel_find_pos(Text.substr(Ofs, Count), C) + Ofs;
}

/**
 * Parallel find position functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = zmm_parallel_find(Text, C);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto zmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() > Ofs);
    return zmm_parallel_find_pos(Text.substr(Ofs), C) + Ofs;
}
inline auto zmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return zmm_parallel_find_pos(Text.substr(Ofs, Count), C) + Ofs;
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
  // XMM - Quotes (hybride)
inline auto xmm_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    constexpr size_t align = 16u;
    constexpr size_t amask = align - 1u;
    const char* base = Text.data();
    const char* begin = base + Ofs;
    assert(Text.size() >= Ofs + Count);

    // Fast-path: sous 16B -> traitement scalaire
    if (Count < align)
        return static_cast<const char*>(std::memchr(begin, '"', Count));

    // Prologue: traiter jusqu’à la frontière 16B, via XMM non aligné
    if (size_t shift = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(begin) & amask)) {
        int mask = xmm_mask_find_bytes_equal_to(_mm_loadu_si128(reinterpret_cast<const __m128i*>(begin)), xmm_Quote);
        if (mask) return begin + static_cast<size_t>(_tzcnt_u32(mask));
        shift = align - shift;
        begin += shift; Count -= shift;
    }

    // Corps: blocs alignés 16B
    while (Count >= align) {
        int mask = xmm_mask_find_bytes_equal_to(_mm_load_si128(reinterpret_cast<const __m128i*>(begin)), xmm_Quote);
        if (mask) return begin + static_cast<size_t>(_tzcnt_u32(mask));
        begin += align; Count -= align;
    }

    //Épilogue: reliquat < 16B -> traitement scalaire
    return static_cast<const char*>(std::memchr(begin, '"', Count));
}
inline auto xmm_parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return xmm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
}
inline auto xmm_parallel_find_quote(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return xmm_parallel_find_quote(Text, 0, Text.size());
}

/**
 * Parallel find functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    constexpr size_t align = 32u;
    constexpr size_t amask = align - 1;
    const char* base = Text.data();
    const char* begin = base + Ofs;

    assert(Text.size() >= Ofs + Count);

    // Fast-path: sous 32B -> délégation directe à XMM (hybride)
    if (Count < 32u) {
        return static_cast<const char*>(std::memchr(begin, '"', Count));
//        return xmm_parallel_find_quote(Text, Ofs, Count);
    }

    // Prologue: traiter jusqu’à la frontière 64B, via XMM
    if (size_t shift = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(begin) & amask)) {
        const size_t head = std::min(Count, align - shift);
        if (const char* p = xmm_parallel_find_quote(Text, Ofs, head))
            return p;
        begin += head; Ofs += head; Count -= head;
    }

    // Corps: blocs alignés 32B
    while (Count >= align) {
        int mask = ymm_mask_find_bytes_equal_to(_mm256_load_si256(reinterpret_cast<const __m256i*>(begin)), ymm_Quote);
        if (mask) return begin + static_cast<size_t>(_tzcnt_u32(mask));
        begin += align; Ofs += align; Count -= align;
    }

    // Épilogue: reliquat < 32B -> déléguer à XMM
    return static_cast<const char*>(std::memchr(begin, '"', Count));
//    return xmm_parallel_find_quote(Text, Ofs, Count);
}
inline auto ymm_parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return ymm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
}
inline auto ymm_parallel_find_quote(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return ymm_parallel_find_quote(Text, 0, Text.size());
}

//inline auto ymm_parallel_find_quote(std::string_view Text) -> const char* {
//    auto Data = reinterpret_cast<const __m256i*>(Text.data());
//    auto Stop = Data + (Text.size() + 31) / 32; int Pos;
//    while (!(Pos = ymm_mask_find_bytes_equal_to(_mm256_loadu_si256(Data), ymm_Quote)))
//        if (++Data == Stop)
//            break;
//    if (!Pos)
//        return nullptr;
//    return reinterpret_cast<const char*>(Data) + ymm_get_first_match(Pos);
//}
//inline auto ymm_parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
//    assert(Text.size() > Ofs);
//    return ymm_parallel_find_quote(Text.substr(Ofs));
//}
//inline auto ymm_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
//    assert(Text.size() > Ofs + Count);
//    return ymm_parallel_find_quote(Text.substr(Ofs, Count));
//}

/**
 * Parallel find functions for 512-bit AVX-512 vectors
 */
 // ZMM - Quotes (hybride)
inline auto zmm_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    constexpr size_t align = 64u;
    constexpr size_t amask = align - 1u;
    const char* base = Text.data();
    const char* begin = base + Ofs;
    assert(Text.size() >= Ofs + Count);

    //  Fast-path: sous 64B -> délégation directe à YMM (hybride)
    if (Count < align) {
        return ymm_parallel_find_quote(Text, Ofs, Count);
    }

    // Prologue: traiter jusqu’à la frontière 64B, via YMM
    if (size_t shift = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(begin) & amask)) {
        const size_t head = std::min(Count, align - shift);
        if (const char* p = ymm_parallel_find_quote(Text, Ofs, head))
            return p;
        begin += head; Ofs += head; Count -= head;
    }

    // Corps: blocs alignés 64B
    while (Count >= align) {
        __m512i v = _mm512_load_si512(reinterpret_cast<const void*>(begin));
        __mmask64 k = zmm_mask_find_bytes_equal_to(v, zmm_Quote);
        if (k) return begin + static_cast<size_t>(zmm_get_first_match(k));
        begin += align; Ofs += align; Count -= align;
    }

    // Épilogue: reliquat < 64B -> déléguer à YMM
    return ymm_parallel_find_quote(Text, Ofs, Count);
}
inline auto zmm_parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() > Ofs);
    return zmm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
}
inline auto zmm_parallel_find_quote(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return zmm_parallel_find_quote(Text, 0, Text.size());
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
inline auto xmm_parallel_find_quote_pos(std::string_view Text) -> size_t {
    auto Found = xmm_parallel_find_quote(Text);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto xmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() > Ofs);
    return xmm_parallel_find_quote_pos(Text.substr(Ofs)) + Ofs;
}
inline auto xmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return xmm_parallel_find_quote_pos(Text.substr(Ofs, Count)) + Ofs;
}

/**
 * Parallel find position functions for 256-bit AVX vectors
 */
//inline auto ymm_parallel_find_quote_pos(std::string_view Text) -> size_t {
//    auto Found = ymm_parallel_find_quote(Text);
//    return Found ? Found - Text.data() : Text.npos;
//}
//inline auto ymm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
//    assert(Text.size() > Ofs);
//    return ymm_parallel_find_quote_pos(Text.substr(Ofs)) + Ofs;
//}
//inline auto ymm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
//    assert(Text.size() > Ofs + Count);
//    return ymm_parallel_find_quote_pos(Text.substr(Ofs, Count)) + Ofs;
//}

inline auto ymm_parallel_find_quote_pos(std::string_view Text) -> size_t {
    if (const char* p = ymm_parallel_find_quote(Text))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto ymm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) -> size_t {
    assert(Text.size() >= Ofs);
    if (const char* p = ymm_parallel_find_quote(Text, Ofs, Text.size() - Ofs))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto ymm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
    assert(Text.size() >= Ofs + Count);
    if (const char* p = ymm_parallel_find_quote(Text, Ofs, Count))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}

/**
 * Parallel find position functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find_quote_pos(std::string_view Text) -> size_t {
    auto Found = zmm_parallel_find_quote(Text);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto zmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() > Ofs);
    return zmm_parallel_find_quote_pos(Text.substr(Ofs)) + Ofs;
}
inline auto zmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs + Count);
    return zmm_parallel_find_quote_pos(Text.substr(Ofs, Count)) + Ofs;
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
inline auto xmm_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    constexpr size_t align = 16u;
    constexpr size_t amask = align - 1u;
    const char* base = Text.data();
    const char* begin = base + Ofs;
    assert(Text.size() >= Ofs + Count);

    // Fast-path: sous 16B -> traitement scalaire
    if (Count < align) {
        while (Count--) {
            const auto c = static_cast<unsigned char>(*begin);
            if (c < '0' || c > '9') return begin;
            ++begin;
        }
        return nullptr;
    }

    // Prologue: traiter jusqu’à la frontière 16B, via XMM non aligné
    if (size_t shift = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(begin) & amask)) {
        int mask = xmm_mask_find_bytes_not_in_range(_mm_loadu_si128(reinterpret_cast<const __m128i*>(begin)), xmm_Zero, xmm_Nine);
        if (mask) return begin + static_cast<size_t>(_tzcnt_u32(mask));
        shift = align - shift;
        begin += shift; Count -= shift;
    }

    // Corps: blocs alignés 16B
    while (Count >= align) {
        int mask = xmm_mask_find_bytes_not_in_range(_mm_load_si128(reinterpret_cast<const __m128i*>(begin)), xmm_Zero, xmm_Nine);
        if (mask) return begin + static_cast<size_t>(_tzcnt_u32(mask));
        begin += align; Count -= align;
    }

    // Épilogue: reliquat < 16B -> traitement scalaire
    while (Count--) {
        const auto c = static_cast<unsigned char>(*begin);
        if (c < '0' || c > '9') return begin;
        ++begin;
    }
    return nullptr;
}
inline auto xmm_parallel_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return xmm_parallel_digits(Text, Ofs, Text.size() - Ofs);
}
inline auto xmm_parallel_digits(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return xmm_parallel_digits(Text, 0, Text.size());
}

/**
 * Parallel comparison functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    constexpr size_t align = 32u;
    constexpr size_t amask = align - 1;
    const char* base = Text.data();
    const char* begin = base + Ofs;
    assert(Text.size() >= Ofs + Count);

    // Fast-path: sous 32B -> délégation directe à XMM (hybride)
    if (Count < align) {
        while (Count--) {
            const auto c = static_cast<unsigned char>(*begin);
            if (c < '0' || c > '9') return begin;
            ++begin;
        }
        return nullptr;
//        return xmm_parallel_digits(Text, Ofs, Count);
    }

    // Prologue: traiter jusqu’à la frontière 32B, via XMM
    if (size_t shift = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(begin) & amask)) {
        const size_t head = std::min(Count, align - shift);
        if (const char* p = xmm_parallel_digits(Text, Ofs, head))
            return p;
        begin += head; Ofs += head; Count -= head;
    }

    // Corps: blocs alignés 32B
    while (Count >= align) {
        int mask = ymm_mask_find_bytes_not_in_range(_mm256_load_si256(reinterpret_cast<const __m256i*>(begin)), ymm_Zero, ymm_Nine);
        if (mask) return begin + static_cast<size_t>(_tzcnt_u32(mask));
        begin += align; Ofs += align; Count -= align;
    }

    // Épilogue: reliquat < 32B -> déléguer à XMM
    while (Count--) {
        const auto c = static_cast<unsigned char>(*begin);
        if (c < '0' || c > '9') return begin;
        ++begin;
    }
    return nullptr;
//    return xmm_parallel_digits(Text, Ofs, Count);
}

inline auto ymm_parallel_digits(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return ymm_parallel_digits(Text, 0, Text.size());
}
inline auto ymm_parallel_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return ymm_parallel_digits(Text, Ofs, Text.size() - Ofs);
}

//inline auto ymm_parallel_digits(std::string_view Text) -> const char* {
//    auto Data = reinterpret_cast<const __m256i*>(Text.data());
//    auto Stop = Data + (Text.size() + 31) / 32; int Pos;
//    while (!(Pos = ymm_mask_find_bytes_not_in_range(_mm256_loadu_si256(Data), ymm_Zero, ymm_Nine)))
//        if (++Data == Stop)
//            break;
//    if (!Pos)
//        return nullptr;
//    return reinterpret_cast<const char*>(Data) + ymm_get_first_match(Pos);
//}
//inline auto ymm_parallel_digits(std::string_view Text, size_t Ofs) {
//    assert(Text.size() > Ofs);
//    return ymm_parallel_digits(Text.substr(Ofs));
//}
//inline auto ymm_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) {
//    assert(Text.size() > Ofs);
//    return ymm_parallel_digits(Text.substr(Ofs, Count));
//}

/**
 * Parallel comparison functions for 512-bit AVX-512 vectors
 */
 // ZMM - Digits (hybride)
inline auto zmm_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    constexpr size_t align = 64u;
    constexpr size_t amask = align - 1u;
    const char* base = Text.data();
    const char* begin = base + Ofs;
    assert(Text.size() >= Ofs + Count);

    // Fast-path: sous 64B -> délégation directe à YMM (hybride)
    if (Count < align) {
        return ymm_parallel_digits(Text, Ofs, Count);
    }

    // Prologue: traiter jusqu’à la frontière 64B, via YMM
    if (size_t shift = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(begin) & amask)) {
        const size_t head = std::min(Count, align - shift);
        if (const char* p = ymm_parallel_digits(Text, Ofs, head))
            return p;
        begin += head; Ofs += head; Count -= head;
    }

    // Corps: blocs alignés 64B
    while (Count >= align) {
        __m512i v = _mm512_load_si512(reinterpret_cast<const void*>(begin));
        __mmask64 k = zmm_mask_find_bytes_not_in_range(v, zmm_Zero, zmm_Nine);
        if (k) return begin + static_cast<size_t>(zmm_get_first_match(k));
        begin += align; Ofs += align; Count -= align;
    }

    // Épilogue: reliquat < 64B -> déléguer à YMM
    return ymm_parallel_digits(Text, Ofs, Count);
}
inline auto zmm_parallel_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() > Ofs);
    return zmm_parallel_digits(Text, Ofs, Text.size() - Ofs);
}
inline auto zmm_parallel_digits(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return zmm_parallel_digits(Text, 0, Text.size());
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
inline auto xmm_parallel_digits_pos(std::string_view Text) -> size_t {
    auto Found = xmm_parallel_digits(Text);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto xmm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() > Ofs);
    auto Found{ xmm_parallel_digits(Text.substr(Ofs)) };
    return Found ? Found - Text.data() + Ofs : Text.npos;
}
inline auto xmm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs);
    auto Found{ xmm_parallel_digits(Text.substr(Ofs, Count)) };
    return Found ? Found - Text.data() + Ofs : Text.npos;
}

/**
 * Parallel comparison functions for 256-bit AVX vectors
 */
//inline auto ymm_parallel_digits_pos(std::string_view Text) -> size_t {
//    auto Found = ymm_parallel_digits(Text);
//    return Found ? Found - Text.data() : Text.npos;
//}
//inline auto ymm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
//    assert(Text.size() > Ofs);
//    auto Found{ ymm_parallel_digits(Text.substr(Ofs)) };
//    return Found ? Found - Text.data() + Ofs : Text.npos;
//}
//inline auto ymm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
//    assert(Text.size() > Ofs);
//    auto Found{ ymm_parallel_digits(Text.substr(Ofs, Count)) };
//    return Found ? Found - Text.data() + Ofs : Text.npos;
//}
inline auto ymm_parallel_digits_pos(std::string_view Text) -> size_t {
    if (const char* p = ymm_parallel_digits(Text, 0, Text.size()))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto ymm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    if (const char* p = ymm_parallel_digits(Text, Ofs, Text.size() - Ofs))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto ymm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    if (const char* p = ymm_parallel_digits(Text, Ofs, Count))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}

/**
 * Parallel comparison functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_digits_pos(std::string_view Text) -> size_t {
    auto Found = zmm_parallel_digits(Text);
    return Found ? Found - Text.data() : Text.npos;
}
inline auto zmm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() > Ofs);
    auto Found{ zmm_parallel_digits(Text.substr(Ofs)) };
    return Found ? Found - Text.data() + Ofs : Text.npos;
}
inline auto zmm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() > Ofs);
    auto Found{ zmm_parallel_digits(Text.substr(Ofs, Count)) };
    return Found ? Found - Text.data() + Ofs : Text.npos;
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
inline auto xmm_parallel_compare(std::string_view Left, std::string_view Right) {
    auto StartLeft = reinterpret_cast<const __m128i*>(Left.data());
    auto StartRight = reinterpret_cast<const __m128i*>(Right.data());
    auto Stop = StartLeft + (Left.size() < Right.size() ? (Left.size() + 15) / 16 : (Right.size() + 15) / 16);
    auto DataLeft = StartLeft;
    auto DataRight = StartRight;
    int Pos;
    while ((Pos = xmm_mask_find_bytes_equal_to(_mm_loadu_si128(DataLeft), _mm_loadu_si128(DataRight))) == 0xffff)
        if ((++DataRight, ++DataLeft) == Stop)
            break;
    if (Pos != 0xffff)
    {
        size_t CharPos = static_cast<size_t>((DataLeft - StartLeft) * 16 + xmm_get_first_match(Pos ^ 0xffff));
        auto CharLeft = CharPos < Left.size() ? Left[CharPos] : 0;
        auto CharRight = CharPos < Right.size() ? Right[CharPos] : 0;
        if (CharLeft < CharRight)
            return -1;
        else
            return 1;
    }
    return 0;
}
inline auto xmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft) {
    assert(Left.size() > OfsLeft);
    return xmm_parallel_compare(Left.substr(OfsLeft), Right);
}
inline auto xmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft) {
    assert(Left.size() > OfsLeft);
    return xmm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right);
}
inline auto xmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight) {
    assert(Left.size() > OfsLeft);
    assert(Right.size() > OfsRight);
    return xmm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight));
}
inline auto xmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight, size_t CountRight) {
    assert(Left.size() > OfsLeft);
    assert(Right.size() > OfsRight);
    return xmm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight, CountRight));
}

/**
 * Parallel comparison functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_compare(std::string_view Left, std::string_view Right) {
    auto StartLeft = reinterpret_cast<const __m256i*>(Left.data());
    auto StartRight = reinterpret_cast<const __m256i*>(Right.data());
    auto Stop = StartLeft + (Left.size() < Right.size() ? (Left.size() + 31) / 32 : (Right.size() + 31) / 32);
    auto DataLeft = StartLeft;
    auto DataRight = StartRight;
    int Pos;
    while ((Pos = ymm_mask_find_bytes_equal_to(_mm256_loadu_si256(DataLeft), _mm256_loadu_si256(DataRight))) == 0xffffffff)
        if ((++DataRight, ++DataLeft) == Stop)
            break;
    if (Pos != 0xffffffff)
    {
        size_t CharPos = static_cast<size_t>((DataLeft - StartLeft) * 32 + ymm_get_first_match(Pos ^ 0xffffffff));
        auto CharLeft = CharPos < Left.size() ? Left[CharPos] : 0;
        auto CharRight = CharPos < Right.size() ? Right[CharPos] : 0;
        if (CharLeft < CharRight)
            return -1;
        else
            return 1;
    }
    return 0;
}
inline auto ymm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft) {
    assert(Left.size() > OfsLeft);
    return ymm_parallel_compare(Left.substr(OfsLeft), Right);
}
inline auto ymm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft) {
    assert(Left.size() > OfsLeft);
    return ymm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right);
}
inline auto ymm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight) {
    assert(Left.size() > OfsLeft);
    assert(Right.size() > OfsRight);
    return ymm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight));
}
inline auto ymm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight, size_t CountRight) {
    assert(Left.size() > OfsLeft);
    assert(Right.size() > OfsRight);
    return ymm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight, CountRight));
}

/**
 * Parallel comparison functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_compare(std::string_view Left, std::string_view Right) {
    auto StartLeft = reinterpret_cast<const __m512i*>(Left.data());
    auto StartRight = reinterpret_cast<const __m512i*>(Right.data());
    auto Stop = StartLeft + (Left.size() < Right.size() ? (Left.size() + 63) / 64 : (Right.size() + 63) / 64);
    auto DataLeft = StartLeft;
    auto DataRight = StartRight;
    __mmask64 Pos;
    while ((Pos = zmm_mask_find_bytes_equal_to(_mm512_loadu_si512(DataLeft), _mm512_loadu_si512(DataRight))) == 0xffffffffffffffff)
        if ((++DataRight, ++DataLeft) == Stop)
            break;
    if (Pos != 0xffffffffffffffff)
    {
        size_t CharPos = static_cast<size_t>((DataLeft - StartLeft) * 64 + zmm_get_first_match(Pos ^ 0xffffffffffffffff));
        auto CharLeft = CharPos < Left.size() ? Left[CharPos] : 0;
        auto CharRight = CharPos < Right.size() ? Right[CharPos] : 0;
        if (CharLeft < CharRight)
            return -1;
        else
            return 1;
    }
    return 0;
}
inline auto zmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft) {
    assert(Left.size() > OfsLeft);
    return zmm_parallel_compare(Left.substr(OfsLeft), Right);
}
inline auto zmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft) {
    assert(Left.size() > OfsLeft);
    return zmm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right);
}
inline auto zmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight) {
    assert(Left.size() > OfsLeft);
    assert(Right.size() > OfsRight);
    return zmm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight));
}
inline auto zmm_parallel_compare(std::string_view Left, std::string_view Right, size_t OfsLeft, size_t CountLeft, size_t OfsRight, size_t CountRight) {
    assert(Left.size() > OfsLeft);
    assert(Right.size() > OfsRight);
    return zmm_parallel_compare(Left.substr(OfsLeft, CountLeft), Right.substr(OfsRight, CountRight));
}

} // namespace simd

#endif // SIMD_COMPARISONS_H