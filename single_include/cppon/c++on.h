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

// -----------------------------------------------------------------------------
// C++ON Uber Header
// -----------------------------------------------------------------------------
// This is the single-include header for the C++ON library.
//
// Including this file gives you access to all features of C++ON:
//   - High-performance JSON parsing and generation
//   - SIMD-accelerated processing (SSE, AVX2, AVX-512)
//   - Path-based navigation (absolute/relative)
//   - Cross-references and binary blobs
//   - Extended C++ type support
//   - Thread-safety and more
//
// Usage:
//   #include <cppon/c++on.h>
//
// This header includes all necessary components in the correct order.
// For modular development, you can include only the headers you need from the
// main include/ directory. For deployment or end-user integration, prefer this
// single header for simplicity and maximum compatibility.
// -----------------------------------------------------------------------------

/*
 * @brief Configuration macros
 *
 * #define CPPON_ENABLE_SIMD              - Opt-in: enable SIMD paths (SSE/AVX2/AVX512 if available)
 * #define CPPON_TRUSTED_INPUT            - Enable fast path for skip_spaces on trusted input
 * #define CPPON_ENABLE_STD_GET_INJECTION - Enable std::get injection into ch5 namespace
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
// SECTION: Standard Library Includes
// -----------------------------------------------------------------------------
// All required C++ standard library headers are consolidated here for clarity
// and to ensure consistent inclusion order throughout the library.
// -----------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#ifdef NDEBUG
#define CPPON_ASSERT(cond) ((void)0)
#else
#define CPPON_ASSERT(cond) assert(cond)
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
/*
 * Processor Features Info - C++ library
 * https://github.com/methanium/cppon
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef PROCESSOR_FEATURES_INFO_H
#define PROCESSOR_FEATURES_INFO_H

#if defined(_MSC_VER)
    #ifndef WORD
        #define NOMINMAX
        #include <Windows.h>
    #endif
    #if defined (__clang__)
        #include <x86intrin.h>
    #else
        #include <intrin.h>
    #endif
    #ifndef ulong
        typedef unsigned long ulong;
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #include <stdint.h>
    #include <string.h>
    #include <cpuid.h>
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <pthread.h>
    #include <x86intrin.h>

    #ifndef WORD
        typedef uint16_t WORD;
    #endif
    #ifndef ulong
        typedef unsigned long ulong;
    #endif
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

#include <cassert>
#include <string_view>

namespace platform {

/**
 * @brief Detects and provides information about processor features
 *
 * This class encapsulates detection of SIMD capabilities (SSE, AVX, AVX-512) and other
 * processor features that can be used to optimize performance-critical code paths.
 * Supports runtime detection of CPU instructions across different platforms.
 */
class processor_features_info
{
private:
	union cpu_probe_t
		{
		mutable int data[ 4 ];
		struct {
			unsigned rEAX;
			unsigned rEBX;
			unsigned rECX;
			unsigned rEDX;
			};
		cpu_probe_t( );
		cpu_probe_t( unsigned Function );
		cpu_probe_t( unsigned Function, unsigned Leaf );
		cpu_probe_t const& operator()( unsigned Function ) const;
		cpu_probe_t const& operator()( unsigned Function, unsigned Leaf ) const;
		};

public:
	struct version_info_t
	{
		unsigned stepping : 4; // Processor Stepping
		unsigned model    : 4; // Processor Model
		unsigned family   : 4; // Processor Family
		unsigned type     : 2; // 00: Original OEM Processor, 01: Intel OverDrive Processor, 10: Dual Processor, 11: Reserved
		unsigned reserved2: 2; // Reserved
		unsigned modelEx  : 4; // Extended Model
		unsigned familyEx : 8; // Extended Family
		unsigned reserved4: 4; // Reserved

		version_info_t(unsigned data);
	};

	struct model_info_t
	{
		unsigned brandIndex   : 8; // Brand Index
		unsigned cacheLineLen : 8; // The size of the CLFLUSH cache line, in quadwords
		unsigned logicalCPUs  : 8; // The maximum number of addressable IDs for logical processors in this physical package
		unsigned localApicId  : 8; // The initial APIC ID of the processor

		model_info_t(unsigned data);
	};

	struct features_info_t
	{
		unsigned FloatingPointUnit        : 1; // On-Chip x87 FPU
		unsigned VirtualModeEnhancements  : 1; // Virtual 8086 Mode Enhancements
		unsigned DebuggingExtensions      : 1; // Debugging Extensions
		unsigned PageSizeExtension        : 1; // Page Size Extension
		unsigned TimeStampCounter         : 1; // Time Stamp Counter
		unsigned ModelSpecificRegisters   : 1; // Model Specific Registers
		unsigned PhysicalAddressExtension : 1; // Physical Address Extension
		unsigned MachineCheckException    : 1; // Machine Check Exception
		unsigned CAS2                     : 1; // CMPXCHG8B Instruction
		unsigned APIC                     : 1; // APIC On-Chip
		unsigned reserved_10              : 1; // Reserved
		unsigned SysEnterAndSysExit       : 1; // SYSENTER and SYSEXIT Instructions
		unsigned MemoryTypeRangeRegisters : 1; // Memory Type Range Registers
		unsigned PTEGlobalBit             : 1; // PTE Global Bit
		unsigned MachineCheckArchitecture : 1; // Machine Check Architecture
		unsigned CMOV                     : 1; // CMOV Instruction
		unsigned PageAttributeTable       : 1; // Page Attribute Table
		unsigned PageSizeExtension36      : 1; // 36-bit Page Size Extension
		unsigned ProcessorSerialNumber    : 1; // Processor Serial Number
		unsigned CLFLUSH                  : 1; // CLFLUSH Instruction
		unsigned reserved_20              : 1; // Reserved
		unsigned DebugStore               : 1; // Debug Store
		unsigned ACPI                     : 1; // Thermal Monitor & Software Controlled Clock
		unsigned MMX                      : 1; // Intel MMX Technology
		unsigned FXSaveAndFXRStore        : 1; // FXSAVE and FXRSTOR Instructions
		unsigned SSE                      : 1; // SSE Extensions
		unsigned SSE2                     : 1; // SSE2 Extensions
		unsigned SelfSnoop                : 1; // Self-Snoop
		unsigned HyperThreading           : 1; // Multi-core CPU (AMD)
		unsigned ThermalMonitor           : 1; // Thermal Monitor
		unsigned IA64                     : 1; // IA64 Processor Emulating x86
		unsigned PendingBreakEnable       : 1; // Pending Break Enable

		features_info_t(unsigned data);
	};

	struct features_info_ex_t
	{
		unsigned SSE3                     : 1; // SSE3 Extensions
		unsigned PCLMUL                   : 1; // PCLMULQDQ
		unsigned DTES64                   : 1; // 64-bit Debug Trace and EMON Store MSRs
		unsigned MONITOR                  : 1; // MONITOR/MWAIT Instructions
		unsigned DSCPL                    : 1; // CPL Qualified Debug Store	- Reserved (AMD)
		unsigned VMX                      : 1; // Virtual Machine Technology	- Reserved (AMD)
		unsigned SMX                      : 1; // Virtual Machine Technology	- Reserved (AMD)
		unsigned EST                      : 1; // Enhanced SpeedStep Technology- Reserved (AMD)
		unsigned TM2                      : 1; // Thermal Monitor 2			- Reserved (AMD)*
		unsigned SSSE3                    : 1; // SSSE3 Supplemental Streaming SIMD Extensions 3
		unsigned CID                      : 1; // Context ID: the L1 data cache can be set to adaptive or shared mode- Reserved (AMD)
		unsigned SDBG                     : 1; // Silicon Debug Interface		- Reserved (AMD)
		unsigned FMA256                   : 1; // 256-bit FMA extensions		- Reserved (AMD)
		unsigned CX16                     : 1; // CMPXCHG16B
		unsigned ETPRD                    : 1; // xTPR Update Control
		unsigned PDCM                     : 1; // Perf/Debug Capability MSR
		unsigned reserved_16              : 1; // Reserved
		unsigned PCID                     : 1; // Proccess Context Identifier
		unsigned DCA                      : 1; // Direct Cache Access			- Reserved (AMD)
		unsigned SSE4_1                   : 1; // SSSE3 Extensions SSE4.1
		unsigned SSE4_2                   : 1; // SSSE3 Extensions SSE4.2
		unsigned X2APIC                   : 1; // x2APIC support (Intel)		- Reserved (AMD)
		unsigned MOVBE                    : 1; // MOVBE support (Intel)		- Reserved (AMD)
		unsigned POPCNT                   : 1; // SSSE3 Extensions
		unsigned TSC_Deadline             : 1; // TSC Deadline
		unsigned AES                      : 1; // Reserved (AMD)
		unsigned XSAVE                    : 1; // Reserved (AMD)
		unsigned OSXSAVE                  : 1; // Reserved (AMD)
		unsigned AVX                      : 1; // Reserved (AMD)
		unsigned F16C                     : 1; // 16-bit floating-point conversion instructions
		unsigned RDRAND                   : 1; // RDRAND instruction
		unsigned ZERO                     : 1; // Reserved

		features_info_ex_t(unsigned data);
	};

	struct structured_extended_features_t {
		unsigned FSGSBASE    : 1; // FSGSBASE instructions
		unsigned PREFETCHWT1 : 1; // PREFETCHWT1 from ECX
		unsigned AVX512VBMI  : 1; // AVX512VBMI from ECX
		unsigned BMI1        : 1; // Bit Manipulation Instruction Set 1
		unsigned HLE         : 1; // Hardware Lock Elision
		unsigned AVX2        : 1; // Advanced Vector Extensions 2
		unsigned Reserved_6  : 1; // Reserved
		unsigned SMEP        : 1; // Supervisor Mode Execution Protection

		unsigned BMI2        : 1; // Bit Manipulation Instruction Set 2
		unsigned ERMS        : 1; // Enhanced REP MOVSB/STOSB
		unsigned INVPCID     : 1; // INVPCID instruction
		unsigned RTM         : 1; // Restricted Transactional Memory
		unsigned Reserved_12 : 2; // Reserved
		unsigned Intel_MPE   : 1; // Intel Memory Protection Extensions
		unsigned Reserved_15 : 1; // Reserved

		unsigned AVX512F     : 1; // AVX-512 Foundation Instructions
		unsigned AVX512DQ    : 1; // AVX-512 Doubleword and Quadword Instructions
		unsigned RDSEED      : 1; // RDSEED instruction
		unsigned ADX         : 1; // Multi-Precision Add-Carry Instruction Extensions
		unsigned SMAP        : 1; // Supervisor Mode Access Prevention
		unsigned AVX512IFMA  : 1; // AVX-512 Integer Fused Multiply-Add Instructions
		unsigned PCOMMIT     : 1; // PCOMMIT instruction
		unsigned CLFLUSHOPT  : 1; // CLFLUSHOPT instruction

		unsigned CLWB        : 1; // CLWB instruction
		unsigned Intel_PT    : 1; // Intel Processor Trace
		unsigned AVX512PF    : 1; // AVX-512 Prefetch Instructions
		unsigned AVX512ER    : 1; // AVX-512 Exponential and Reciprocal Instructions
		unsigned AVX512CD    : 1; // AVX-512 Conflict Detection Instructions
		unsigned SHA         : 1; // SHA Extensions
		unsigned AVX512BW    : 1; // AVX-512 Byte and Word Instructions
		unsigned AVX512VL    : 1; // AVX-512 Vector Length Extensions

		structured_extended_features_t(unsigned ebx, unsigned ecx, unsigned edx = 0);
	};

	/**
	 * @brief Processor features summary structure
	 *
	 * Provides a simplified view of the most relevant CPU features
	 * for SIMD optimization in a bit-field format.
	 */
	struct cpu_features_t {
		struct {
			WORD MMX         : 1; // MMX Technology
			WORD SSE         : 1; // SSE Extensions
			WORD SSE2        : 1; // SSE2 Extensions
			WORD SSE3        : 1; // SSE3 Extensions
			WORD SSSE3       : 1; // SSSE3 Extensions
			WORD SSE4_1      : 1; // SSE4.1 Extensions
			WORD SSE4_2      : 1; // SSE4.2 Extensions
			WORD AES         : 1; // AES Instructions
			WORD AVX         : 1; // AVX Instructions
			WORD AVX2        : 1; // AVX2 Instructions
			WORD HT          : 1; // Hyper-Threading Technology
			WORD CMOV        : 1; // CMOV Instruction
			WORD POPCNT      : 1; // POPCNT Instruction
			WORD PCLMULDQ    : 1; // PCLMULQDQ Instruction
			WORD CMPXCHG8B   : 1; // CMPXCHG8B Instruction
			WORD CMPXCHG16B  : 1; // CMPXCHG16B Instruction
			WORD FMA256      : 1; // Fused Multiply-Add (FMA) 256-bit
			WORD AVX512F     : 1; // AVX-512 Foundation
			WORD BMI1        : 1; // Bit Manipulation Instruction Set 1
			WORD BMI2        : 1; // Bit Manipulation Instruction Set 2
			WORD ADX         : 1; // Multi-Precision Add-Carry Instruction Extensions
			WORD SHA         : 1; // SHA Extensions
			WORD PREFETCHWT1 : 1; // PREFETCHWT1 Instruction
		};
		cpu_features_t(const features_info_t& Features, const features_info_ex_t& FeaturesEx, const structured_extended_features_t& ExtFeatures = structured_extended_features_t(0, 0, 0));
	};

	struct vendor_id_t;
private:
	struct info_t
	{
		version_info_t     Version;
		model_info_t       Model;
		features_info_ex_t FeaturesEx;
		features_info_t    Features;

		info_t(cpu_probe_t const& Probe);
	};

	info_t      Info;

public:
	processor_features_info();

	/**
	 * @brief Retrieves the processor vendor identification
	 * @return Object containing vendor information (Intel, AMD, etc.)
	 */
	const vendor_id_t vendor_id(void) const;
	const version_info_t& version_info(void) const;
	const model_info_t& model_info(void) const;
	const features_info_t& features_info(void) const;
	const features_info_ex_t& features_info_ex(void) const;
	/**
	 * @brief Provides information about available SIMD instruction sets
	 * @return Structure containing flags for SSE, AVX, AVX-512, etc.
	 */
	const cpu_features_t cpu_features(void) const;
	const structured_extended_features_t structured_extended_features(void) const;
};

/*
 * NOTE TO CONTRIBUTORS:
 * This implementation is functional but still under development.
 * Several areas are marked with comments for future improvement:
 * - Additional CPUID leaf processing
 * - Unused variables are kept for reference and future implementation
 * - Some platform-specific optimizations could be added
 *
 * Feel free to contribute by implementing these features or improving
 * the existing code structure.
 */

inline processor_features_info::cpu_probe_t::cpu_probe_t( ) {
	}

inline processor_features_info::cpu_probe_t::cpu_probe_t(unsigned Function) {
	#if defined(_MSC_VER) || defined(__INTEL_COMPILER) && defined(_WIN32)
		__cpuid(data, int(Function));
	#elif defined(__GNUC__) || defined(__clang__)
		__get_cpuid(Function, (unsigned int*)&data[0], (unsigned int*)&data[1],
			(unsigned int*)&data[2], (unsigned int*)&data[3]);
	#endif
	}

inline processor_features_info::cpu_probe_t::cpu_probe_t(unsigned Function, unsigned Leaf) {
	#if defined(_MSC_VER) || defined(__INTEL_COMPILER) && defined(_WIN32)
		__cpuidex(data, int(Function), int(Leaf));
	#elif defined(__GNUC__) || defined(__clang__)
		__cpuid_count(Function, Leaf, data[0], data[1], data[2], data[3]);
	#endif
	}

inline processor_features_info::cpu_probe_t const& processor_features_info::cpu_probe_t::operator()(unsigned Function) const {
	#if defined(_MSC_VER) || defined(__INTEL_COMPILER) && defined(_WIN32)
		__cpuid(data, int(Function));
	#elif defined(__GNUC__) || defined(__clang__)
		__get_cpuid(Function, (unsigned int*)&data[0], (unsigned int*)&data[1],
			(unsigned int*)&data[2], (unsigned int*)&data[3]);
	#endif
		return *this;
	}

inline processor_features_info::cpu_probe_t const& processor_features_info::cpu_probe_t::operator()(unsigned Function, unsigned Leaf) const {
	#if defined(_MSC_VER) || defined(__INTEL_COMPILER) && defined(_WIN32)
		__cpuidex(data, int(Function), int(Leaf));
	#elif defined(__GNUC__) || defined(__clang__)
		__cpuid_count(Function, Leaf, data[0], data[1], data[2], data[3]);
	#endif
		return *this;
	}

inline processor_features_info::version_info_t::version_info_t( unsigned data ){
	*reinterpret_cast<unsigned*>( this ) = data;
	}

inline processor_features_info::model_info_t::model_info_t( unsigned data ){
	*reinterpret_cast<unsigned*>( this ) = data;
	}

inline processor_features_info::features_info_t::features_info_t( unsigned data ){
	*reinterpret_cast<unsigned*>( this ) = data;
	}

inline processor_features_info::features_info_ex_t::features_info_ex_t( unsigned data ){
	*reinterpret_cast<unsigned*>( this ) = data;
	}

inline processor_features_info::structured_extended_features_t::structured_extended_features_t(unsigned ebx, unsigned ecx, unsigned edx) {
	FSGSBASE = (ebx >> 0) & 1;
	PREFETCHWT1 = (ecx >> 0) & 1;
	AVX512VBMI = (ecx >> 1) & 1;
	BMI1 = (ebx >> 3) & 1;
	HLE = (ebx >> 4) & 1;
	AVX2 = (ebx >> 5) & 1;
	SMEP = (ebx >> 7) & 1;
	Reserved_6 = (ebx >> 6) & 1;
	BMI2 = (ebx >> 8) & 1;
	ERMS = (ebx >> 9) & 1;
	INVPCID = (ebx >> 10) & 1;
	RTM = (ebx >> 11) & 1;
	Reserved_12 = (ebx >> 12) & 1;
	Reserved_12 |= ((ebx >> 13) & 1) << 1;
	Intel_MPE = (ebx >> 14) & 1;
	Reserved_15 = (ebx >> 15) & 1;

	AVX512F = (ebx >> 16) & 1;
	AVX512DQ = (ebx >> 17) & 1;
	RDSEED = (ebx >> 18) & 1;
	ADX = (ebx >> 19) & 1;
	SMAP = (ebx >> 20) & 1;
	AVX512IFMA = (ebx >> 21) & 1;
	PCOMMIT = (ebx >> 22) & 1;
	CLFLUSHOPT = (ebx >> 23) & 1;

	CLWB = (ebx >> 24) & 1;
	Intel_PT = (ebx >> 25) & 1;
	AVX512PF = (ebx >> 26) & 1;
	AVX512ER = (ebx >> 27) & 1;
	AVX512CD = (ebx >> 28) & 1;
	SHA = (ebx >> 29) & 1;
	AVX512BW = (ebx >> 30) & 1;
	AVX512VL = (ebx >> 31) & 1;
	}

struct processor_features_info::vendor_id_t
	{
	private:
		char vendor[ 14 ];
		char brand[ 50 ];

	public:
		vendor_id_t( cpu_probe_t const& Probe );

		char const* vendor_begin( ) const;
		char const* vendor_end( ) const;
		char const* brand_begin( ) const;
		char const* brand_end( ) const;
	};

inline processor_features_info::vendor_id_t::vendor_id_t(cpu_probe_t const& Probe)
	{
	memset(this, 0, sizeof *this);

	// Probe(0x00000000);
	unsigned maxProbeFunct = Probe.rEAX;
	*(unsigned*)&vendor[0] = Probe.rEBX;
	*(unsigned*)&vendor[4] = Probe.rEDX;
	*(unsigned*)&vendor[8] = Probe.rECX;

	if( 0x00000002 <= maxProbeFunct ) {
		//	0x00000002; // Deterministic Cache Information
		}
	if( 0x00000003 <= maxProbeFunct ) {
		//	0x00000003; // Monitor Feature Information
		}
	if( 0x00000004 <= maxProbeFunct ) {
		//	0x00000004; // Deterministic Cache Parameters
		Probe(0x00000004);
		struct deterministic_cache_paramteters_t { // Probe.rEAX
			deterministic_cache_paramteters_t( unsigned data, unsigned data_ex )
				{
				*reinterpret_cast<unsigned*>( this ) = data;
				*reinterpret_cast<unsigned*>( this ) &= ~0x3800;
				*reinterpret_cast<unsigned*>( this ) |= ( data_ex << 11 ) & 0x3800;
				}
			ulong has_no_more_cache_informations : 1; // Null - No more caches
			ulong has_data_cache_in_informations : 1; // Data cache
			ulong has_code_cache_in_informations : 1; // Instruction Cache
			ulong has_both_cache_in_informations : 1; // Unified cache
			ulong has_reserved_bit4_informations : 1; // RESERVED
			ulong has_cache_level_n_informations : 3; // Cache level
			ulong has_hw_self_initializing_cache : 1; // Self initializing cache level (no need SW init)
			ulong has_hw_fully_associative_cache : 1; // Fully associative cache
		//	ulong has_reserved_word_informations : 4; // RESERVED
			ulong has_reserved_word_informations : 1; // RESERVED with new size
			ulong has_wbinvd_invd_behavior_lower : 1; // WBINVD/INVD behavior on lower level caches - FROM ECX
			ulong has_cache_inclusiveness_option : 1; // Cache inclusiveness                        - FROM ECX
			ulong has_complex_cache_indexing_bit : 1; // Complex cache indexing                     - FROM ECX
			ulong max_addr_id_logical_processors : 12;// Max addressable id's for logical processor sharing this cache
			ulong max_addr_id_physical_cpu_cores : 6; // Max addressable id's for physical processor cores in the package
			};
		struct deterministic_cache_limits_t { // Probe.rEBX
			deterministic_cache_limits_t( unsigned data )
				{
				*reinterpret_cast<unsigned*>( this ) = data;
				}
			ulong system_coherency_line_num_size : 12;// System coherency line size
			ulong physical_line_partitions_count : 10;// Physical line partitions
			ulong ways_of_connectivity_per_cache : 10;// Ways of associativity
			};
		deterministic_cache_paramteters_t(Probe.rEAX, Probe.rECX);
		deterministic_cache_limits_t(Probe.rEBX);
		//auto cacheParams = deterministic_cache_paramteters_t(Probe.rEAX, Probe.rECX);
		//auto cacheLimits = deterministic_cache_limits_t(Probe.rEBX);
		}
	if( 0x00000005 <= maxProbeFunct ) {
		//	0x00000005; // Monitor/Wait Feature Information
		}
	if( 0x00000006 <= maxProbeFunct ) {
		//	0x00000006; // Digital Temperature/Power Management Information - Thermal and Power Management
		}
	if( 0x00000007 <= maxProbeFunct ) {
		//	0x00000007; // Structured Extended feature
		Probe(0x00000007);
		//if( Probe.rEBX )
		structured_extended_features_t( Probe.rEBX, Probe.rECX );
		// Reports the maximum number sub-leaves that are supported in leaf 07H
		// auto MaxSubLeaves = Probe.rEAX, Leaf = 0u;
		// do { Probe( 0x00000007, ++Leaf ); } while( Leaf < MaxSubLeaves );
		}
	if( 0x00000009 <= maxProbeFunct ) {
		// 0x00000009: Direct Cache Access Information
		/*
		EAX         : Value of bits [31:0] of IA32_PLATFORM_DCA_CAP MSR (address 1F8H)
		EBX,ECX,EDX : Reserved

		*/
	//	__asm CMPEQ xmm0, xmm0; // set_one( void )
		}
	if( 0x0000000A <= maxProbeFunct ) {
		//	0x0000000A; // Architectural Performance Monitoring Information
		/*
		EAX Bits 07 - 00: Version ID of architectural performance monitoring
			Bits 15- 08: Number of general-purpose performance monitoring counter per logical processor
			Bits 23 - 16: Bit width of general-purpose, performance monitoring counter
			Bits 31 - 24: Length of EBX bit vector to enumerate architectural performance monitoring events
		EBX Bit 00: Core cycle event not available if 1
			Bit 01: Instruction retired event not available if 1
			Bit 02: Reference cycles event not available if 1
			Bit 03: Last-level cache reference event not available if 1
			Bit 04: Last-level cache misses event not available if 1
			Bit 05: Branch instruction retired event not available if 1
			Bit 06: Branch mispredict retired event not available if 1
			Bits 31- 07: Reserved = 0
		ECX Reserved = 0
		EDX Bits 04 - 00: Number of fixed-function performance counters (if Version ID > 1)
			Bits 12- 05: Bit width of fixed-function performance counters (if Version ID > 1)
			Bits 31- 13: Reserved = 0
		*/
		}
	// 0xD - Processor Extended State Enumeration Main
	/*
	== Leaf 0DH main leaf (ECX = 0) ==
	EAX Bits 31-00: Reports the valid bit fields of the lower 32 bits of the XFEATURE_ENABLED_MASK register.
					If a bit is 0, the corresponding bit field in XCR0 is reserved.
		Bit 00: legacy x87
		Bit 01: 128-bit SSE
		Bit 02: 256-bit AVX
	EBX Bits 31-00: Maximum size (bytes, from the beginning of the XSAVE/XRSTOR save area) required by
					enabled features in XCR0. May be different than ECX if some features at the end of the XSAVE save
					area are not enabled.
	ECX Bits 31-00:	Maximum size (bytes, from the beginning of the XSAVE/XRSTOR save area) of the
					XSAVE/XRSTOR save area required by all supported features in the processor, i.e all the valid bit
					fields in XCR0.
	EDX Bits 31-00: Reports the valid bit fields of the upper 32 bits of the XCR0 register. If a bit is 0, the corresponding
					bit field in XCR0 is reserved
	==  Sub-leaf (EAX = 0DH, ECX = 1) ==
	EAX Bit  00:    XSAVEOPT is available
		Bits 31-1:  Reserved
	EBX,ECX,EDX: -  Reserved
	== Sub-leaves (EAX = 0DH, ECX = n, n > 1) ==
	EAX Bits 31-0:  The size in bytes (from the offset specified in EBX) of the save area for an extended state
					feature associated with a valid sub-leaf index, n. This field reports 0 if the sub-leaf index, n, is
					invalid*.
	EBX Bits 31-0:  The offset in bytes of this extended state component's save area from the beginning of
					the XSAVE/XRSTOR area. This field reports 0 if the sub-leaf index, n, is invalid*.
	ECX Bit 0		is set if the sub-leaf index, n, maps to a valid bit in the IA32_XSS MSR and bit 0 is clear if n maps
					to a valid bit in XCR0. Bits 31-1 are reserved. This field reports 0 if the sub-leaf index, n, is invalid*.
	EDX				This field reports 0 if the sub-leaf index, n, is invalid*; otherwise it is reserved.

	*The highest valid sub-leaf index, n, is (POPCNT(CPUID.(EAX=0D, ECX=0):EAX) + POPCNT(CPUID.(EAX=0D, ECX=0):EDX) - 1)
	*/
	Probe(0x80000000);
	unsigned maxExtended = Probe.rEAX;

	Probe(0x80000001);
	bool has_lahf_sahf_in_64bit_mode_on = ( Probe.rECX >> 0 ) & 1;  // LAHF/SAHF available in 64-bit mode
	bool has_leading_zero_count_support = ( Probe.rECX >> 5 ) & 1;  // LZCNT instruction
	bool has_prefetchw_instruction_code = ( Probe.rECX >> 8 ) & 1;  // PREFETCHW
	bool has_fma4_instruction_available = ( Probe.rECX >> 16 ) & 1;
	bool has_syscall_and_sysret_support = ( Probe.rEDX >> 11 ) & 1; // SYSCALL/SYSRET available (when in 64-bit mode)
	bool has_execute_disable_bit_enable = ( Probe.rEDX >> 20 ) & 1;
//	bool has_reserved_intel_feature_set = ( Probe.rEDX >> 27 ) & 1;
	bool has_64bit_technology_available = ( Probe.rEDX >> 29 ) & 1;
//	bool has_3DExt_technology_available = ( Probe.rEDX >> 30 ) & 1; // AMD
//	bool has_3DNow_technology_available = ( Probe.rEDX >> 31 ) & 1; // AMD

	unsigned maxBrand = maxExtended;
	if(maxBrand > 0x80000004)
		maxBrand = 0x80000004;
	char* ptr = (char*)brand;
	for(unsigned func=0x80000002; func <= maxBrand; ++func) {
		memcpy(ptr, &Probe(func), sizeof Probe);
		ptr += sizeof Probe;
		}

//	0x80000005 [0-3] [ 0-31] L1 cache and TLB identifiers (AMD) - Reserved (Intel)

	if( 0x80000006 <= maxExtended ) {
		/*
		0x80000006 ECX  [ 0- 7] Cache line size in bytes
		  	            [12-15] L2 associativity field encodings:
								00H - Disabled
								01H - Direct mapped
								02H - 2-way
								04H - 4-way
								06H - 8-way
								08H - 16-way
								0FH - Fully associative
		  	            [16-31] Cache size in 1K units
		*/
		}

	//	0x80000007 [0-3] [ 0-31] Advanced power management information (AMD) - Reserved (Intel)

	if( 0x80000008 <= maxExtended ) {
		//	0x80000008 EAX [ 0- 7] Physical address bits
		//	               [ 8-15] Virtual address bits
		}
	}

inline char const* processor_features_info::vendor_id_t::vendor_begin( ) const {
	return vendor;
	}

inline char const* processor_features_info::vendor_id_t::vendor_end( ) const {
	return vendor + sizeof vendor - 2;
	}

inline char const* processor_features_info::vendor_id_t::brand_begin( ) const {
	return brand;
	}

inline char const* processor_features_info::vendor_id_t::brand_end( ) const {
	return brand + sizeof brand - 2;
	}

inline processor_features_info::cpu_features_t::cpu_features_t(
	const features_info_t& Features,
	const features_info_ex_t& FeaturesEx,
	const structured_extended_features_t& ExtFeatures)
	: MMX(Features.MMX)
	, SSE(Features.SSE)
	, SSE2(Features.SSE2)
	, SSE3(FeaturesEx.SSE3)
	, SSSE3(FeaturesEx.SSSE3)
	, SSE4_1(FeaturesEx.SSE4_1)
	, SSE4_2(FeaturesEx.SSE4_2)
	, AES(FeaturesEx.AES)
	, AVX(FeaturesEx.AVX)
	, AVX2(ExtFeatures.AVX2)
	, HT(Features.HyperThreading)
	, CMOV(Features.CMOV)
	, POPCNT(FeaturesEx.POPCNT)
	, PCLMULDQ(FeaturesEx.PCLMUL)
	, CMPXCHG8B(Features.CAS2)
	, CMPXCHG16B(FeaturesEx.CX16)
	, FMA256(FeaturesEx.FMA256)
	, AVX512F(ExtFeatures.AVX512F)
	, BMI1(ExtFeatures.BMI1)
	, BMI2(ExtFeatures.BMI2)
	, ADX(ExtFeatures.ADX)
	, SHA(ExtFeatures.SHA)
	, PREFETCHWT1(ExtFeatures.PREFETCHWT1)
	{
	}

inline processor_features_info::info_t::info_t(cpu_probe_t const& Probe)
	:	Version    (Probe.rEAX)
	,	Model      (Probe.rEBX)
	,	FeaturesEx (Probe.rECX)
	,	Features   (Probe.rEDX)
	{
	Probe( 7 );
	}

inline processor_features_info::processor_features_info()
	:	Info(cpu_probe_t(1))
	{
	#if defined(_MSC_VER) || defined(__INTEL_COMPILER) && defined(_WIN32)
		SYSTEM_INFO SysInfo;
		::GetSystemInfo(&SysInfo);
		SysInfo.dwNumberOfProcessors;
		SysInfo.dwActiveProcessorMask;
		SysInfo.wProcessorLevel;
		SysInfo.dwProcessorType;
		SysInfo.lpMinimumApplicationAddress;
		SysInfo.lpMaximumApplicationAddress;
		SysInfo.dwAllocationGranularity;
		SysInfo.dwPageSize;
	#elif defined(__GNUC__) || defined(__clang__)
		long numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);
		long pageSize = sysconf(_SC_PAGESIZE);
	#endif
	}

inline const processor_features_info::vendor_id_t processor_features_info::vendor_id(void) const {
	return cpu_probe_t(0);
	}

inline const processor_features_info::version_info_t& processor_features_info::version_info(void) const {
	return Info.Version;
	}

inline const processor_features_info::model_info_t& processor_features_info::model_info(void) const {
	return Info.Model;
	}

inline const processor_features_info::features_info_t& processor_features_info::features_info(void) const {
	return Info.Features;
	}

inline const processor_features_info::features_info_ex_t& processor_features_info::features_info_ex(void) const {
	return Info.FeaturesEx;
	}

inline const processor_features_info::structured_extended_features_t processor_features_info::structured_extended_features(void) const {
	cpu_probe_t Probe(0x00000007);
	return structured_extended_features_t(Probe.rEBX, Probe.rECX, Probe.rEDX);
}

inline const processor_features_info::cpu_features_t processor_features_info::cpu_features(void) const {
	return cpu_features_t(Info.Features, Info.FeaturesEx, structured_extended_features());
}

}//namespace platform

#endif//PROCESSOR_FEATURES_INFO_H

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
 * Scalar digits comparison helper
 */
inline auto scalar_digits(const char* p, size_t Count) -> const char* {
    while (Count--) {
        const unsigned c = static_cast<unsigned char>(*p);
        if (c - 0x30u > 0x09u) return p;
        ++p;
    }
    return nullptr;
}

/**
 * Scalar skip-spaces helper: returns first non-space within Count, or nullptr if none
 */
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


inline __m512i zmm_load_aligned(const char* p) noexcept {
    assert((reinterpret_cast<uintptr_t>(p) & 63u) == 0 && "Data must be 64-byte aligned for AVX-512 load");
    return _mm512_load_si512(reinterpret_cast<const __m512i*>(p));
}
inline __m256i ymm_load_aligned(const char* p) noexcept {
    assert((reinterpret_cast<uintptr_t>(p) & 31u) == 0 && "Data must be 32-byte aligned for AVX2 load");
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
}
inline __m128i xmm_load_aligned(const char* p) noexcept {
    assert((reinterpret_cast<uintptr_t>(p) & 15u) == 0 && "Data must be 16-byte aligned for SSE load");
    return _mm_load_si128(reinterpret_cast<const __m128i*>(p));
}

inline __m512i zmm_load_unaligned(const char* p) noexcept {
    return _mm512_loadu_si512(reinterpret_cast<const __m512i*>(p));
}
inline __m256i ymm_load_unaligned(const char* p) noexcept {
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
}
inline __m128i xmm_load_unaligned(const char* p) noexcept {
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
}

inline __m512i zmm_load_aligned(std::string_view Text, size_t Ofs = 0) noexcept {
    return zmm_load_aligned(Text.data() + Ofs);
}
inline __m256i ymm_load_aligned(std::string_view Text, size_t Ofs = 0) noexcept {
    return ymm_load_aligned(Text.data() + Ofs);
}
inline __m128i xmm_load_aligned(std::string_view Text, size_t Ofs = 0) noexcept {
    return xmm_load_aligned(Text.data() + Ofs);
}

inline __m512i zmm_load_unaligned(std::string_view Text, size_t Ofs = 0) noexcept {
    return zmm_load_unaligned(Text.data() + Ofs);
}
inline __m256i ymm_load_unaligned(std::string_view Text, size_t Ofs = 0) noexcept {
    return ymm_load_unaligned(Text.data() + Ofs);
}
inline __m128i xmm_load_unaligned(std::string_view Text, size_t Ofs = 0) noexcept {
    return xmm_load_unaligned(Text.data() + Ofs);
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
 * Parallel skip-spaces functions for 128-bit SSE vectors
 */
inline auto xmm_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 16u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    const __m128i SP = _mm_set1_epi8(' ');
    const __m128i TB = _mm_set1_epi8('\t');
    const __m128i NL = _mm_set1_epi8('\n');
    const __m128i CR = _mm_set1_epi8('\r');

    for (size_t i = 0; i < blocks; ++i) {
        const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        const __m128i m = _mm_or_si128(
            _mm_or_si128(_mm_cmpeq_epi8(v, SP), _mm_cmpeq_epi8(v, TB)),
            _mm_or_si128(_mm_cmpeq_epi8(v, NL), _mm_cmpeq_epi8(v, CR))
        );
        const uint32_t k = static_cast<uint32_t>(_mm_movemask_epi8(m));
        const uint32_t non = (~k) & 0xFFFFu;
        if (non) return p + static_cast<size_t>(_tzcnt_u32(non));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return scalar_skip_spaces(p, tail);
}
inline auto xmm_parallel_skip_spaces(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return xmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
}
inline auto xmm_parallel_skip_spaces(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return xmm_parallel_skip_spaces(Text, 0, Text.size());
}
inline auto xmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
    assert(Text.size() >= Ofs + Count);
    if (const char* p = xmm_parallel_skip_spaces(Text, Ofs, Count))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto xmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) -> size_t {
    assert(Text.size() >= Ofs);
    if (const char* p = xmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto xmm_parallel_skip_spaces_pos(std::string_view Text) -> size_t {
    if (Text.empty()) return Text.npos;
    if (const char* p = xmm_parallel_skip_spaces(Text, 0, Text.size()))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}

/**
 * Parallel skip-spaces functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 32u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    const __m256i SP = _mm256_set1_epi8(' ');
    const __m256i TB = _mm256_set1_epi8('\t');
    const __m256i NL = _mm256_set1_epi8('\n');
    const __m256i CR = _mm256_set1_epi8('\r');

    for (size_t i = 0; i < blocks; ++i) {
        const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        const __m256i m = _mm256_or_si256(
            _mm256_or_si256(_mm256_cmpeq_epi8(v, SP), _mm256_cmpeq_epi8(v, TB)),
            _mm256_or_si256(_mm256_cmpeq_epi8(v, NL), _mm256_cmpeq_epi8(v, CR))
        );
        const uint32_t k = static_cast<uint32_t>(_mm256_movemask_epi8(m));
        const uint32_t non = ~k;
        if (non) return p + static_cast<size_t>(_tzcnt_u32(non));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return scalar_skip_spaces(p, tail);
}
inline auto ymm_parallel_skip_spaces(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return ymm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
}
inline auto ymm_parallel_skip_spaces(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return ymm_parallel_skip_spaces(Text, 0, Text.size());
}
inline auto ymm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
    assert(Text.size() >= Ofs + Count);
    if (const char* p = ymm_parallel_skip_spaces(Text, Ofs, Count))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto ymm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) -> size_t {
    assert(Text.size() >= Ofs);
    if (const char* p = ymm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto ymm_parallel_skip_spaces_pos(std::string_view Text) -> size_t {
    if (Text.empty()) return Text.npos;
    if (const char* p = ymm_parallel_skip_spaces(Text, 0, Text.size()))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}

/**
 * Parallel skip-spaces functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 64u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    const __m512i SP = _mm512_set1_epi8(' ');
    const __m512i TB = _mm512_set1_epi8('\t');
    const __m512i NL = _mm512_set1_epi8('\n');
    const __m512i CR = _mm512_set1_epi8('\r');

    for (size_t i = 0; i < blocks; ++i) {
        const __m512i v = _mm512_loadu_si512(reinterpret_cast<const void*>(p));
        const __mmask64 k = _mm512_cmpeq_epi8_mask(v, SP)
                          | _mm512_cmpeq_epi8_mask(v, TB)
                          | _mm512_cmpeq_epi8_mask(v, NL)
                          | _mm512_cmpeq_epi8_mask(v, CR);
        const __mmask64 non = ~k;
        if (non) return p + static_cast<size_t>(zmm_get_first_match(non));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return scalar_skip_spaces(p, tail);
}
inline auto zmm_parallel_skip_spaces(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return zmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
}
inline auto zmm_parallel_skip_spaces(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return zmm_parallel_skip_spaces(Text, 0, Text.size());
}
inline auto zmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) -> size_t {
    assert(Text.size() >= Ofs + Count);
    if (const char* p = zmm_parallel_skip_spaces(Text, Ofs, Count))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto zmm_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) -> size_t {
    assert(Text.size() >= Ofs);
    if (const char* p = zmm_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
}
inline auto zmm_parallel_skip_spaces_pos(std::string_view Text) -> size_t {
    if (Text.empty()) return Text.npos;
    if (const char* p = zmm_parallel_skip_spaces(Text, 0, Text.size()))
        return static_cast<size_t>(p - Text.data());
    return Text.npos;
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
inline auto xmm_parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    if (Count == 0) return nullptr;

    constexpr size_t W = 16u;
    const auto Char = _mm_set1_epi8(C);
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        const int mask = xmm_mask_find_bytes_equal_to(
            _mm_loadu_si128(reinterpret_cast<const __m128i*>(p)),
            Char
        );
        if (mask) return p + static_cast<size_t>(_tzcnt_u32(mask));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, static_cast<unsigned char>(C), tail));
}
inline auto xmm_parallel_find(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return xmm_parallel_find(Text, C, Ofs, Text.size() - Ofs);
}
inline auto xmm_parallel_find(std::string_view Text, char C) -> const char* {
    if (Text.empty()) return nullptr;
    return xmm_parallel_find(Text, C, 0, Text.size());
}

/**
 * Parallel find functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    if (Count == 0) return nullptr;

    constexpr size_t W = 32u;
    const auto Char = _mm256_set1_epi8(C);
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        const int mask = ymm_mask_find_bytes_equal_to(
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)),
            Char
        );
        if (mask) return p + static_cast<size_t>(_tzcnt_u32(mask));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, static_cast<unsigned char>(C), tail));
}
inline auto ymm_parallel_find(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return ymm_parallel_find(Text, C, Ofs, Text.size() - Ofs);
}
inline auto ymm_parallel_find(std::string_view Text, char C) -> const char* {
    if (Text.empty()) return nullptr;
    return ymm_parallel_find(Text, C, 0, Text.size());
}

/**
 * Parallel find functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find(std::string_view Text, char C, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    if (Count == 0) return nullptr;

    constexpr size_t W = 64u;
    const auto Char = _mm512_set1_epi8(C);
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        const __mmask64 k = zmm_mask_find_bytes_equal_to(
            _mm512_loadu_si512(reinterpret_cast<const void*>(p)),
            Char
        );
        if (k) return p + static_cast<size_t>(zmm_get_first_match(k));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, static_cast<unsigned char>(C), tail));
}
inline auto zmm_parallel_find(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return zmm_parallel_find(Text, C, Ofs, Text.size() - Ofs);
}
inline auto zmm_parallel_find(std::string_view Text, char C) -> const char* {
    if (Text.empty()) return nullptr;
    return zmm_parallel_find(Text, C, 0, Text.size());
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
inline auto xmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = xmm_parallel_find(Text, C, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto xmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = xmm_parallel_find(Text, C, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto xmm_parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = xmm_parallel_find(Text, C, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Parallel find position functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = ymm_parallel_find(Text, C, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto ymm_parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = ymm_parallel_find(Text, C, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto ymm_parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = ymm_parallel_find(Text, C, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Parallel find position functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = zmm_parallel_find(Text, C, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto zmm_parallel_find_pos(std::string_view Text, char C, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = zmm_parallel_find(Text, C, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto zmm_parallel_find_pos(std::string_view Text, char C) -> size_t {
    auto Found = zmm_parallel_find(Text, C, 0, Text.size());
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
inline auto xmm_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 16u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        int mask = xmm_mask_find_bytes_equal_to(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p)), xmm_Quote);
        if (mask) return p + static_cast<size_t>(_tzcnt_u32(mask));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, '"', tail));
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
    assert(Text.size() >= Ofs + Count);

    constexpr size_t W = 32u;
    const char* p = Text.data() + Ofs;

    // Bounded full blocks (no reads outside [Ofs, Ofs+Count))
    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        int mask = ymm_mask_find_bytes_equal_to(
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)),
            ymm_Quote
        );
        if (mask) return p + static_cast<size_t>(_tzcnt_u32(mask));
        p += W;
    }

    // Bounded epilogue
    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, '"', tail));
}
inline auto ymm_parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
    return ymm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
}
inline auto ymm_parallel_find_quote(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return ymm_parallel_find_quote(Text, 0, Text.size());
}

/**
 * Parallel find functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 64u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        __mmask64 k = zmm_mask_find_bytes_equal_to(
            _mm512_loadu_si512(reinterpret_cast<const void*>(p)),
            zmm_Quote
        );
        if (k) return p + static_cast<size_t>(zmm_get_first_match(k));
        p += W;
    }

    const size_t tail = Count - blocks * W;
    return static_cast<const char*>(std::memchr(p, '"', tail));
}
inline auto zmm_parallel_find_quote(std::string_view Text, size_t Ofs) -> const char* {
    assert(Text.size() >= Ofs);
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
inline auto xmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = xmm_parallel_find_quote(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto xmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = xmm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto xmm_parallel_find_quote_pos(std::string_view Text) -> size_t {
    auto Found = xmm_parallel_find_quote(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Parallel find position functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = ymm_parallel_find_quote(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto ymm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = ymm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto ymm_parallel_find_quote_pos(std::string_view Text) -> size_t {
    auto Found = ymm_parallel_find_quote(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Parallel find position functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = zmm_parallel_find_quote(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto zmm_parallel_find_quote_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = zmm_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto zmm_parallel_find_quote_pos(std::string_view Text) -> size_t {
    auto Found = zmm_parallel_find_quote(Text, 0, Text.size());
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
inline auto xmm_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 16u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        int mask = xmm_mask_find_bytes_not_in_range(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p)), xmm_Zero, xmm_Nine);
        if (mask) return p + static_cast<size_t>(_tzcnt_u32(mask));
        p += W;
    }

    size_t tail = Count - blocks * W;
    return scalar_digits(p, tail + 1); // +1 to include the null terminator in the scalar scan
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
    assert(Text.size() >= Ofs + Count);

    constexpr size_t W = 32u;
    const char* p = Text.data() + Ofs;

    // Bounded full blocks
    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        int mask = ymm_mask_find_bytes_not_in_range(
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)),
            ymm_Zero, ymm_Nine
        );
        if (mask) return p + static_cast<size_t>(_tzcnt_u32(mask));
        p += W;
    }

    // Bounded epilogue
    size_t tail = Count - blocks * W;
    return scalar_digits(p, tail + 1); // +1 to include the null terminator in the scalar scan
}
inline auto ymm_parallel_digits(std::string_view Text) -> const char* {
    if (Text.empty()) return nullptr;
    return ymm_parallel_digits(Text, 0, Text.size());
}
inline auto ymm_parallel_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    return ymm_parallel_digits(Text, Ofs, Text.size() - Ofs);
}

/**
 * Parallel comparison functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) -> const char* {
    assert(Text.size() >= Ofs + Count);
    constexpr size_t W = 64u;
    const char* p = Text.data() + Ofs;

    const size_t blocks = Count / W;
    for (size_t i = 0; i < blocks; ++i) {
        __mmask64 k = zmm_mask_find_bytes_not_in_range(_mm512_loadu_si512(reinterpret_cast<const void*>(p)), zmm_Zero, zmm_Nine);
        if (k) return p + static_cast<size_t>(zmm_get_first_match(k));
        p += W;
    }

    size_t tail = Count - blocks * W;
    return scalar_digits(p, tail + 1); // +1 to include the null terminator in the scalar scan
}
inline auto zmm_parallel_digits(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
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
    auto Found = xmm_parallel_digits(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto xmm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = xmm_parallel_digits(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto xmm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = xmm_parallel_digits(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Parallel comparison functions for 256-bit AVX vectors
 */
inline auto ymm_parallel_digits_pos(std::string_view Text) -> size_t {
    auto Found = ymm_parallel_digits(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto ymm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = ymm_parallel_digits(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto ymm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = ymm_parallel_digits(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

/**
 * Parallel comparison functions for 512-bit AVX-512 vectors
 */
inline auto zmm_parallel_digits_pos(std::string_view Text) -> size_t {
    auto Found = zmm_parallel_digits(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto zmm_parallel_digits_pos(std::string_view Text, size_t Ofs) {
    assert(Text.size() >= Ofs);
    auto Found = zmm_parallel_digits(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline auto zmm_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) {
    assert(Text.size() >= Ofs + Count);
    auto Found = zmm_parallel_digits(Text, Ofs, Count);
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
#endif

// -----------------------------------------------------------------------------
// SECTION: C++ON Core Headers
// -----------------------------------------------------------------------------
// Main C++ON components: exceptions, type alternatives, core types, parser,
// visitors, printer, and references. These headers define the public API and
// all core features of the library.
// -----------------------------------------------------------------------------

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

/**
 * @brief Exception for object reference lost.
 */
class object_reference_lost_error : public std::logic_error {
public:
    explicit object_reference_lost_error(std::string_view detail = {})
        : std::logic_error{ detail.empty()
            ? "object reference lost"
            : std::string{ "object reference lost: " }.append(detail) } {}
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

/*EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
 * @section I/O Exceptions
 *EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/

/**
 * @brief Exception for file operations errors.
 */
class file_operation_error : public std::runtime_error {
public:
    explicit file_operation_error(std::string_view filename, std::string_view operation = {})
        : std::runtime_error{ operation.empty()
            ? std::string{"file operation error: "}.append(filename)
            : std::string{"failed to "}.append(operation).append(" file: ").append(filename) } {}
};

} // namespace ch5

#endif // CPPON_EXCEPTIONS_H

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

inline bool operator==(const number_t& lhs, const number_t& rhs) noexcept {
    return lhs.value == rhs.value;
}
inline bool operator!=(const number_t& lhs, const number_t& rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator==(const path_t& lhs, const path_t& rhs) noexcept {
    return lhs.value == rhs.value;
}
inline bool operator!=(const path_t& lhs, const path_t& rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator==(const blob_string_t& lhs, const blob_string_t& rhs) noexcept {
    return lhs.value == rhs.value;
}
inline bool operator!=(const blob_string_t& lhs, const blob_string_t& rhs) noexcept {
    return !(lhs == rhs);
}

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

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-printer-state.h : C++ON Printer State Management
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_PRINTER_STATE_H
#define CPPON_PRINTER_STATE_H

namespace ch5 {
namespace printer_state {

typedef std::unordered_set<std::string_view> string_set_t;

struct state {
	std::string Out;              /**< The output string that stores the printed JSON representation. */
	string_set_t Compacted;       /**< The set of labels for compacted objects. */
	int Level{ 0 };               /**< The current indentation level. */
	int Tabs{ 2 };                /**< The number of spaces per indentation level. */
	int Margin{ 0 };              /**< The margin of the printed JSON representation. */
	bool Reserve{ true };         /**< Flag indicating whether to reserve memory for printing the JSON representation. */
	bool Flatten{ false };        /**< Flag indicating whether to expand all objects. */
	bool Pretty{ false };         /**< Flag indicating whether to print the JSON representation in a pretty format. */
	bool AltLayout{ false };      /**< Flag indicating whether to print the JSON representation in an alternative layout. */
	bool Compatible{ false };     /**< Flag indicating whether to print the JSON representation in a compatible format. */
	bool Exact{ false };          /**< Flag indicating whether to print numbers in exact format. */
	bool RetainBuffer{ false };   /**< Flag indicating whether to retain the buffer between printing sessions. */
	cppon to_cppon() const;
};

} // namespace printer_state
} // namespace ch5

#endif // CPPON_PRINTER_STATE_H

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-swar.h : C++ON SIMD Within A Register (SWAR) utilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_SWAR_H
#define CPPON_SWAR_H

namespace ch5 {

// SWAR constants
constexpr uint64_t kOnes = 0x0101010101010101ULL;   // (~0ULL / 255)
constexpr uint64_t kHigh = 0x8080808080808080ULL;   // (~0ULL / 255) * 128

inline uint64_t m64_load_aligned(const char* p) noexcept {
    assert((reinterpret_cast<uintptr_t>(p) & 7u) == 0 && "Pointer must be 8-byte aligned for m64 load");
    return *reinterpret_cast<const uint64_t*>(p);
}
inline uint64_t m64_load_unaligned(const char* p) noexcept {
    uint64_t v; std::memcpy(&v, p, sizeof(v)); return v;
}

inline uint64_t m64_load_aligned(std::string_view Text, size_t Ofs = 0) noexcept {
	assert(Text.size() >= Ofs + 8);
    return m64_load_aligned(Text.data() + Ofs);
}
inline uint64_t m64_load_unaligned(std::string_view Text, size_t Ofs = 0) noexcept {
    assert(Text.size() >= Ofs + 8);
    return m64_load_unaligned(Text.data() + Ofs);
}

constexpr inline uint64_t byte_mask(uint8_t b) noexcept { return uint64_t(b) * kOnes; }
constexpr inline uint64_t zero_byte_mask(uint64_t v) noexcept { return uint64_t((v - kOnes) & ~v) & kHigh; }
constexpr inline uint64_t lt_byte_mask(uint64_t x, uint8_t n) noexcept { return (x - byte_mask(n)) & kHigh; }
constexpr inline uint64_t gt_byte_mask(uint64_t x, uint8_t n) noexcept { return (byte_mask(n) - x) & kHigh; }
constexpr inline uint64_t eq_byte_mask(uint64_t x, uint8_t b) noexcept { return zero_byte_mask(x ^ byte_mask(b)); }

// Outside ['0'..'9']
constexpr inline uint64_t not_digit_mask(uint64_t x) noexcept {
    return (lt_byte_mask(x, '0') | gt_byte_mask(x, '9'));
}

// Non-space mask (high bit set where byte is NOT a space)
constexpr inline uint64_t not_space_mask(uint64_t x) noexcept {
#ifndef CPPON_TRUSTED_INPUT
    // JSON strict spaces: ' ', '\t', '\n', '\r'
    const uint64_t ws =
            (eq_byte_mask(x, static_cast<uint8_t>(' ')) |
            eq_byte_mask(x, static_cast<uint8_t>('\t')) |
            eq_byte_mask(x, static_cast<uint8_t>('\n')) |
            eq_byte_mask(x, static_cast<uint8_t>('\r')));
    return (~ws) & kHigh;
#else
    // Trusted input: whitespace 0x01..0x20 (non-zero controls). Test: (c-1) < 0x20 ⇒ space
    const uint64_t y = x - kOnes;                  // c - 1
    const uint64_t ws = gt_byte_mask(y, 0x1fu);    // high bit if (c-1) >= 0x20
    return ws & kHigh;                             // high bit if NOT space
#endif
}

constexpr inline bool has_zero_byte(uint64_t v) noexcept { return zero_byte_mask(v) != 0; }
constexpr inline bool has_byte_less_than(uint64_t x, int8_t n) noexcept { return lt_byte_mask(x, static_cast<uint8_t>(n)) != 0; }
constexpr inline bool has_byte_greater_than(uint64_t x, int8_t n) noexcept { return gt_byte_mask(x, static_cast<uint8_t>(n)) != 0; }
constexpr inline bool has_byte(uint64_t x, int8_t b) noexcept { return eq_byte_mask(x, static_cast<uint8_t>(b)) != 0; }

constexpr int m64_npos = 8;

inline auto m64_get_first_match(uint64_t Mask) {
    if (Mask == 0)
        return m64_npos;
    #if defined(_MSC_VER)
    unsigned long tz; _BitScanForward64(&tz, Mask); return static_cast<int>(tz >> 3);
    #else
    return static_cast<int>(static_cast<unsigned>(__builtin_ctzll(Mask)) >> 3);
    #endif
    }

/**
 * SWAR parallel comparison functions
 */
inline const char* m64_parallel_digits(std::string_view Text, size_t Ofs, size_t Count) noexcept {
    assert(Text.size() >= Ofs + Count);
    const char* p   = Text.data() + Ofs;
    const char* end = Text.data() + Ofs + Count;

    // Align
    //while (p <= end && (reinterpret_cast<uintptr_t>(p) & 7u)) {
    //    const unsigned char c = static_cast<unsigned char>(*p);
    //    if ((c - 0x30u) > 0x09u) return p;
    //    ++p;
    //}
    // Aligned Words
    while (p + 8 <= end) {
        uint64_t w = m64_load_unaligned(p);
        uint64_t mask = not_digit_mask(w);
        if (mask) {
            return p + m64_get_first_match(mask);
        }
        p += 8;
    }
    // Unaligned Tail
    while (p <= end) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if ((c - 0x30u) > 0x09u) return p;
        ++p;
    }
    return nullptr;
}
inline const char* m64_parallel_digits(std::string_view Text, size_t Ofs) noexcept {
    return m64_parallel_digits(Text, Ofs, Text.size() - Ofs);
}
inline const char* m64_parallel_digits(std::string_view Text) noexcept {
    return m64_parallel_digits(Text, 0, Text.size());
}

inline const char* m64_parallel_find_quote(std::string_view Text, size_t Ofs, size_t Count) noexcept {
	assert(Text.size() >= Ofs + Count);
    const char* p = Text.data() + Ofs;
    const char* end = Text.data() + Ofs + Count;

    // Align to 8 bytes
    //while (p < end && (reinterpret_cast<uintptr_t>(p) & 7u)) {
    //    if (*p == '"') return p;
    //    ++p;
    //}
    // Aaligned Words
    while (p + 8 <= end) {
        uint64_t w = m64_load_unaligned(p);
        uint64_t mask = eq_byte_mask(w, static_cast<uint8_t>('"'));
        if (mask) {
            return p + m64_get_first_match(mask);
        }
        p += 8;
    }
    // Unaligned Tail
    while (p < end) {
        if (*p == '"') return p;
        ++p;
    }
    return nullptr;
}
inline const char* m64_parallel_find_quote(std::string_view Text, size_t Ofs) noexcept {
    return m64_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
}
inline const char* m64_parallel_find_quote(std::string_view Text) noexcept {
    return m64_parallel_find_quote(Text, 0, Text.size());
}

inline const char* m64_parallel_skip_spaces(std::string_view Text, size_t Ofs, size_t Count) noexcept {
    assert(Text.size() >= Ofs + Count);
    const char* p = Text.data() + Ofs;
    const char* end = Text.data() + Ofs + Count;

    // Align
    //while (p < end && (reinterpret_cast<uintptr_t>(p) & 7u)) {
    //    const unsigned char c = static_cast<unsigned char>(*p);
    //    #ifndef CPPON_TRUSTED_INPUT
    //    if (c != 0x20 && c != '\t' && c != '\n' && c != '\r') return p;
    //    #else
    //    if ((c - 1u) >= 0x20) return p;
    //    #endif
    //    ++p;
    //}
    // Aligned Words
    while (p + 8 <= end) {
        uint64_t w = m64_load_unaligned(p);
        uint64_t mask = not_space_mask(w);
        if (mask) { // Not all spaces
            return p + m64_get_first_match(mask);
        }
        p += 8;
    }
    // Unaligned Tail
    while (p < end) {
        const unsigned char c = static_cast<unsigned char>(*p);
        #ifndef CPPON_TRUSTED_INPUT
        if (c != 0x20 && c != '\t' && c != '\n' && c != '\r') return p;
        #else
        if ((c - 1u) >= 0x20) return p;
        #endif
        ++p;
    }
    return nullptr;
}
inline const char* m64_parallel_skip_spaces(std::string_view Text, size_t Ofs) noexcept {
    return m64_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
}
inline const char* m64_parallel_skip_spaces(std::string_view Text) noexcept {
    return m64_parallel_skip_spaces(Text, 0, Text.size());
}

inline size_t m64_parallel_digits_pos(std::string_view Text, size_t Ofs, size_t Count) noexcept {
    assert(Text.size() >= Ofs + Count);
    const char* Found = m64_parallel_digits(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline size_t m64_parallel_digits_pos(std::string_view Text, size_t Ofs) noexcept {
    assert(Text.size() >= Ofs);
    const char* Found = m64_parallel_digits(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline size_t m64_parallel_digits_pos(std::string_view Text) noexcept {
    const char* Found = m64_parallel_digits(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

inline size_t m64_parallel_find_quote_pos(std::string_view Text, size_t Ofs, size_t Count) noexcept {
    assert(Text.size() >= Ofs + Count);
    const char* Found = m64_parallel_find_quote(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline size_t m64_parallel_find_quote_pos(std::string_view Text, size_t Ofs) noexcept {
    assert(Text.size() >= Ofs);
    const char* Found = m64_parallel_find_quote(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline size_t m64_parallel_find_quote_pos(std::string_view Text) noexcept {
    const char* Found = m64_parallel_find_quote(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

inline size_t m64_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs, size_t Count) noexcept {
    assert(Text.size() >= Ofs + Count);
    const char* Found = m64_parallel_skip_spaces(Text, Ofs, Count);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline size_t m64_parallel_skip_spaces_pos(std::string_view Text, size_t Ofs) noexcept {
    assert(Text.size() >= Ofs);
    const char* Found = m64_parallel_skip_spaces(Text, Ofs, Text.size() - Ofs);
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}
inline size_t m64_parallel_skip_spaces_pos(std::string_view Text) noexcept {
    const char* Found = m64_parallel_skip_spaces(Text, 0, Text.size());
    return Found ? static_cast<size_t>(Found - Text.data()) : Text.npos;
}

} // namespace ch5

#endif // CPPON_SWAR_H

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

namespace roots {
void push_root(const cppon& root);
void pop_root(const cppon& root) noexcept;
cppon& get_root() noexcept;
}//namespace roots
namespace visitors {
cppon& visitor(cppon& object, size_t index);
cppon& visitor(cppon& object, string_view_t index);
const cppon& visitor(const cppon& object, size_t index);
const cppon& visitor(const cppon& object, string_view_t index);
}//namespace visitors

class root_guard {
    cppon& new_root;
public:
    root_guard(const cppon& new_root_arg) noexcept
        : new_root(const_cast<cppon&>(new_root_arg)) {
        roots::push_root(new_root);
    }
    ~root_guard() noexcept {
        roots::pop_root(new_root);
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
    cppon() = default;
    cppon(const cppon&) = default;
    cppon(cppon&&) noexcept = default;
    ~cppon() noexcept { roots::pop_root(*this); }
    cppon& operator=(const cppon&) = default;
    cppon& operator=(cppon&&) noexcept = default;

    /**
    * @brief Verifies that a cppon object is valid and throws an exception if it is not
    *
    * Helper that checks if a cppon object is not in the valueless_by_exception state,
    * throwing an explicit exception if it is. This prevents undefined behavior
    * by transforming a potential UB into a detectable logic error.
    *
    * @throws object_reference_lost_error if the object is invalid
    */
    inline bool check_valid() const noexcept {
        if (valueless_by_exception()) {
            CPPON_ASSERT(!"Reference lost: Object is in valueless_by_exception state");
            return false;
        }
        return true;
    }
    inline void ensure_valid() const {
        if (!check_valid()) throw object_reference_lost_error{ "Object is in valueless_by_exception state" };
    }

    inline auto is_null() const -> bool {
        ensure_valid();
        return std::holds_alternative<nullptr_t>(*this);
    }

    inline auto operator[](string_view_t index)->cppon& {
        ensure_valid();
        CPPON_ASSERT(!index.empty() && "Index cannot be empty");
        if (index.front() == '/') {
            roots::push_root(*this);
            return visitors::visitor(roots::get_root(), index.substr(1));
        }
        return visitors::visitor(*this, index);
    }

    inline auto operator[](string_view_t index)const->const cppon& {
        ensure_valid();
        CPPON_ASSERT(!index.empty() && "Index cannot be empty");
        if (index.front() == '/') {
            roots::push_root(*this);
            return visitors::visitor(static_cast<const cppon&>(roots::get_root()), index.substr(1));
        }
        return visitors::visitor(*this, index);
    }

    inline auto operator[](size_t index)->cppon& {
        ensure_valid();
        return visitors::visitor(*this, index);
    }

    inline auto operator[](size_t index)const->const cppon& {
        ensure_valid();
        return visitors::visitor(*this, index);
    }

    inline auto& operator=(const char* val) {
        ensure_valid();
        value_t::operator=(string_view_t{ val });
        return *this;
    }

    inline auto& operator=(pointer_t pointer) {
        ensure_valid();
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
        typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
    inline auto& operator=(const T& val) {
        ensure_valid();
        value_t::operator=(val); return *this;
    }

    // Template for rvalue references, disabled for types not contained in value_t
    template<
        typename T,
        typename = std::enable_if_t<is_in_variant_rv<T, value_t>::value>>
    inline auto& operator=(T&& val) {
        ensure_valid();
        value_t::operator=(std::forward<T>(val)); return *this;
    }

    // Underlying container access (throws type_mismatch_error on wrong type)
    inline auto array() -> array_t& {
        ensure_valid();
        if (auto p = std::get_if<array_t>(this)) return *p;
        throw type_mismatch_error{ "array_t expected" };
    }
    inline auto array() const -> const array_t& {
        ensure_valid();
        if (auto p = std::get_if<array_t>(this)) return *p;
        throw type_mismatch_error{ "array_t expected" };
    }
    inline auto object() -> object_t& {
        ensure_valid();
        if (auto p = std::get_if<object_t>(this)) return *p;
        throw type_mismatch_error{ "object_t expected" };
    }
    inline auto object() const -> const object_t& {
        ensure_valid();
        if (auto p = std::get_if<object_t>(this)) return *p;
        throw type_mismatch_error{ "object_t expected" };
    }

    // Optional non-throw helpers
    inline auto try_array() noexcept -> array_t* { return check_valid() ? std::get_if<array_t>(this) : nullptr; }
    inline auto try_array() const noexcept -> const array_t* { return check_valid() ? std::get_if<array_t>(this) : nullptr; }
    inline auto try_object() noexcept -> object_t* { return check_valid() ? std::get_if<object_t>(this) : nullptr; }
    inline auto try_object() const noexcept -> const object_t* { return check_valid() ? std::get_if<object_t>(this) : nullptr; }

    // Array iteration
    inline auto begin() { return array().begin(); }
    inline auto end() { return array().end(); }
    inline auto begin() const{ return array().begin(); }
    inline auto end() const { return array().end(); }
};

template<
    typename T,
    typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
inline bool operator==(const cppon& lhs, const T& rhs) noexcept {
    if (lhs.valueless_by_exception())
        return false;

    // Vérifier si le type stocké correspond à T
    const T* val = std::get_if<T>(&lhs);
    if (!val)
        return false;

    // Comparer les valeurs
    return *val == rhs;
}
template<
    typename T,
    typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
inline bool operator==(const T& lhs, const cppon& rhs) noexcept {
    if (rhs.valueless_by_exception())
        return false;

    // Vérifier si le type stocké correspond à T
    const T* val = std::get_if<T>(&rhs);
    if (!val)
        return false;

    // Comparer les valeurs
    return lhs == *val;
}

template<
    typename T,
    typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
inline bool operator!=(const cppon& lhs, const T& rhs) noexcept {
    return !(lhs == rhs);
}
template<
    typename T,
    typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
inline bool operator!=(const T& lhs, const cppon& rhs) noexcept {
    return !(lhs == rhs);
}

#ifdef CPPON_ENABLE_STD_GET_INJECTION
using std::get_if;
using std::get;
#endif

}//namespace ch5

#endif //CPPON_TYPES_H

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-thread.h : C++ON thread local state and SIMD utilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_THREAD_H
#define CPPON_THREAD_H

#if CPPON_USE_SIMD
#endif

namespace ch5 {
enum class SimdLevel {
    None,
    SSE,
    AVX2,
    AVX512
};

#if CPPON_USE_SIMD
namespace scanner {
using find_quote_fn = size_t(*)(std::string_view, size_t);
using scan_digits_fn = const char* (*)(std::string_view, size_t);
inline void ensure_dispatch_bound() noexcept;
inline find_quote_fn bind_find_quote_pos(SimdLevel lvl) noexcept;
inline scan_digits_fn bind_parallel_digits(SimdLevel lvl) noexcept;
}
#endif

#if CPPON_USE_SIMD
namespace global {
inline static std::atomic<int> g_simd_override_global{ -1 };
}
#endif

namespace thread {

template<typename T>
struct config_var {
    cppon current;
    config_var(const T& v) : current{ v } {}
    operator T& () { return std::get<T>(current); }
    operator const T& () const { return std::get<T>(current); }
};

inline SimdLevel effective_simd_level() noexcept;

inline namespace storage {
inline static thread_local std::vector<cppon*> stack{ nullptr }; // stack first
inline static thread_local cppon null{ nullptr }; // null second
inline static thread_local printer_state::state printer;
#if CPPON_USE_SIMD
inline static thread_local SimdLevel max_simd_level = ([]() noexcept -> SimdLevel {
    platform::processor_features_info cpu_info;
    auto features = cpu_info.cpu_features();
    if (features.AVX512F) return SimdLevel::AVX512;
    if (features.AVX2)    return SimdLevel::AVX2;
    if (features.SSE4_2)  return SimdLevel::SSE;
    return SimdLevel::None;
    })();
inline static thread_local scanner::find_quote_fn p_find_quote = scanner::bind_find_quote_pos(max_simd_level);
inline static thread_local scanner::scan_digits_fn p_scan_digits = scanner::bind_parallel_digits(max_simd_level);
#else
inline static thread_local SimdLevel max_simd_level = SimdLevel::None;
#endif
inline static thread_local config_var<int64_t> object_min_reserve{ CPPON_OBJECT_MIN_RESERVE };
inline static thread_local config_var<int64_t> array_min_reserve{ CPPON_ARRAY_MIN_RESERVE };
inline static thread_local config_var<int64_t> printer_reserve_per_element{ CPPON_PRINTER_RESERVE_PER_ELEMENT };
inline static thread_local config_var<string_view_t> path_prefix{ CPPON_PATH_PREFIX };
inline static thread_local config_var<string_view_t> blob_prefix{ CPPON_BLOB_PREFIX };
inline static thread_local config_var<string_view_t> number_prefix{ CPPON_NUMBER_PREFIX };
inline static thread_local config_var<int64_t> simd_override_thread{ (int64_t)-1 };
inline static thread_local config_var<int64_t> simd_default{ (int64_t)effective_simd_level() };
inline static thread_local config_var<boolean_t> exact_number_mode{ false };
}

inline static auto& get_printer_state() noexcept {
    return printer;
}

#if CPPON_USE_SIMD
// Global process-wide override; -1 = none
inline SimdLevel cap_to_supported(SimdLevel lvl) noexcept {
    return (static_cast<int>(lvl) <= static_cast<int>(max_simd_level)) ? lvl : max_simd_level;
}
inline void set_simd_override_global(SimdLevel lvl) noexcept {
    global::g_simd_override_global.store(static_cast<int>(cap_to_supported(lvl)), std::memory_order_relaxed);
    scanner::ensure_dispatch_bound();
}
inline void clear_simd_override_global() noexcept {
    global::g_simd_override_global.store(-1, std::memory_order_relaxed);
    scanner::ensure_dispatch_bound();
}
inline void set_simd_override_thread(SimdLevel lvl) noexcept {
    simd_override_thread = static_cast<int>(cap_to_supported(lvl));
    scanner::ensure_dispatch_bound();
}
inline void clear_simd_override_thread() noexcept {
    simd_override_thread = -1;
    scanner::ensure_dispatch_bound();
}
inline bool has_simd_override() noexcept {
    return simd_override_thread >= 0 || global::g_simd_override_global.load(std::memory_order_relaxed) >= 0;
}
inline SimdLevel current_simd_override() noexcept {
    if (simd_override_thread >= 0)
        return static_cast<SimdLevel>(static_cast<int64_t>(simd_override_thread));
    int v = global::g_simd_override_global.load(std::memory_order_relaxed);
    return v < 0 ? SimdLevel::None : static_cast<SimdLevel>(v);
}
inline SimdLevel detect_simd_level() noexcept {
    if (has_simd_override()) { // Honor override first
        return cap_to_supported(current_simd_override());
    }
    return max_simd_level;
}

// Public export of SIMD override helpers at ch5 scope
inline void set_global_simd_override(SimdLevel lvl) noexcept {
    set_simd_override_global(lvl);
}
inline void clear_global_simd_override() noexcept {
    clear_simd_override_global();
}
inline void set_thread_simd_override(SimdLevel lvl) noexcept {
    set_simd_override_thread(lvl);
}
inline void clear_thread_simd_override() noexcept {
    clear_simd_override_thread();
}
inline SimdLevel effective_simd_level() noexcept {
    return detect_simd_level();
}
#else
// No-SIMD build: keep API stable with no-op stubs
inline void set_global_simd_override(SimdLevel) noexcept {}
inline void clear_global_simd_override() noexcept {}
inline void set_thread_simd_override(SimdLevel) noexcept {}
inline void clear_thread_simd_override() noexcept {}
inline SimdLevel effective_simd_level() noexcept { return SimdLevel::None; }
#endif

} // namespace thread
} // namespace ch5

#endif // CPPON_THREAD_H

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-references.h : Reference resolution and circular reference handling
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_REFERENCES_H
#define CPPON_REFERENCES_H

namespace ch5 {
namespace details {

using reference_vector_t = std::vector<std::tuple<string_view_t, pointer_t>>;
using pointer_map_t = std::unordered_map<string_view_t, pointer_t>;

/**
 * @brief Checks if an object is nested within a parent object.
 */
inline auto is_object_inside(cppon& parent, pointer_t object) -> bool {
    return std::visit(
        [&](auto&& vector) -> bool {
            using type = std::decay_t<decltype(vector)>;
            if constexpr (std::is_same_v<type, array_t>) {
                for (auto& value : vector)
                    if (std::get_if<pointer_t>(&value) && object == *std::get_if<pointer_t>(&value))
                        return true;
                for (auto& value : vector)
                    if (is_object_inside(value, object))
                        return true;
            } else if constexpr (std::is_same_v<type, object_t>) {
                for (auto& [name, value] : vector)
                    if (std::get_if<pointer_t>(&value) && object == *std::get_if<pointer_t>(&value))
                        return true;
                for (auto& [dummy, value] : vector)
                    if (is_object_inside(value, object))
                        return true;
            } else if constexpr (std::is_same_v<type, pointer_t>) {
                if (object == vector)
                    return true;
            }
            return false;
        },
        static_cast<value_t&>(parent));
}

/**
 * @brief Checks for cyclic references.
 */
inline auto is_pointer_cyclic(pointer_t pointer) -> bool {
    return is_object_inside(*pointer, pointer);
}

/**
 * @brief Recursively finds the path of an object.
 */
inline auto find_object_path(cppon& from, pointer_t object) -> std::string {
    std::string result;
    result.reserve(64);
    std::visit(
        [&](auto&& vector) -> string_view_t {
            using type = std::decay_t<decltype(vector)>;
            if constexpr (std::is_same_v<type, array_t>) {
                for (size_t i = 0; i < vector.size(); ++i) {
                    if (object == &vector[i]) {
                        result += "/";
                        result += std::to_string(i);
                        return result;
                    }
					const auto& path = find_object_path(vector[i], object);
					if (!path.empty()) {
						result += "/";
						result += std::to_string(i);
						result += path;
						return result;
					}
				}
            } else if constexpr (std::is_same_v<type, object_t>) {
                for (auto& [name, value] : vector) {
                    if (object == &value) {
                        result += "/";
                        result += name;
                        return result;
                    }
                }
                for (auto& [name, value] : vector) {
                    const auto& path = find_object_path(value, object);
                    if (!path.empty()) {
                        result += "/";
                        result += name;
                        result += path;
                        return result;
                    }
                }
            }
            return result;
        },
        static_cast<value_t&>(from));
    return result;
}

/**
 * @brief Gets the path associated with a pointer.
 */
inline auto get_object_path(reference_vector_t& refs, pointer_t ptr) -> path_t {
    for (auto& [path, obj_ptr] : refs) {
        if (*std::get_if<pointer_t>(obj_ptr) == ptr)
            return path;
    }
    CPPON_ASSERT(!"The given pointer has no associated path in reference_vector_t.");
    throw std::runtime_error("The given pointer has no associated path in reference_vector_t.");
}

/**
 * @brief Finds and stores object references based on paths.
 */
inline void find_objects(cppon& object, pointer_map_t& objects, reference_vector_t& refs) {
    for (auto& [reference_path, path_source] : refs) {
        string_view_t path{ reference_path };
        pointer_t target_object = &object;
        size_t offset = 1;
        do {
            auto next = path.find('/', offset);
            auto segment = path.substr(offset, next - offset);
            auto& immutable = std::as_const(*target_object);
            target_object = const_cast<cppon*>(&immutable[segment]);
            if (target_object->is_null()) {
                path_source = nullptr;
                break;
            }
            if (next == string_view_t::npos) {
                objects.emplace(path, target_object);
                break;
            }
            offset = next + 1;
        } while (true);
    }
}

/**
 * @brief Identifies and records references in a cppon structure.
 */
inline void find_references(cppon& object, reference_vector_t& refs) {
    std::visit(
        [&](auto&& node) {
            using type = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<type, path_t>) {
                refs.emplace_back(node, &object); // std::tuple(node, &object)
            } else if constexpr (std::is_same_v<type, array_t>) {
                for (auto& sub_object : node)
                    find_references(sub_object, refs);
            } else if constexpr (std::is_same_v<type, object_t>) {
                for (auto& [name, sub_object] : node) {
                    find_references(sub_object, refs);
                }
            }
        },
        static_cast<value_t&>(object));
}

/**
 * @brief Establishes connections between referenced and target objects.
 */
inline auto resolve_paths(cppon& object) -> reference_vector_t {
    reference_vector_t references;
    pointer_map_t objects;
    references.reserve(16);
    objects.reserve(16);
    find_references(object, references);
    find_objects(object, objects, references);
    for (auto& [var, ptr] : references) {
        if (ptr)
            ptr->emplace<pointer_t>(objects[var]);
    }
    return references;
}

/**
 * @brief Reverts the linking process by replacing object pointers with their original reference strings.
 */
inline void restore_paths(const reference_vector_t& references) {
    for (auto& [var, ptr] : references) {
        ptr->emplace<path_t>(var);
    }
}

} // namespace details

using details::reference_vector_t;
using details::is_object_inside;
using details::is_pointer_cyclic;
using details::find_object_path;
using details::get_object_path;
using details::resolve_paths;
using details::restore_paths;

} // namespace ch5

#endif // CPPON_REFERENCES_H

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-scanner.h : C++ON Scanner dispatch and utilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_SCANNER_H
#define CPPON_SCANNER_H

#if CPPON_USE_SIMD
    namespace ch5 {
    namespace scanner {
        // Dynamic detection of SIMD capabilities

        // Lazily bound function pointer dispatch
        using find_quote_fn = size_t(*)(std::string_view, size_t);
        using scan_digits_fn = const char* (*)(std::string_view, size_t);

        inline find_quote_fn bind_find_quote_pos(SimdLevel lvl) noexcept {
            switch (lvl) {
            case SimdLevel::AVX512: return simd::zmm_parallel_find_quote_pos;
            case SimdLevel::AVX2:   return simd::ymm_parallel_find_quote_pos;
            case SimdLevel::SSE:    return simd::xmm_parallel_find_quote_pos;
            default:                return m64_parallel_find_quote_pos;
            }
        }
        inline scan_digits_fn bind_parallel_digits(SimdLevel lvl) noexcept {
            switch (lvl) {
            case SimdLevel::AVX512: return simd::zmm_parallel_digits;
            case SimdLevel::AVX2:   return simd::ymm_parallel_digits;
            case SimdLevel::SSE:    return simd::xmm_parallel_digits;
            default:                return m64_parallel_digits;
            }
        }

        inline void bind_dispatch(SimdLevel lvl) noexcept {
            thread::p_find_quote = bind_find_quote_pos(lvl);
            thread::p_scan_digits = bind_parallel_digits(lvl);
        }


        inline void ensure_dispatch_bound() noexcept {
            bind_dispatch(thread::detect_simd_level());
        }

        // Hot-path APIs
        inline size_t find_quote_pos(std::string_view text, size_t start = 0) noexcept {
            return thread::p_find_quote(text, start);
        }

        inline const char* scan_digits(std::string_view text, size_t start = 0) noexcept {
            return thread::p_scan_digits(text, start);
        }
    }
    }
#else
    // Implementations without SIMD
    namespace ch5 {
    namespace scanner {

        // Scalar implementation (fallback)
        inline size_t find_quote_pos(std::string_view text, size_t start = 0) noexcept {
            return m64_parallel_find_quote_pos(text, start);
        }

        inline const char* scan_digits(std::string_view text, size_t start = 0) noexcept {
            return m64_parallel_digits(text, start);
        }
    }
    }
#endif

#endif // CPPON_SCANNER_H

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

namespace ch5 {

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
        if (value.rfind(thread::path_prefix, 0) == 0)
		    return cppon{ path_t{ value.substr(std::size<string_view_t>(thread::path_prefix)) } };

        // If the blob prefix is found, return a blob object
        if (value.rfind(thread::blob_prefix, 0) == 0) {
            if (opt == options::full)
                // If the full option is set, return a blob object with the decoded value
			    return cppon{ decode_base64(value.substr(std::size<string_view_t>(thread::blob_prefix))) };
		    else
                // Otherwise, return a blob object with the encoded value
			    return cppon{ blob_string_t{ value.substr(std::size<string_view_t>(thread::blob_prefix)) } };
        }

        if (value.rfind(thread::number_prefix, 0) == 0) {
            std::array<string_view_t, 11> types{
                "int64", "double", "float", "int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64"
            };
            auto TypedValue{ value.substr(std::size<string_view_t>(thread::number_prefix)) };
            auto lPar = TypedValue.find('(');
            if (lPar == TypedValue.npos) throw expected_symbol_error{ "(", found };
            auto rPar = TypedValue.rfind(')');
            if (rPar == TypedValue.npos) throw expected_symbol_error{ ")", found };
            auto Type = TypedValue.substr(0, lPar);
            if (Type.empty()) throw expected_symbol_error{ "type", peek + lPar };
            auto found = std::find(types.begin(), types.end(), Type);
            if (found == types.end()) throw expected_symbol_error{ "type", peek + lPar };
            auto Number = TypedValue.substr(lPar + 1, rPar - lPar - 1);
            if (Number.empty()) throw expected_symbol_error{ "number", peek + lPar + 1 };
			NumberType index = static_cast<NumberType>(std::distance(types.begin(), found));
            switch (index) {
                case NumberType::json_int64: return cppon{ number_t{ Number, NumberType::json_int64 } };
				case NumberType::json_double: return cppon{ number_t{ Number, NumberType::json_double } };
				case NumberType::cpp_float: return cppon{ number_t{ Number, NumberType::cpp_float } };
				case NumberType::cpp_int8: return cppon{ number_t{ Number, NumberType::cpp_int8 } };
				case NumberType::cpp_uint8: return cppon{ number_t{ Number, NumberType::cpp_uint8 } };
				case NumberType::cpp_int16: return cppon{ number_t{ Number, NumberType::cpp_int16 } };
                case NumberType::cpp_uint16: return cppon{ number_t{ Number, NumberType::cpp_uint16 } };
                case NumberType::cpp_int32: return cppon{ number_t{ Number, NumberType::cpp_int32 } };
                case NumberType::cpp_uint32: return cppon{ number_t{ Number, NumberType::cpp_uint32 } };
                case NumberType::cpp_int64: return cppon{ number_t{ Number, NumberType::cpp_int64 } };
                case NumberType::cpp_uint64: return cppon{ number_t{ Number, NumberType::cpp_uint64 } };
                default: throw type_mismatch_error{ "Invalid number type" };
            }
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

    //if (!Printer.Exact &&
    if (!thread::exact_number_mode && opt < options::quick) // options:{full,eval}
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
        array.reserve(thread::array_min_reserve);
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
        object.reserve(thread::object_min_reserve);
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

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-printer.h : JSON generation and formatting capabilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_PRINTER_H
#define CPPON_PRINTER_H

namespace ch5 {
cppon printer_state::state::to_cppon() const {
	cppon Options;
	auto& buffer = Options["buffer"];
	auto& layout = Options["layout"];

	buffer["retain"] = RetainBuffer;
	buffer["reserve"] = Reserve;

	layout["exact"] = Exact;
	layout["json"] = Compatible;
	layout["flatten"] = Flatten;
	layout["compact"] = !Pretty;

	if (Pretty) layout["pretty"] = AltLayout;
	if (Margin != 0) layout["margin"] = Margin;
	if (Tabs != 2) layout["tabulation"] = Tabs;
	if (!Pretty) layout["compact"] = true;
	if (Compacted.empty()) layout["compact"] = !Pretty;
	else {
		size_t index = 0;
		auto& compact = layout["compact"];
		for (auto& Label : Compacted)
			compact[index++] = Label;
	}
	return Options;
}

namespace details {

// These limits represent the largest integers that can be precisely represented
// in IEEE-754 double-precision floating point, which is what JSON uses for numbers
constexpr int64_t json_min_limit = -9007199254740991;
constexpr int64_t json_max_limit = 9007199254740991;
typedef std::unordered_set<std::string_view> string_set_t;

struct printer;
inline auto apply_options(const cppon& Options) {
	string_set_t CompactedList{};
	int Alternative = -1;
	int Compacted = -1;
	int Compacting = -1;
	int Compatible = -1;
	int Exact = -1;
	int Flattening = -1;
	int Reserving = -1;
	int Reseting = -1;
	int Retaining = -1;
	int Margin = -1;
	int Tabulation = -1;

	// "buffer" : "reset" | "retain" | "noreserve" | | "reserve" | {"reset|retain"|"reserve" : false|true}
	// - reset:      reset the buffer before printing
	// - retain:     retain the buffer after printing
	// - noreserve:  do not reserve memory for printing the JSON representation
	// - reserve:    reserve memory for printing the JSON representation
	auto visit_buffer = [&](const value_t& Buffer) {
		std::visit([&](auto&& Opt) {
			using type = std::decay_t<decltype(Opt)>;
			if constexpr (std::is_same_v<type, nullptr_t>)
				return; // no buffer option specified
			else if constexpr (std::is_same_v<type, object_t>) {
				for (auto& [Name, Value] : Opt) {
					const auto& Label{ Name };
					std::visit([&](auto&& Val) {
						using type = std::decay_t<decltype(Val)>;
						if constexpr (std::is_same_v<type, boolean_t>) {
							if (Label == "reset") Reseting = Val;
							else if (Label == "retain") Retaining = Val;
							else if (Label == "reserve") Reserving = Val;
							else throw bad_option_error("buffer: invalid option");
						}
						else throw bad_option_error("buffer: type mismatch");
						}, static_cast<const value_t&>(Value));
				}
			}
			else if constexpr (std::is_same_v<type, string_view_t>) {
				if (Opt == "reset") Reseting = static_cast<int>(true);
				else if (Opt == "retain") Retaining = static_cast<int>(true);
				else if (Opt == "noreserve") Reserving = static_cast<int>(false);
				else if (Opt == "reserve") Reserving = static_cast<int>(true);
				else throw bad_option_error("buffer: invalid option");
			}
			else throw bad_option_error("buffer: type mismatch");
			}, Buffer);
		};

	// "compact" : false | true | ["label", ...]
	// - false:          do not compact any object
	// - true:           compact all objects (print without indentation, space or newline)
	// - ["label", ...]: compact objects with the specified labels, [] to reset list
	auto visit_compact = [&](const value_t& Compact) {
		std::visit([&](auto&& Opt) {
			using type = std::decay_t<decltype(Opt)>;
			if constexpr (std::is_same_v<type, nullptr_t>)
				return; // no compacting option specified
			else if constexpr (std::is_same_v<type, boolean_t>) Compacting = static_cast<int>(Opt);
			else if constexpr (std::is_same_v<type, array_t>) {
				for (auto& Element : Opt) {
					std::visit([&](auto&& Label) {
						using type = std::decay_t<decltype(Label)>;
						if constexpr (std::is_same_v<type, string_view_t>) CompactedList.emplace(Label);
						else throw bad_option_error("compact: array type mismatch");
						}, static_cast<const value_t&>(Element));
				}
				Compacted = !CompactedList.empty();
			}
			else throw bad_option_error("compact: type mismatch");
			}, Compact);
		};

	// "pretty" : false | true
	// - false: uses default layout for pretty printing
	// - true:  uses alternative layout for pretty printing
	auto visit_pretty = [&](const value_t& Compact) {
		std::visit([&](auto&& Opt) {
			using type = std::decay_t<decltype(Opt)>;
			if constexpr (std::is_same_v<type, nullptr_t>)
				return; // no compacting option specified
			else if constexpr (std::is_same_v<type, boolean_t>) Alternative = static_cast<int>(Opt);
			else throw bad_option_error("pretty: type mismatch");
			}, Compact);
		};

	// "margin" : value
	// - value: set the margin to the specified value
	auto visit_margin = [&](const value_t& MarginVal) {
		std::visit([&](auto&& Opt) {
			using type = std::decay_t<decltype(Opt)>;
			if constexpr (std::is_same_v<type, nullptr_t>)
				return; // no margin option specified
			else if constexpr (std::is_arithmetic_v<type>) Margin = static_cast<int>(Opt);
			else throw bad_option_error("margin: type mismatch");
			}, MarginVal);
		};

	// "tabulation" : value | [value, ...]
	// - value:        set the tabulation to the specified value
	// - [value, ...]: set the tabulations to the specified values
	auto visit_tabulation = [&](const value_t& TabulationVal) {
		std::visit([&](auto&& Opt) {
			using type = std::decay_t<decltype(Opt)>;
			if constexpr (std::is_same_v<type, nullptr_t>)
				return; // no tabulation option specified
			else if constexpr (std::is_arithmetic_v<type>) Tabulation = static_cast<int>(Opt);
			else throw bad_option_error("tabulation: type mismatch");
			}, TabulationVal);
		};

	// "layout" : "flatten" | "json" | "cppon" | {"flatten"|"json"|"cppon"|"pretty" : false|true , "compact" : compaction}
	// - flatten: flatten all objects (resolve all references)
	// - json: print the JSON representation of the object (compatible with JSON)
	// - cppon: print the CPPON representation of the object (default, not compatible with JSON)
	// - {"flatten"|"json" : false|true , "compact" : compaction}
	// --- flatten: flatten all objects (resolve all references)
	// --- json:    print the JSON representation of the object
	// --- cppon:   print the CPPON representation of the object
	// --- pretty:  uses alternative layout for pretty printing
	// --- compact: compact all or specified objects
	auto visit_layout = [&](const value_t& Layout) {
		std::visit([&](auto&& Opt) {
			using type = std::decay_t<decltype(Opt)>;
			if constexpr (std::is_same_v<type, nullptr_t>)
				return; // no layout option specified
			else if constexpr (std::is_same_v<type, object_t>) {
				for (auto& [Name, Value] : Opt) {
					const auto& Label{ Name };
					if (Label == "compact") {
						visit_compact(Value);
					}
					else {
						std::visit([&](auto&& Val) {
							using type = std::decay_t<decltype(Val)>;
							if constexpr (std::is_same_v<type, boolean_t>) {
								if (Label == "flatten") Flattening = Val;
								else if (Label == "json") Compatible = Val;
								else if (Label == "cppon") Compatible = !Val;
								else if (Label == "exact") Exact = Val;
								else if (Label == "pretty") Alternative = Val;
								else throw bad_option_error("layout: invalid option");
							}
							else if constexpr (std::is_arithmetic_v<type>) {
								if (Label == "margin") Margin = static_cast<int>(Val);
								else if (Label == "tabulation") Tabulation = static_cast<int>(Val);
								else throw bad_option_error("layout: invalid option");
							}
							else throw bad_option_error("layout: type mismatch");
							}, static_cast<const value_t&>(Value));
					}
				}
			}
			else if constexpr (std::is_same_v<type, string_view_t>) {
				if (Opt == "flatten") Flattening = true;
				else if (Opt == "json") Compatible = true;
				else if (Opt == "cppon") Compatible = false;
				else if (Opt == "exact") Exact = true;
				else throw bad_option_error("layout: invalid option");
			}
			else throw bad_option_error("layout: type mismatch");
			}, Layout);
		};

	visit_buffer(Options["buffer"]);
	visit_layout(Options["layout"]);
	visit_compact(Options["compact"]);
	visit_pretty(Options["pretty"]);
	visit_margin(Options["margin"]);
	visit_tabulation(Options["tabulation"]);

	return std::make_tuple(CompactedList, Alternative, Compacted, Compacting,
		Compatible, Exact, Flattening, Reserving, Reseting,
		Retaining, Margin, Tabulation);
}
inline void write_options(printer& Printer, const cppon& Options);

/**
 * @brief The printer struct is responsible for printing the JSON representation of cppon objects.
 */
struct printer : public printer_state::state
{
	reference_vector_t* Refs{};   /**< The object map that stores the references to objects. */
	string_view_t Compacting;          /**< The compacting string that stores the current compacting object. */

	void swap(printer_state::state& State) noexcept {
		std::swap(State.Out, Out);
		std::swap(State.Compacted, Compacted);
		std::swap(State.Level, Level);
		std::swap(State.Tabs, Tabs);
		std::swap(State.Margin, Margin);
		std::swap(State.Reserve, Reserve);
		std::swap(State.Flatten, Flatten);
		std::swap(State.Pretty, Pretty);
		std::swap(State.AltLayout, AltLayout);
		std::swap(State.Compatible, Compatible);
		std::swap(State.Exact, Exact);
		std::swap(State.RetainBuffer, RetainBuffer);
	}

	void configure(const cppon& Options, reference_vector_t* References = nullptr) {
		Refs = References;
		write_options(*this, Options);
	};

	/**
	 * @brief Returns the number of characters printed so far.
	 * @return The number of characters printed.
	 */
	size_t printed_count() const noexcept {
		return Out.size();
		}

	/**
	 * @brief Preallocates memory for printing the JSON representation.
	 * @param ElementCount The number of elements to be printed.
	 */
	void preallocate(size_t ElementCount) {
		if (Reserve) {
			Out.reserve(Out.size() + thread::printer_reserve_per_element * ElementCount + 2);
			}
		}

	/**
	 * @brief Adjusts the preallocated memory based on the actual size of the printed array.
	 * @param ReservePerElement The preallocated memory per element.
	 * @param StartSize The starting size of the printed array.
	 * @param CurrentElementCount The current number of printed elements.
	 * @param ElementCount The total number of elements to be printed.
	 */
	void preallocate(size_t& ReservePerElement, size_t StartSize, size_t CurrentElementCount, size_t ElementCount) {
		if (Reserve) {
			auto ArraySize{ Out.size() - StartSize };
			if (ArraySize > ReservePerElement * CurrentElementCount) {
				// bad guess, adjust with mean progression
				ReservePerElement = (ReservePerElement + ArraySize) / CurrentElementCount;
				size_t AdjustedReserve = ReservePerElement / 2;
				Out.reserve(Out.size() + AdjustedReserve * (ElementCount - CurrentElementCount) + 2);
				}
			}
		}

	/**
	 * @brief Prints a single character.
	 * @param Char The character to be printed.
	 */
	void print(char Char) {
		Out.push_back(Char);
		}

	/**
	 * @brief Prints a string.
	 * @param Text The string to be printed.
	 */
	void print(string_view_t Text) {
		Out.append(Text);
		}

	/**
	 * @brief Prints a character array.
	 * @param Text The character array to be printed.
	 * @param Length The length of the character array.
	 */
	void print(const char* Text, size_t Length) {
		Out.append(Text, Length);
		}

	/**
	 * @brief Resets the printer to its initial state.
	 */
	void reset() noexcept {
		Level = 0;
		Tabs = 2;
		Reserve = true;
		Flatten = false;
		Pretty = false;
		AltLayout = false;
		Compatible = false;
		std::exchange(Out, {});
		std::exchange(Compacting, {});
		std::exchange(Compacted, {});
		}

	int level_to_margin(int Level) const noexcept {
		return Level * Tabs;
		}

	/**
	 * @brief Resets the margin of the printed JSON representation.
	 * @param CurrentSize The current size of the printed JSON representation.
	 */
	void reset_margin() {
		if (Pretty) {
			Out.append(size_t(Level * Tabs), ' ');
			}
		}

	/**
	 * @brief Resets the set of compacted objects.
	 */
	void reset_compacted() noexcept {
		std::exchange(Compacted, {});
		}

    /**
     * @brief Merges the given set of compacted objects into the current set of compacted objects.
     * @param CompactedList The set of compacted objects to be merged.
     */
    void merge_compacted(const string_set_t& CompactedList) {
        Compacted.insert(CompactedList.begin(), CompactedList.end());
    }

	/**
	 * @brief Appends a label to the set of compacted objects.
	 * @param Label The label to be appended.
	 */
	void append_compacted(string_view_t Label) {
		Compacted.emplace(Label);
		}

	/**
	 * @brief Pushes the current compacting string to the stack.
	 * @param Stack The stack to store the current compacting string.
	 */
	void push(string_view_t& Stack) {
		if (Pretty) {
			Stack = std::exchange(Compacting, {});
			}
		}

	/**
	 * @brief Pops the top compacting string from the stack.
	 * @param Stack The stack to retrieve the top compacting string.
	 */
	void pop(string_view_t& Stack) {
		if (Pretty) {
			Compacting = Stack;
			}
		}

	/**
	 * @brief Pushes the current compacting string to the stack and tags compacted objects.
	 * @param Stack The stack to store the current compacting string.
	 * @param Member The member to be tagged as a compacted object.
	 */
	void push(string_view_t& Stack, string_view_t Member) {
		if (Pretty) {
			// with pretty space
			Out.push_back(' ');
			// tag compacted objects
			if (Compacting.empty() && Compacted.find(Member) != Compacted.end()) {
				Stack = std::exchange(Compacting, Member);
				}
			}
		}

	/**
	 * @brief Pops the top compacting string from the stack and tags compacted objects.
	 * @param Stack The stack to retrieve the top compacting string.
	 * @param Member The member to be tagged as a compacted object.
	 */
	void pop(string_view_t& Stack, string_view_t Member) {
		if (Pretty) {
			// tag compacted objects
			if (Compacting == Member) {
				Compacting = Stack;
				}
			}
		}

	/**
	 * @brief Prints a newline character.
	 */
	void newline() {
		if (Pretty) {
			if (Compacting.empty()) {
				if (Reserve) {
					Out.reserve(Out.size() + size_t(Level * Tabs + 1));
					}
				Out.push_back('\n');
				Out.append(size_t(Level * Tabs), ' ');
				}
			else {
				Out.push_back(' ');
				}
			}
		}

	/**
	 * @brief Enters a new indentation level.
	 */
	void enter() {
		if (Pretty) {
			++Level;
			newline();
			}
		}

	/**
	 * @brief Exits the current indentation level.
	 */
	void exit() {
		if (Pretty) {
			Level -= static_cast<int>(!AltLayout);
			newline();
			Level -= static_cast<int>(AltLayout);
			}
		}

	/**
	 * @brief Prints a comma character.
	 */
	void next() {
		Out.push_back(',');
		if (Pretty) {
			newline();
			}
		}
};


inline void write_options(printer& Printer, const cppon& Options) {
	details::string_set_t CompactedList{};
	int Alternative = -1;
	int Compacted = -1;
	int Compacting = -1;
	int Compatible = -1;
	int Exact = -1;
	int Flattening = -1;
	int Reserving = -1;
	int Reseting = -1;
	int Retaining = -1;
	int Margin = -1;
	int Tabulation = -1;
	auto ResetPrinter = false;
	if (!Options.is_null()) {
		std::tie(CompactedList, Alternative, Compacted, Compacting,
			Compatible, Exact, Flattening, Reserving, Reseting,
			Retaining, Margin, Tabulation) = details::apply_options(Options);
	}

	if (Reseting == 1 && Retaining == 1) {
		throw bad_option_error("buffer: cannot reset and retain the buffer at the same time");
	}
	else if (Retaining != -1) {
		// set retain flag
		Printer.RetainBuffer = static_cast<bool>(Retaining);
	}
	else if (Reseting == 1) {
		// enable resetting
		ResetPrinter = true;
	}

	// apply reset if set
	if (ResetPrinter) {
		Printer.reset();
	}

	// clear buffer if not retained
	if (!Printer.RetainBuffer) {
		Printer.Out.clear();
	}

	// enable memory reservation
	if (Reserving != -1) {
		Printer.Reserve = static_cast<bool>(Reserving);
	}

	// compacting
	if (Compacting == 1 && Compacted == 1) {
		throw bad_option_error("compact: cannot compact all and compact some at the same time");
	}
	else if (Compacting != -1) {
		Printer.Pretty = !static_cast<bool>(Compacting);
	}
	else if (Compacted != -1) {
		if (Compacted) {
			Printer.merge_compacted(CompactedList);
		}
		else {
			Printer.reset_compacted();
		}
	}

	// pretty
	if (Alternative != -1) {
		Printer.Pretty = true;
		Printer.AltLayout = static_cast<bool>(Alternative);
	}
	// margin
	if (Margin != -1) {
		Printer.Pretty = true;
		Printer.Margin = Margin; // TODO: must set a valid range
	}
	// tabulation
	if (Tabulation != -1) {
		Printer.Pretty = true;
		Printer.Tabs = Tabulation;  // TODO: must set a valid range
	}
	// compatibility
	if (Compatible != -1) {
		Printer.Compatible = static_cast<bool>(Compatible);
	}
	// compatibility
	if (Exact != -1) {
		Printer.Exact = static_cast<bool>(Exact);
	}
	else {
		Printer.Exact = static_cast<bool>(thread::exact_number_mode);
	}

	// reference flattening
	if (Flattening != -1) {
		Printer.Flatten = static_cast<bool>(Flattening);
	}

	//margin
	if (Printer.Margin) {
		Printer.Level = Printer.Margin;
		Printer.reset_margin();
	}
}

inline cppon configure_printer(const cppon& Options, bool get_previous = false) {
	auto& state = thread::get_printer_state();
	cppon previous = get_previous ? state.to_cppon() : cppon{};
	printer Printer;
	Printer.swap(state);
	Printer.configure(Options);
	Printer.swap(state);
	return previous;
}

inline cppon configure_printer(string_view_t OptionsJson, bool get_previous = false) {
	if (OptionsJson.empty()) throw bad_option_error{"empty options"};
	return configure_printer(eval(OptionsJson), get_previous);
}

/**
 * @brief Formats and prints floating-point numbers with appropriate precision.
 *
 * This function handles various edge cases in floating-point representation:
 * - Ensures proper decimal point representation (adds .0 when needed)
 * - Handles exponential notation correctly
 * - Adds type suffix (f) for float values in non-compatible mode
 *
 * @param Printer The printer object used for output
 * @param Buffer Character buffer for formatting
 * @param BufferSize Size of the buffer
 * @param Num The floating-point value to print
 * @param isDouble Whether the value is a double (true) or float (false)
 */
inline size_t print_floats(printer& Printer, char* Buffer, size_t BufferSize, const double_t Num, bool isDouble) {
	auto Len = snprintf(Buffer, BufferSize, isDouble ? "%.16lg" : "%.7g", Num);
	char* Dot = strchr(Buffer, '.');
	char* Exp = strpbrk(Buffer, "eE");
	if (Dot) {
		if (Exp && Exp == Dot + 1) {
			memmove(Dot, Dot + 1, strlen(Dot));
			Len--;
			}
		else if (!Exp && Dot == &Buffer[Len - 1]) {
			Buffer[Len++] = '0';
			}
		}
	else if (!Exp) {
		Buffer[Len++] = '.';
		Buffer[Len++] = '0';
		}
	if (!(Printer.Compatible || isDouble)) Buffer[Len++] = 'f';
	Buffer[Len] = '\0';
	return Len;
	}

/**
 * @brief Garde RAII pour configuration temporaire du thread printer
 *
 * Permet de modifier la configuration du thread printer pour un scope
 * et de la restaurer automatiquement en sortant du scope.
 */
class printer_guard {
	printer saved_printer;
public:
	printer_guard(const cppon& Options) {
		saved_printer.swap(thread::get_printer_state());
		configure_printer(Options);
	}

	~printer_guard() {
		saved_printer.swap(thread::get_printer_state());
	}
};

}//namespace details

using details::printer;
using details::printer_guard;
using details::configure_printer;

/**
 * @brief Overloads of the print function for various data types.
 *
 * These overloads facilitate the printing of different data types using the specified printer object.
 * They handle the conversion of numeric types to string representations and append specific suffixes
 * to indicate the data type in the printed format, except in compatible mode where a strict JSON representation
 * is required.
 *
 * - Integer types (int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t) are converted to int or unsigned int
 *   before being passed to std::to_string. Suffixes such as "i8", "u8", "i16", etc., are appended to explicitly
 *   indicate the integer type, except in compatible mode.
 *
 * - The int64_t and uint64_t types are specially treated to check JSON limits in compatible mode. They are
 *   converted to long long or unsigned long long, respectively, before being printed. A 'u' suffix is added
 *   for uint64_t in non-compatible mode. In compatible mode, an exception is thrown if the value is outside
 *   the range that can be precisely represented in JSON (-9007199254740991 to 9007199254740991 for int64_t and up to
 *   9007199254740991 for uint64_t), ensuring accurate data representation.
 *
 * - Floating-point types (float_t and double_t) are printed with specified precision and may receive a 'f'
 *   suffix for float_t in non-compatible mode. The print_floats function handles the correct representation
 *   of floating-point numbers, including the correction of decimal and exponential notations.
 *
 * - The number_t type (previously known as string_number_t) is printed as is, without enclosing quotation marks,
 *   distinguishing it from the string_view_t type.
 *
 * - The string_view_t type is printed with enclosing quotation marks to clearly identify it as a string.
 *
 * - Boolean types are printed as 'true' or 'false' without quotation marks.
 *
 * - Null values are printed as 'null' without quotation marks.
 *
 * - Arrays and objects are recursively printed, adhering to the specified formatting options such as pretty printing,
 *   compacting, and compatibility mode.
 *
 * These overloads ensure uniform and flexible printing of data in various formats while adhering to JSON
 * representation requirements in compatible mode. They also handle exceptions for 64-bit numbers to maintain
 * precision and compatibility.
 *
 * @exception json_compatibility_error Thrown when a value is out of range for JSON representation.
 */
inline void print(printer& Printer, const cppon& Object); // forward declaration for array and object types
inline void print(printer& Printer, const nullptr_t) {
	Printer.print("null", 4);
	}
inline void print(printer& Printer, const boolean_t Bool) {
	constexpr string_view_t Boolean[]{ "false","true" };
	Printer.print(Boolean[Bool]);
	}
inline void print(printer& Printer, const float_t Num) {
	char Buffer[24];
	size_t Len = details::print_floats(Printer, Buffer, sizeof(Buffer), static_cast<const double_t>(Num), false);
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::string_view{ Buffer, Len }, NumberType::cpp_float } });
		return;
		}
	Printer.print(Buffer, Len);
	}
inline void print(printer& Printer, const double_t Num) {
	char Buffer[48];
	size_t Len = details::print_floats(Printer, Buffer, sizeof(Buffer), static_cast<const double_t>(Num), true);
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::string_view{ Buffer, Len }, NumberType::json_double } });
		return;
		}
	Printer.print(Buffer, Len);
	}
inline void print(printer& Printer, const int8_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<int>(Num)), NumberType::cpp_int8 } });
		return;
		}
	Printer.print(std::to_string(static_cast<int>(Num)));
	if (!Printer.Compatible) Printer.print("i8");
	}
inline void print(printer& Printer, const uint8_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<unsigned int>(Num)), NumberType::cpp_uint8 } });
		return;
		}
    Printer.print(std::to_string(static_cast<unsigned int>(Num)));
    if (!Printer.Compatible) Printer.print("u8");
	}
inline void print(printer& Printer, const int16_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<int>(Num)), NumberType::cpp_int16 } });
		return;
		}
    Printer.print(std::to_string(static_cast<int>(Num)));
    if (!Printer.Compatible) Printer.print("i16");
	}
inline void print(printer& Printer, const uint16_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<unsigned int>(Num)), NumberType::cpp_uint16 } });
		return;
		}
    Printer.print(std::to_string(static_cast<unsigned int>(Num)));
    if (!Printer.Compatible) Printer.print("u16");
	}
inline void print(printer& Printer, const int32_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<int>(Num)), NumberType::cpp_int32 } });
		return;
		}
    Printer.print(std::to_string(static_cast<int>(Num)));
    if (!Printer.Compatible) Printer.print("i32");
	}
inline void print(printer& Printer, const uint32_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<unsigned int>(Num)), NumberType::cpp_uint32 } });
		return;
		}
    Printer.print(std::to_string(static_cast<unsigned int>(Num)));
    if (!Printer.Compatible) Printer.print("u32");
	}
inline void print(printer& Printer, const int64_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<long long>(Num)), NumberType::cpp_int64 } });
		return;
		}
	if (Printer.Compatible && (Num < details::json_min_limit || Num > details::json_max_limit)) {
		throw json_compatibility_error("Value out of range for JSON.");
		}
	Printer.print(std::to_string(static_cast<long long>(Num)));
	}
inline void print(printer& Printer, const uint64_t Num) {
	if (Printer.Exact) {
		print(Printer, cppon{ number_t{ std::to_string(static_cast<long long>(Num)), NumberType::cpp_uint64 } });
		return;
		}
	if (Printer.Compatible && Num > details::json_max_limit) {
		throw json_compatibility_error("Value out of range for JSON.");
		}
	Printer.print(std::to_string(static_cast<unsigned long long>(Num)));
	if (!Printer.Compatible) Printer.print('u');
	}
inline void print(printer& Printer, const number_t TextualNumber) {
	if (Printer.Compatible || Printer.Exact) {
		std::array<string_view_t, 11> types{
			"int64", "double", "float", "int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64"
			};
		if (Printer.Compatible)
			switch (TextualNumber.type) {
				case NumberType::json_int64:
				case NumberType::json_double:
					Printer.print(TextualNumber);
					return;
				default:
					break; // fall through
				}
		Printer.print('"');
		Printer.print(thread::number_prefix);
		Printer.print(types[static_cast<size_t>(TextualNumber.type)]);
		Printer.print('(');
		Printer.print(TextualNumber);
		Printer.print(')');
		Printer.print('"');
		}
	else {
		Printer.print(TextualNumber);
		}
	}
inline void print(printer& Printer, const path_t& Path) {
	Printer.print('"');
    Printer.print(thread::path_prefix);
    Printer.print(Path);
	Printer.print('"');
	}
inline void print(printer& Printer, const blob_string_t& Blob) {
	Printer.print('"');
    Printer.print(thread::blob_prefix);
    Printer.print(Blob);
	Printer.print('"');
	}
inline void print(printer& Printer, const string_view_t Text) {
	Printer.print('"');
	Printer.print(Text);
	Printer.print('"');
	}
inline void print(printer& Printer, const array_t& Array) {
	bool once = true;
	string_view_t Stack;

	// preallocate storage
	size_t StructureSize = 2; // '[' and ']'
	size_t ReservePerElement = thread::printer_reserve_per_element; // guess
	size_t StartSize = Printer.printed_count();
	size_t ElementCount = Array.size();
	size_t CurrentElementCount = 0;

	Printer.preallocate(ElementCount);

	Printer.print('[');
	if (!Array.empty())
		Printer.enter();

	for (auto&& Element : Array) {
		if (once) once = false;
		else Printer.next();

		// visit element
		Printer.push(Stack);
		print(Printer, Element);
		Printer.pop(Stack);

		++CurrentElementCount;
		Printer.preallocate(ReservePerElement, StartSize, CurrentElementCount, ElementCount);
		}

	if (!Array.empty())
		Printer.exit();
	Printer.print(']');
	}
inline void print(printer& Printer, const object_t& Object) {
	bool once = true;
	string_view_t Stack;

	// preallocate storage
	size_t StructureSize = 2; // '{' and '}' or '"' and '"'
	size_t ReservePerElement = thread::printer_reserve_per_element; // guess
	size_t StartSize = Printer.printed_count();
	size_t ElementCount = Object.size();
	size_t CurrentElementCount = 0;

	Printer.preallocate(ElementCount);

	Printer.print('{');
	if (!Object.empty())
		Printer.enter();

	for (auto& [MemberName, MemberValue] : Object) {
		if (once) once = false;
		else Printer.next();

		// member name
		print(Printer, MemberName);

		// colon separator
		Printer.print(':');

		// visit value
		Printer.push(Stack, MemberName);
		print(Printer, MemberValue);// , ObjectMap);
		Printer.pop(Stack, MemberName);

		++CurrentElementCount;
		Printer.preallocate(ReservePerElement, StartSize, CurrentElementCount, ElementCount);
		}

	if (!Object.empty())
		Printer.exit();
	Printer.print('}');
	}
inline void print(printer& Printer, const pointer_t& Pointer) {
    if (!Pointer) {
        Printer.print("null", 4);
        return;
		}
	if (!Printer.Flatten || is_pointer_cyclic(Pointer)) {
		if (Printer.Refs)
			print(Printer, path_t{ get_object_path(*Printer.Refs, Pointer) });
		else
			print(Printer, path_t{ find_object_path(roots::get_root(), Pointer) });
		}
	else {
		print(Printer, *Pointer);
		}
	}
inline void print(printer& Printer, const blob_t& Blob) {
	print(Printer, blob_string_t{ encode_base64(Blob) });
	}
inline void print(printer& Printer, const cppon& Value) {
	std::visit([&](auto&& Val) {
		print(Printer, Val);
		}, static_cast<const value_t&>(Value));
	}
inline auto operator<<(printer& Printer, const cppon& Value) -> printer& {
	print(Printer, Value);
	return Printer;
}
inline auto operator<<(std::ostream& Stream, const cppon& Value) -> std::ostream& {
	auto& State = thread::get_printer_state();
	printer Printer;
	Printer.swap(State);
	print(Printer, Value);
	Stream.write(Printer.Out.data(), static_cast<std::streamsize>(Printer.Out.size()));
	Printer.swap(State);
    return Stream;
}

inline auto to_string_view(const cppon& Object, reference_vector_t* Refs, const cppon& Options) -> std::string_view {
	auto& State = thread::get_printer_state();
	printer Printer;

	// swap printer state
	Printer.swap(State);

	Printer.configure(Options, Refs);
	print(Printer, Object);

	// swap back printer state
	Printer.swap(State);

	// return the printed JSON representation
	return std::string_view{ State.Out };
	}
inline auto to_string_view(const cppon& Object, reference_vector_t* Refs, string_view_t Options = {}) {
	cppon Opt; if (!Options.empty()) Opt = eval(Options);
	return to_string_view(Object, Refs, Opt);
	}
inline auto to_string_view(const cppon& Object, const cppon& Options) {
	return to_string_view(Object, nullptr, Options);
	}
inline auto to_string_view(const cppon& Object, string_view_t Options = {}) {
	cppon Opt; if (!Options.empty()) Opt = eval(Options);
	return to_string_view(Object, nullptr, Opt);
	}
inline auto to_string(const cppon& Object, reference_vector_t* Refs, const cppon& Options) -> std::string {
	return  std::string{ to_string_view(Object, Refs, Options) };
}
inline auto to_string(const cppon& Object, reference_vector_t* Refs, string_view_t Options = {}) {
	cppon Opt; if (!Options.empty()) Opt = eval(Options);
	return std::string{ to_string_view(Object, Refs, Opt) };
	}
inline auto to_string(const cppon& Object, const cppon& Options) {
	return std::string{ to_string_view(Object, nullptr, Options) };
	}
inline auto to_string(const cppon& Object, string_view_t Options = {}) {
	cppon Opt; if (!Options.empty()) Opt = eval(Options);
	return std::string{ to_string_view(Object, nullptr, Opt) };
	}

}//namespace ch5

#endif // CPPON_PRINTER_H
/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-literals.h : C++ON
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_ROOTS_H
#define CPPON_ROOTS_H

namespace ch5 {

inline cppon& null() noexcept {
	CPPON_ASSERT(thread::null.is_null());
	return thread::null;
}

namespace roots {

/**
 * @brief Central utilities for managing cppon objects and their interconnections.
 *
 * This suite of utility functions is foundational to the cppon framework. It manages a per-thread root
 * context via a stack, exposes a per-thread null sentinel, and supports dereferencing of path- and pointer-
 * based references consistently across traversals.
 *
 * Sentinel and invariants:
 * - The root stack is constructed with a bottom sentinel (nullptr) to avoid repeated stack.empty() checks.
 *   This sentinel is never dereferenced. Access to the current root goes through get_root(), which asserts
 *   that the top entry is non-null.
 * - Uniqueness: each pushed root must be unique within the stack (no duplicates). In Debug, push_root() asserts this.
 * - LIFO discipline: pop_root() only pops if the given object is at the top; otherwise it is a no-op (harmless temporaries).
 *
 * - `null()`: Provides a per-thread singleton instance representing a null value, ensuring consistent
 *   representation of null or undefined states across the cppon object hierarchy.
 *
 * - Root stack and accessors:
 *   - `root_stack()`: Access to the thread-local stack that holds `cppon*` roots.
 *   - `get_root()`: Returns a reference to the current root. Asserts the stack is never empty and the
 *     top is never nullptr (thus the sentinel is never accessed).
 *   - `push_root(const cppon&)` and `pop_root(const cppon&)`: Push the given object if not already on top,
 *     and pop it only if it is the current top, respectively. Guarantees balanced stack usage in scoped code.
 *
 * - Absolute paths and guards:
 *   - Absolute-path access (string indices starting with '/') uses `push_root(*this)` and resolves the
 *     remainder of the path against `get_root()`.
 *   - `root_guard` is the RAII helper to scope a root change safely (push on construction, pop on destruction).
 *
 * @note `deref_if_ptr` resolves `pointer_t` directly and `path_t` against the current root. Assertions enforce
 *       that `get_root()` is only used when the top is non-null.
 *
 * Collectively, these utilities underscore the cppon framework's flexibility, robustness, and capability to represent
 * and manage complex data structures and relationships. They are instrumental in ensuring that cppon objects can be
 * dynamically linked, accessed, and manipulated within a coherent and stable framework.
 */

// Notes:
// - The stack is initialized with a bottom sentinel (nullptr). This avoids empty() checks.
// - The sentinel is never dereferenced because get_root() asserts top != nullptr.

inline std::vector<cppon*>& root_stack() noexcept {
	return thread::stack;
}
inline cppon& get_root() noexcept {
	CPPON_ASSERT(!root_stack().empty() && root_stack().back() != nullptr &&
		"Root stack shall never be empty and stack top shall never be nullptr");
	return *root_stack().back();
}
// Promote the entry to top if found; returns true if present (already top or promoted)
inline bool hoist_if_found(std::vector<cppon*>& s, const cppon* p) noexcept {
	if (s.back() == p) return true;
	auto found = std::find(s.rbegin(), s.rend(), p);
	if (found == s.rend()) return false;
	std::swap(*found, s.back());
	return true;
}
inline void pop_root(const cppon& root) noexcept {
	auto& stack = root_stack();
	if (hoist_if_found(stack, &root))
		stack.pop_back();
}
inline void push_root(const cppon& root) {
	auto& stack = root_stack();
	if (!hoist_if_found(stack, &root))
		stack.push_back(&const_cast<cppon&>(root));
}

} // namespace roots
} // namespace ch5


#endif // CPPON_ROOTS_H

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-visitors.h : Object traversal and manipulation utilities
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_VISITORS_H
#define CPPON_VISITORS_H

namespace ch5 {
namespace visitors {

constexpr size_t max_array_delta = CPPON_MAX_ARRAY_DELTA;
constexpr size_t object_min_reserve = CPPON_OBJECT_MIN_RESERVE;
constexpr size_t array_min_reserve = CPPON_ARRAY_MIN_RESERVE;

struct key_t {
	std::string_view value;
	explicit key_t(std::string_view v) : value(v) {}
	operator std::string_view() const { return value; }
};

/**
 * @brief Central utilities for managing cppon objects and their interconnections.
 *
 * This suite of utility functions is foundational to the cppon framework. It manages a per-thread root
 * context via a stack, exposes a per-thread null sentinel, and supports dereferencing of path- and pointer-
 * based references consistently across traversals.
 *
 * This module provides:
 * - A thread_local static_storage that holds:
 *   - a std::vector<cppon*> root stack (used with stack semantics),
 *   - a singleton cppon Null used as a null sentinel.
 *
 * Sentinel and invariants:
 * - The root stack is constructed with a bottom sentinel (nullptr) to avoid repeated stack.empty() checks.
 *   This sentinel is never dereferenced. Access to the current root goes through get_root(), which asserts
 *   that the top entry is non-null.
 * - Uniqueness: each pushed root must be unique within the stack (no duplicates). In Debug, push_root() asserts this.
 * - LIFO discipline: pop_root() only pops if the given object is at the top; otherwise it is a no-op (harmless temporaries).
 *
 * - `null()`: Provides a per-thread singleton instance representing a null value, ensuring consistent
 *   representation of null or undefined states across the cppon object hierarchy.
 *
 * - Root stack and accessors:
 *   - `root_stack()`: Access to the thread-local stack that holds `cppon*` roots.
 *   - `get_root()`: Returns a reference to the current root. Asserts the stack is never empty and the
 *     top is never nullptr (thus the sentinel is never accessed).
 *   - `push_root(const cppon&)` and `pop_root(const cppon&)`: Push the given object if not already on top,
 *     and pop it only if it is the current top, respectively. Guarantees balanced stack usage in scoped code.
 *
 * - Absolute paths and guards:
 *   - Absolute-path access (string indices starting with '/') uses `push_root(*this)` and resolves the
 *     remainder of the path against `get_root()`.
 *   - `root_guard` is the RAII helper to scope a root change safely (push on construction, pop on destruction).
 *
 * @note `deref_if_ptr` resolves `pointer_t` directly and `path_t` against the current root. Assertions enforce
 *       that `get_root()` is only used when the top is non-null.
 *
 * Collectively, these utilities underscore the cppon framework's flexibility, robustness, and capability to represent
 * and manage complex data structures and relationships. They are instrumental in ensuring that cppon objects can be
 * dynamically linked, accessed, and manipulated within a coherent and stable framework.
 */

// Return true if every character is '0'..'9' (empty => false for path segments)
inline bool all_digits(string_view_t sv) noexcept {
	if (sv.empty()) return false;
	return std::all_of(sv.begin(), sv.end(), [](unsigned char c) {
		unsigned d = static_cast<unsigned>(c) - static_cast<unsigned>('0');
		return d <= 9u;
	});
}

// Bounded helper to convert a numeric string_view into size_t
inline size_t parse_index(string_view_t sv) noexcept {
	size_t idx = 0;
	for (char c : sv) {
		idx = idx * 10 + static_cast<size_t>(c - '0');
	}
	return idx;
}

/**
 * @brief Internal dereference helpers for pointer_t and path_t.
 *
 * deref_if_ptr:
 * - pointer_t: returns *arg (or the thread-local null sentinel if the pointer is null).
 * - path_t: resolves against the current root (get_root()); the leading '/' is removed unconditionally.
 * - other alternatives: returns the original object reference.
 *
 * deref_if_not_null (non-const only):
 * - If the slot holds a null pointer_t, returns the slot itself (enables autovivification at the slot).
 * - Otherwise delegates to deref_if_ptr.
 *
 * Notes
 * - These helpers are internal (namespace ch5::visitors). Public traversal uses visitor(...) and cppon::operator[].
 * - On path_t resolution, visitor(...) may throw: member_not_found_error, type_mismatch_error, null_value_error, etc.
 */
inline auto deref_if_ptr(const cppon& obj) -> const cppon& {
	return std::visit([&](auto&& arg) -> const cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, pointer_t>)
			return arg ? *arg : null();
		if constexpr (std::is_same_v<type, path_t>) {
			string_view_t tmp = arg.value;
			tmp.remove_prefix(1);
			return visitor(static_cast<const cppon&>(roots::get_root()), tmp);
		}
		return obj;
	}, static_cast<const value_t&>(obj));
}
inline auto deref_if_ptr(cppon& obj) -> cppon& {
	return std::visit([&](auto&& arg) -> cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, pointer_t>)
			return arg ? *arg : null();
		if constexpr (std::is_same_v<type, path_t>) {
			string_view_t tmp = arg.value;
			tmp.remove_prefix(1);
			return visitor(roots::get_root(), tmp);
		}
		return obj;
	}, static_cast<value_t&>(obj));
}

/**
 * @brief Selects the write reference for a non-leaf path segment.
 *
 * Behavior:
 * - If the slot currently holds a null pointer_t, returns the slot itself (autovivification at the slot).
 * - Otherwise returns the dereferenced target via deref_if_ptr (pointer_t/path_t/value).
 *
 * Exceptions:
 * - May propagate exceptions from deref_if_ptr -> visitor(get_root(), tmp) when resolving path_t
 *   (e.g., member_not_found_error, type_mismatch_error, null_value_error, ...).
 * - A null pointer_t path does not throw here.
 */
inline cppon& deref_if_not_null(cppon& slot) {
	if (auto p = std::get_if<pointer_t>(&slot); p && !*p)
		return slot;
	return deref_if_ptr(slot);
}

inline cppon& vivify(cppon& slot, string_view_t key);

/**
 * @brief Visitor functions for accessing and potentially modifying members in an object using a key.
 *
 * These specializations of the visitor function serve a dual purpose within the cppon framework. They allow for direct access
 * to members within an object by their name key. The non-const version additionally supports dynamically resizing the object
 * to accommodate new members, facilitating the mutation of the object structure.
 *
 * The const version of the function provides read-only access to the members within a constant object, ensuring that the operation does not
 * modify the object. It checks if the requested key is present inside the object and returns a reference to the member's value at that
 * location. If the key is not found, a null object is returned, indicating that the value does not exist at the specified key.
 *
 * Exceptions:
 * - anything that std::vector::emplace_back might throw (e.g., std::bad_alloc).
 */
inline auto visitor(const object_t& object, key_t key) noexcept -> const cppon& {
	for (auto& [name, value] : object)
		if (name == key) return value;
	return null();
}
inline auto visitor(object_t& object, key_t key) -> cppon& {
	for (auto& [name, value] : object)
		if (name == key) return value;
	if (object.empty()) object.reserve(object_min_reserve);
	object.emplace_back(key, null());
	return std::get<cppon>(object.back());
}

/**
 * @brief Visitor functions for accessing and potentially modifying elements in an array using a numeric index.
 *
 * These specializations of the visitor function serve a dual purpose within the cppon framework. They allow for direct access
 * to elements within an array by their numerical index. The non-const version additionally supports dynamically resizing the array
 * to accommodate indices beyond the current bounds, facilitating the mutation of the array structure. This dynamic resizing is
 * constrained by a permissible range, defined by the current size plus a max_array_delta threshold, to prevent excessive expansion.
 *
 * The const version of the function provides read-only access to elements within a constant array, ensuring that the operation does not
 * modify the array. It checks if the requested index is within the bounds of the array and returns a reference to the element at that
 * index. If the index is out of bounds, a null object is returned, indicating that the element does not exist at the specified index.
 *
 * Note that the non-const version is the source of `excessive_array_resize_error` if the requested index exceeds the permissible range
 * (current size + max_array_delta).
 *
 * Exceptions:
 * - excessive_array_resize_error: The requested index exceeds the permissible range (current size + max_array_delta) in the non-const version.
 * - anything that std::vector::resize might throw (e.g., std::bad_alloc).
 */
inline const cppon& visitor(const array_t& array, size_t index) noexcept {
	if (index >= array.size()) {
		return null();
	}
	return array[index];
}
inline cppon& visitor(array_t& array, size_t index) {
	if (index >= array.size() ) {
		const size_t max_allowed = array.size() + max_array_delta;
		if (index > max_allowed)
			throw excessive_array_resize_error();
		if (array.empty()) array.reserve(std::max(array_min_reserve, index));
		array.resize(index + 1, null());
	}
	return array[index];
}

/**
 * @brief Visitor functions for accessing and potentially modifying elements in an array using a string index.
 *
 * The first path segment must be a numeric index. If not, bad_array_index_error is thrown.
 * If more segments follow, traversal recurses into the child (arrays/objects only).
 *
 * Exceptions:
 * - bad_array_index_error: the immediate segment is not numeric.
 * - null_value_error (const variant): next segment requested but current value is null.
 * - type_mismatch_error: non-terminal segment is not array/object.
 */
inline auto visitor(const array_t& array, string_view_t index) -> const cppon& {
	auto next{ index.find('/') };
	auto digits{ index.substr(0, next) }; // digits are numbers
	if (!all_digits(digits))
		throw bad_array_index_error(digits);
	auto& element = visitor(array, parse_index(digits));
	if (next == index.npos) return element; // if there is no next segment, it's a value
	auto& value = deref_if_ptr(element); // if there is a next segment, it's a path
	if (value.is_null()) throw null_value_error(); // if the value is null, throw an error
    return visitor(value, index.substr(next + 1));
}
inline auto visitor(array_t& array, string_view_t index) -> cppon& {
	auto next{ index.find('/') };
	auto digits{ index.substr(0, next) }; // digits are numbers
	if (!all_digits(digits))
		throw bad_array_index_error(digits);
	auto& element = visitor(array, parse_index(digits));
	if (next == index.npos) return element; // if there is no next segment, it's a value
	auto& value = deref_if_not_null(element); // if there is a next segment, it's a path
	auto newIndex{ index.substr(next + 1) };
	auto nextKey{ newIndex.substr(0, newIndex.find('/')) };
	CPPON_ASSERT(!nextKey.empty() && "Next key shall never be empty here");
    return vivify(value, newIndex);
}

/**
 * @brief Visitor functions for accessing and potentially modifying members in an object using a string key.
 *
 * The first path segment can be any name key.
 * If more segments follow, traversal recurses into the child (arrays/objects only).
 *
 * Exceptions:
 * - member_not_found_error (const variant): next segment requested but current value is null.
 * - type_mismatch_error: non-terminal segment is not array/object.
 */
inline const cppon& visitor(const object_t& object, string_view_t index) {
	auto next{ index.find('/') };
	auto key = index.substr(0, next); // key is a name
	auto& element = visitor(object, key_t{ key });
	if (next == index.npos) return element; // if there is no next segment, it's a value
	auto& value = deref_if_ptr(element); // if there is a next segment, it's a path
	if (value.is_null())
		throw member_not_found_error{}; // if the value not defined, throw an error
	return visitor(value, index.substr(next + 1));
}
inline cppon& visitor(object_t& object, string_view_t index) {
	auto next{ index.find('/') };
	auto key = index.substr(0, next); // key is a name
	auto& element = visitor(object,  key_t{ key });
	if (next == index.npos) return element; // no next segment, return value reference
	auto& value = deref_if_not_null(element); // if there is a next segment, it's a path
	auto newIndex{ index.substr(next + 1) };
	auto nextKey{ newIndex.substr(0, newIndex.find('/')) };
	CPPON_ASSERT(!nextKey.empty() && "Next key shall never be empty here");
    return vivify(value, newIndex);
}

/**
 * @brief Visitor functions for accessing and potentially modifying elements within a cppon object by a numerical index.
 *
 * These functions facilitate direct access to elements within a cppon object, which is expected to be an array,
 * using a numerical index. They are designed to handle both const and non-const contexts, allowing for read-only or
 * modifiable access to the elements, respectively.
 *
 * In the non-const version, if the specified index exceeds the current size of the array, the array is automatically resized
 * to accommodate the new element at the given index. This resizing is subject to a maximum increment limit defined by max_array_delta to prevent
 * excessive and potentially harmful allocation. If the required increment to accommodate the new index exceeds this limit, a bad_array_index_error exception is thrown,
 * signaling an attempt to access or create an element at an index that would require an excessively large increase in the array size.
 *
 * In both versions, if the cppon object is not an array (indicating a misuse of the type), a type_mismatch_error exception is
 * thrown. This ensures that the functions are used correctly according to the structure of the cppon object and maintains type safety.
 *
 * The const version of these functions provides read-only access to the elements, ensuring that the operation does not modify the
 * object being accessed. It allows for dynamic dereferencing of objects within a const context, supporting scenarios where the
 * cppon object structure is accessed in a read-only manner.
 *
 * Key Points:
 * - Direct access to elements by numerical index.
 * - Automatic resizing of the array in the non-const version to accommodate new elements, with a safeguard increment limit (max_array_delta).
 * - Throws excessive_array_resize_error if the required increment to reach the specified index exceeds the permissible range.
 * - Throws type_mismatch_error if the cppon object is not an array.
 * - Supports both const and non-const contexts, enabling flexible data manipulation and access patterns.
 *
 * @param object The cppon object to visit. This object must be an array for index-based access to be valid.
 * @param index The numerical index to access the element. In the non-const version, if the index is greater than the size of the array, the array is resized according to the increment limit.
 * @return (const) cppon& A reference to the visited element in the array. The non-const version allows for modification of the element.
 * @throws excessive_array_resize_error If the required increment to accommodate the specified index exceeds the maximum limit allowed by max_array_delta.
 * @throws type_mismatch_error If the cppon object is not an array, indicating a type error.
 */
inline const cppon& visitor(const cppon& object, size_t index) {
    return std::visit([&](auto&& arg) -> const cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, array_t>)
			return visitor(arg, index);
		throw type_mismatch_error{};
    }, static_cast<const value_t&>(object));
}
inline cppon& visitor(cppon& object, size_t index) {
    return std::visit([&](auto&& arg) -> cppon& {
		using type = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<type, array_t>)
			return visitor(arg, index);
        if constexpr (std::is_same_v<type, nullptr_t>) {
            // autovivify the root as an object if it is null
            object = cppon{ array_t{} };
            auto& arr = std::get<array_t>(object);
            arr.reserve(array_min_reserve);
            return visitor(arr, index);
        }
        throw type_mismatch_error{};
    }, static_cast<value_t&>(object));
}

/**
 * @brief Dynamically accesses elements within a cppon object using a string index, with support for both const and non-const contexts.
 *
 * This function allows dynamic access to elements within a cppon object, which can be either an array or an object,
 * using a string index. The string index can represent either a direct key in an object or a numeric index in an array.
 *
 * Additionally, it supports hierarchical access to nested elements by interpreting the string index as a path, where segments
 * of the path are separated by slashes ('/'). This allows deep access to nested structures, with the non-const version
 * allowing modification of elements.
 *
 * The function first determines the type of the cppon object (array or object) based on its current state. It then attempts
 * to access the element specified by the index or path. If the element is within a nested structure, the function recursively
 * navigates through the structure to reach the desired element. In the non-const version, any missing elements along the path
 * are created as either objects or arrays, depending on the segment type.
 *
 * Exceptions:
 * - `type_mismatch_error` is thrown if the object is not an array or an object when expected.
 * - In the const version:
 *   - If the key is a path, `member_not_found_error` or `type_mismatch_error` may be thrown depending on the path.
 *   - For array segments, `bad_array_index_error` is thrown if the segment is not numeric.
 *   - When reaching the leaf of the path in an object_t and the member does not exist, it returns the null object, avoiding exceptions for non-existent members in read-only scenarios.
 *   - If the index is out of bounds for the array, it returns the null object.
 * - In the non-const version:
 *   - If the path exists and the next segment is numeric for object_t, it throws `type_mismatch_error`.
 *   - For array segments, `bad_array_index_error` is thrown if the segment is not numeric.
 *   - If the index is out of bounds for the array and requires excessive resizing, it throws `excessive_array_resize_error`.
 *
 * This ensures that the caller is informed of incorrect access attempts or structural issues within the cppon object.
 *
 * The const version of this function provides the same functionality but ensures that the operation does not modify the object
 * being dereferenced. It allows dynamic dereferencing of objects in a const context, supporting scenarios where the cppon object
 * structure is accessed in a read-only manner.
 *
 * @param object The cppon object to visit. This object can be an array or an object, and may contain nested structures.
 *
 * @param index A string representing the index or path to the element to access. This can specify direct access or hierarchical access to nested elements.
 *              If index is empty, the function returns the object itself.
 *
 * @return (const) cppon& A reference to the cppon element at the specified index or path, with the non-const version allowing modification of the element.
 *
 * @throws type_mismatch_error If the cppon object does not match the expected structure for the specified index or path, indicating a type mismatch or invalid access attempt.
 * @throws member_not_found_error In the specified scenarios, indicating that a specified member in the path does not exist.
 * @throws invalid_path_segment_error In the specified scenarios, indicating that a path segment is invalid or out of bounds.
 * @throws excessive_array_resize_error In the specified scenarios, indicating that excessive array resizing is required.
 */
inline const cppon& visitor(const cppon& object, string_view_t index) {
	if (index.empty()) return object;
    return std::visit([&](auto&& arg) -> const cppon& {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, object_t>)
            return visitor(arg, index);
        if constexpr (std::is_same_v<type, array_t>)
            return visitor(arg, index);
        throw type_mismatch_error{};
    }, static_cast<const value_t&>(object));
}
inline cppon& visitor(cppon& object, string_view_t index) {
	if (index.empty()) return object;
    return std::visit([&](auto&& arg) -> cppon& {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, object_t>)
            return visitor(arg, index);
        if constexpr (std::is_same_v<type, array_t>)
            return visitor(arg, index);
        if constexpr (std::is_same_v<type, nullptr_t>) {
            // autovivify the root as an object if it is null
            return vivify(object, index);
        }
        throw type_mismatch_error{};
    }, static_cast<value_t&>(object));
}

inline cppon& vivify(cppon& slot, string_view_t key) {
    if (all_digits(key.substr(0, key.find('/')))) {
        // next key is a number
        if (slot.try_object()) throw type_mismatch_error{};
        if (slot.try_array() == nullptr) slot = cppon{ array_t{} };
        auto& arr = std::get<array_t>(slot);
        arr.reserve(array_min_reserve);
        return visitor(arr, key);
    }
    else {
        // next key is a string
        if (slot.try_array()) throw type_mismatch_error{};
        if (slot.try_object() == nullptr) slot = cppon{ object_t{} };
        auto& obj = std::get<object_t>(slot);
        obj.reserve(object_min_reserve);
        return visitor(obj, key);
    }

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
            return get_blob(deref_if_ptr(value));
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
            return get_blob(deref_if_ptr(value));
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
    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, pointer_t> || std::is_same_v<T, number_t>, "T must be a numeric type");
    return std::visit([&](auto&& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_strict<T>(deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_strict<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            if (thread::exact_number_mode) {
                auto copy = value;
                convert_to_numeric(copy);
                return get_strict<T>(copy);
            }
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
    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, pointer_t> || std::is_same_v<T, number_t>, "T must be a numeric type");
    return std::visit([&](const auto& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_strict<T>(deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_strict<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            if (thread::exact_number_mode) {
                auto copy = value;
                convert_to_numeric(copy);
                return get_strict<T>(copy);
            }
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
    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, pointer_t> || std::is_same_v<T, number_t>, "T must be a numeric type");
    return std::visit([&](auto&& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_cast<T>(deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_cast<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            if (thread::exact_number_mode) {
                auto copy = value;
                convert_to_numeric(copy);
                return get_cast<T>(copy);
            }
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
    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, pointer_t> || std::is_same_v<T, number_t>, "T must be a numeric type");
    return std::visit([&](auto&& arg) -> T {
        using type = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<type, path_t>) {
            return get_cast<T>(deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_cast<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            if (thread::exact_number_mode) {
                auto copy = value;
                convert_to_numeric(copy);
                return get_cast<T>(copy);
            }
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
            return get_optional<T>(deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_optional<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            if (thread::exact_number_mode) {
                auto copy = value;
                convert_to_numeric(copy);
                return get_optional<T>(copy);
            }
            convert_to_numeric(value);
            return get_optional<T>(value);
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
            return get_optional<T>(deref_if_ptr(value));
        }
        else if constexpr (std::is_same_v<type, pointer_t>) {
            return get_optional<T>(*arg);
        }
        else if constexpr (std::is_same_v<type, number_t>) {
            if (thread::exact_number_mode) {
                auto copy = value;
                convert_to_numeric(copy);
                return get_optional<T>(copy);
            }
            throw number_not_converted_error{};
        }
        else {
            return std::get_if<T>(&value);
        }
        }, value);
}

} // namespace visitors

using visitors::get_blob;
using visitors::get_strict;
using visitors::get_cast;
using visitors::get_optional;

} // namespace ch5

#endif // CPPON_VISITORS_H

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

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-document.h : CPPON document wrapper
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_DOCUMENT_H
#define CPPON_DOCUMENT_H

namespace ch5 {

/*
 * C++ON - document wrapper
 * Owns the input buffer so that all string_view_t / blob_string_t
 * produced by parsing remain valid for the lifetime of the document.
 */
class document : public cppon {
    std::string _buffer;

    void eval_and_assign(const options parse_mode) {
        static_cast<cppon&>(*this) = ch5::eval(_buffer, parse_mode);
        roots::push_root(*this);
    }

public:
    document() = default;

    explicit document(nullptr_t) : cppon{ nullptr } {}

    // Construct + eval (copies text)
    explicit document(const char* text, const options opt = Quick) {
        eval(text, opt);
    }

    // Construct + eval (take ownership)
    explicit document(std::string_view text, const options opt = Quick) {
        eval(text, opt);
    }

    // Construct + eval (take ownership)
    explicit document(std::string&& text, const options opt = Quick) {
        eval(std::move(text), opt);
    }

    // Non-copyable
    document(const document&)            = delete;
    document& operator=(const document&) = delete;

    // Movable
    document(document&& other) noexcept = default;
    document& operator=(document&& other) noexcept = default;

    // Access to underlying source
    bool empty() const noexcept { return _buffer.empty(); }
    std::string_view source_view() const noexcept { return _buffer; }
    const std::string& source() const noexcept { return _buffer; }

    document& set_source(std::string_view text) {
        _buffer.assign(text.begin(), text.end());
        return *this;
    }
    document& set_source(std::string&& text) noexcept {
        _buffer = std::move(text);
        return *this;
    }

    document& eval(const char* text, const options parse_mode = Quick) {
        set_source(std::string_view{ text });
		eval_and_assign(parse_mode);
        return *this;
    }
    document& eval(std::string_view text, const options parse_mode = Quick) {
        set_source(text);
        eval_and_assign(parse_mode);
        return *this;
    }
    document& eval(std::string&& text, const options parse_mode = Quick) {
        set_source(std::move(text));
        eval_and_assign(parse_mode);
        return *this;
    }

    // Reset everything (buffer + DOM)
    document& clear() noexcept {
        _buffer.clear();
        static_cast<cppon&>(*this) = cppon{}; // empty object_t
        return *this;
    }

    std::string serialize(const cppon& print_options = cppon{}) const {
        return to_string(*this, print_options);
	}

    void to_file(const std::string& filename, const cppon& print_options = cppon{}) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) throw file_operation_error(filename, "open");

        auto json = serialize(print_options);
        out.write(json.data(), static_cast<std::streamsize>(json.size()));

        if (!out) throw file_operation_error(filename, "write to");
    }

    bool to_file(const std::string& filename, std::string& error, const cppon& print_options = cppon{}) const noexcept {
        try {
            to_file(filename, print_options);
			return true;
        }
        catch(const std::exception& e) {
            error = e.what();
			return false;
		}
    }

    document& reparse(const options parse_mode = Quick) {
        eval_and_assign(parse_mode);
        return *this;
    }

    document & rematerialize(const cppon & print_options = cppon{}, const options parse_mode = Quick) {
        set_source(std::move(to_string(*this, print_options)));
        eval_and_assign(parse_mode);
        return *this;
    }

    // Load file (throws std::runtime_error on failure)
    static document from_string(std::string& file, const options opt = Quick) {
        return document{ std::move(file), opt };
    }

    // Load file (throws std::runtime_error on failure)
    static document from_file(const std::string& filename, const options opt = Quick) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) throw file_operation_error(filename, "open");
        in.seekg(0, std::ios::end);
        std::string buf(static_cast<size_t>(in.tellg()), 0x00);
        in.seekg(0, std::ios::beg);
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        if (!in && !in.eof()) throw file_operation_error(filename, "read from");
        return document{ std::move(buf), opt };
    }

    // Load file (noexcept, returns invalid document on failure)
    static document from_file(const std::string& filename, std::string& error, const options opt = Quick) noexcept {
        std::ifstream in(filename, std::ios::binary);
        if (in) {
            in.seekg(0, std::ios::end);
            std::string buf(static_cast<size_t>(in.tellg()), 0x00);
            in.seekg(0, std::ios::beg);
            in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
            if (in) return document{ std::move(buf), opt };
            error = "Failed to read from file: " + filename;
        }
		else error = "Failed to open file: " + filename;
        return document{ nullptr };
    }
};

} // namespace ch5

#endif // CPPON_DOCUMENT_H

/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-config.h : C++ON Configuration and Settings
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_CONFIG_H
#define CPPON_CONFIG_H

namespace ch5 {

class config;
inline SimdLevel effective_simd_level() noexcept;
inline SimdLevel set_effective_simd_level();
inline const config& get_config(string_view_t key) noexcept;
inline const std::array<string_view_t, 10>& get_vars() noexcept;
inline void update_config(size_t index);

class config : public cppon {
    inline static thread_local size_t dirty = ~0ULL;
public:
    config() = default;
    config(const cppon& lhr) : cppon{ lhr } {}
    config(cppon&& lvr) : cppon{ std::move(lvr) } {}
    config(const config&) = default;
    config(config&&) noexcept = default;
    ~config() noexcept = default;
    config& operator=(const config&) = default;
    config& operator=(config&&) noexcept = default;

    inline void update() {
        if (dirty == ~0ULL) return;
        update_config(dirty);
        dirty = ~0ULL;
    }

    inline config& operator[](string_view_t path) {
        auto& vars = get_vars();
        auto& var = cppon::operator[](path);
        auto name = path.substr(path.rfind("/") + 1);
        auto found = std::find(vars.begin(), vars.end(), name);
        dirty = (found == vars.end()) ? ~0ULL : distance(vars.begin(), found);
        return static_cast<config&>(var);
    }
    inline config& operator[](size_t index) {
        dirty = ~0ULL;
        return static_cast<config&>(cppon::operator[](index));
    }
    inline const config& operator[](string_view_t path) const {
        return static_cast<const config&>(cppon::operator[](path));
    }
    inline const config& operator[](size_t index) const {
        return static_cast<const config&>(cppon::operator[](index));
    }

    config& operator=(const char* val) {
        cppon::operator=( val );
        update();
        return *this;
    }

    config& operator=(pointer_t pointer) {
        cppon::operator=(pointer);
        update();
        return *this;
    }

    template<
        typename T,
        typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
    config& operator=(const T& val) {
        cppon::operator=(val);
        update();
        return *this;
    }

    template<
        typename T,
        typename = std::enable_if_t<is_in_variant_rv<T, value_t>::value>>
    config& operator=(T&& val) {
        cppon::operator=(std::forward<T>(val));
        update();
        return *this;
    }
};

inline static thread_local config Config{
    { object_t{
        { "parser", { object_t{
            { "prefix", { object_t{
                { "path", { string_view_t{CPPON_PATH_PREFIX} } },
                { "blob", { string_view_t{CPPON_BLOB_PREFIX} } },
                { "number", { string_view_t{CPPON_NUMBER_PREFIX} } }
            } } },
            { "exact", {false} }
        } } },
        { "scanner", { object_t{
            { "simd", { object_t{
                { "global", { nullptr } },
                { "thread", { nullptr } },
                { "current", { nullptr } }
            } } }
        } } },
        { "memory", { object_t{
            { "reserve", { object_t{
                { "object", { (int64_t)CPPON_OBJECT_MIN_RESERVE } },
                { "array", { (int64_t)CPPON_ARRAY_MIN_RESERVE } },
                { "printer", { (int64_t)CPPON_PRINTER_RESERVE_PER_ELEMENT } }
            } } }
        } } }
    } }
};

inline const config& get_config(string_view_t key) noexcept {
    try {
        return Config[key];
    }
    catch (...) { return static_cast<const config&>(null()); }
}
inline bool has_config(string_view_t key) noexcept {
    return get_config(key).is_null() ? false : true;
}
template<
    typename T,
    typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
inline config& set_config(string_view_t key, const T& value) {
    return Config[key] = value;
}
template<
    typename T,
    typename = std::enable_if_t<is_in_variant_rv<T, value_t>::value>>
inline config& set_config(string_view_t key, T&& value) {
    return Config[key] = std::move(value);
}

inline void set_path_prefix(string_view_t prefix) {
    set_config("parser/prefixes/path", prefix);
}
inline void set_blob_prefix(string_view_t prefix) {
    set_config("parser/prefixes/blob", prefix);
}
inline void set_number_prefix(string_view_t prefix) {
    set_config("parser/prefixes/number", prefix);
}
inline void set_exact_number_mode(boolean_t mode) {
    set_config("parser/exact", mode);
}
inline boolean_t get_exact_number_mode() noexcept {
    return thread::exact_number_mode;
}
inline void set_global_simd_override(SimdLevel Level) {
    thread::set_global_simd_override(Level);
    set_config("scanner/simd/global", (int64_t)thread::effective_simd_level());
}
inline void clear_global_simd_override() {
    thread::clear_global_simd_override();
    set_config("scanner/simd/global", nullptr);
}
inline void set_thread_simd_override(SimdLevel Level) {
    thread::set_thread_simd_override(Level);
    set_config("scanner/simd/thread", (int64_t)thread::effective_simd_level());
}
inline void clear_thread_simd_override() {
    thread::clear_thread_simd_override();
    set_config("scanner/simd/thread", nullptr);
}
inline SimdLevel set_effective_simd_level(SimdLevel Level) {
    set_config("scanner/simd/current", (int64_t)Level);
    return thread::effective_simd_level();
}
inline SimdLevel effective_simd_level() noexcept {
    return thread::effective_simd_level();
}
inline void set_object_min_reserve(std::byte reserve) {
    set_config("memory/reserve/object", int64_t(reserve));
}
inline void set_array_min_reserve(std::byte reserve) {
    set_config("memory/reserve/array", int64_t(reserve));
}
inline void set_printer_reserve_per_element(std::byte reserve) {
    set_config("memory/reserve/printer", int64_t(reserve));
}

constexpr static std::array<string_view_t, 10> vars{
    "path", "blob", "number", "exact", "global",
    "thread", "current", "object", "array", "printer" };

constexpr static std::array<void(*)(), 103> updaters{
    [] {const config& path = get_config("parser/prefix/path");
        if (path.is_null()) thread::path_prefix.current = CPPON_PATH_PREFIX;
        else if (thread::path_prefix != std::get<string_view_t>(path))
            thread::path_prefix = std::get<string_view_t>(path);
    },
    [] {const auto& blob = get_config("parser/prefix/blob");
        if (blob.is_null()) thread::blob_prefix.current = CPPON_BLOB_PREFIX;
        else if (thread::blob_prefix != std::get<string_view_t>(blob))
            thread::blob_prefix = std::get<string_view_t>(blob);
    },
    [] {const auto& number = get_config("parser/prefix/number");
        if (number.is_null()) thread::number_prefix.current = CPPON_NUMBER_PREFIX;
        else if (thread::number_prefix != std::get<string_view_t>(number))
            thread::number_prefix = std::get<string_view_t>(number);
    },
    [] {const auto& exact = get_config("parser/exact");
        if (exact.is_null()) thread::exact_number_mode.current = false;
        else if (get_cast<boolean_t>(exact) != thread::exact_number_mode)
            thread::exact_number_mode.current = get_cast<boolean_t>(exact);
    },
    [] {const auto& global = get_config("scanner/simd/global");
        const auto& thread = get_config("scanner/simd/thread");
        const auto& current = get_config("scanner/simd/current");
        if (global.is_null()) thread::clear_global_simd_override();
        else thread::set_global_simd_override((SimdLevel)get_cast<int64_t>(global));
        if (thread.is_null()) thread::clear_thread_simd_override();
        else thread::set_thread_simd_override((SimdLevel)get_cast<int64_t>(thread));
        if (global.is_null() && thread.is_null() && (SimdLevel)get_cast<int64_t>(current) != thread::effective_simd_level()) {
            thread::set_thread_simd_override((SimdLevel)get_cast<int64_t>(current));
        }
        thread::simd_default.current = (int64_t)thread::effective_simd_level();
    },
    [] {updaters[4]();},
    [] {updaters[4]();},
    [] {const auto& object = get_config("memory/reserve/object");
        if (object.is_null()) thread::object_min_reserve.current = CPPON_OBJECT_MIN_RESERVE;
        else if (get_cast<int64_t>(object) != thread::object_min_reserve)
            thread::object_min_reserve.current = get_cast<int64_t>(object);
    },
    [] {const auto& array = get_config("memory/reserve/array");
        if (array.is_null()) thread::array_min_reserve.current = CPPON_ARRAY_MIN_RESERVE;
        else if (get_cast<int64_t>(array) != thread::array_min_reserve)
            thread::array_min_reserve.current = get_cast<int64_t>(array);
    },
    [] {const auto& printer = get_config("memory/reserve/printer");
        if (printer.is_null()) thread::printer_reserve_per_element.current = CPPON_PRINTER_RESERVE_PER_ELEMENT;
        else if (get_cast<int64_t>(printer) != thread::printer_reserve_per_element)
            thread::printer_reserve_per_element.current = get_cast<int64_t>(printer);
    } };

inline void update_config(size_t index) {
    updaters[index]();
}

inline const std::array<string_view_t, 10>& get_vars() noexcept {
    return vars;
}

} // namespace ch5

#endif // CPPON_CONFIG_H


#endif // CPPON_H