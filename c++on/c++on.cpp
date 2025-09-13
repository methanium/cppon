#define CPPON_TRUSTED_INPUT
#define CPPON_ENABLE_SIMD
#define _CPPON_ONLY_SCALAR_TESTS

#ifndef CPPON_BENCH_ADVANCED
#define CPPON_BENCH_ADVANCED 0  // 0 = only min/max/avg; 1 = also median/p95
#endif
#ifndef CPPON_BENCH_EXPERIMENTS
#define CPPON_BENCH_EXPERIMENTS 0 // 0 = no extra runs; 1 = extra Default run/calibration blocks
#endif

#include <platform/processor_features_info.h>
#include <utils/string_buffer.h>
#include <cppon/c++on.h>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string_view>
#include <algorithm>
#include <utility>
#include <variant>
#include <chrono>
#include <vector>
#include <array>
#include <deque>
#include <queue>
#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
	#include <Windows.h>
	// Définitions pour Windows
	#define SET_HIGH_PRIORITY() do { \
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS); \
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); \
        /* Optional: pin to CPU 0 for stability */ \
        SetThreadAffinityMask(GetCurrentThread(), 1ULL << 0); \
	} while (0)
	#define SET_NORMAL_PRIORITY() do { \
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS); \
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL); \
        /* Optionnel: clear specific affinity */ \
        SetThreadAffinityMask(GetCurrentThread(), ~0ULL); \
	} while (0)
#elif defined(__linux__) || defined(__unix__)
	#include <sys/resource.h>
	#include <pthread.h>
	#include <sched.h>
	#include <unistd.h>
	#define SET_HIGH_PRIORITY() do { \
			int prio = -10; setpriority(PRIO_PROCESS, getpid(), prio); \
			struct sched_param sp{}; sp.sched_priority = 0; \
			sched_setscheduler(0, SCHED_BATCH, &sp); \
			/* Optionnel: pin to CPU 0 */ \
			cpu_set_t cpuset; CPU_ZERO(&cpuset); \
            CPU_SET(0, &cpuset); \
			pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset); \
			\
	} while(0)
	#define SET_NORMAL_PRIORITY() do { \
			int prio = 0; setpriority(PRIO_PROCESS, getpid(), prio); \
			struct sched_param sp{}; sp.sched_priority = 0; \
			sched_setscheduler(0, SCHED_OTHER, &sp); \
	} while(0)
#else
	#define SET_HIGH_PRIORITY()
	#define SET_NORMAL_PRIORITY()
#endif

auto test_string_buffer(std::string_view File, double Rate, size_t InitialSize = 1024, size_t ChunkSize = 512) {
	utils::string_buffer<char> Buffer{ InitialSize, Rate };
	std::vector<char> ChunkBuffer(ChunkSize);
	size_t alignedOffset{};
	std::ifstream Catalog{ File.data(), std::ios::binary };
	if (!Catalog) {
		throw std::runtime_error(std::string("cannot open file: ").append(File));
	}

	auto StartBuild = std::chrono::high_resolution_clock().now();
	while (Catalog) {
		Catalog.read(std::data(ChunkBuffer), static_cast<std::streamsize>(ChunkSize));
		std::streamsize n = Catalog.gcount();
		if (n <= 0) break;
		Buffer.append(std::string_view(std::data(ChunkBuffer), static_cast<size_t>(n)));
	}
	auto StopBuild = std::chrono::high_resolution_clock().now();

	auto StartSerialize = std::chrono::high_resolution_clock().now();
	auto String = Buffer.to_string(alignedOffset);
	auto StopSerialize = std::chrono::high_resolution_clock().now();

	return std::tuple(std::move(String), alignedOffset, StartBuild, StopBuild, StartSerialize, StopSerialize, Buffer.Size, Buffer.Buf.size());
}

void run_tests() {

	try {
		platform::processor_features_info Info;
		auto ven_id = Info.vendor_id();
		auto version = Info.version_info();
		auto model = Info.model_info();
		auto features = Info.features_info();
		auto features_ex = Info.features_info_ex();
		auto cpu_features = Info.cpu_features();
		auto extended_features = Info.structured_extended_features();

		auto obj = ch5::eval(R"({
			"data": {"x":"v"},
			"ref": "$cppon-path:/data/x"
		})");
		// The token is stored as path_t until resolved

		auto p = std::get<ch5::path_t>(obj["/ref"]).value;

		// "/data/x"
		// Ensure the correct root when resolving the path:
		// - Either use obj["/..."] once (absolute) before resolving,
		// - Or use ch5::root_guard(obj) around the resolution.
		auto& target = ch5::visitors::visitor(obj, p.substr(1)); // since we use
		auto& v = std::get<std::string_view>(target);   // "v"


		ch5::cppon root{};
		root["/parent/child1"] = "value1";
		root["/parent/child2"] = "value2";

		assert(std::holds_alternative<ch5::object_t>(root["/parent"]));
		assert(std::holds_alternative<ch5::string_view_t>(root["/parent/child1"]));
		assert(std::holds_alternative<ch5::string_view_t>(root["/parent/child2"]));
		assert(std::get<ch5::string_view_t>(root["/parent/child1"]) == "value1");
		assert(std::get<ch5::string_view_t>(root["/parent/child2"]) == "value2");

		root["/array/0/member1"] = "value1";
		root["/array/1/member2"] = "value2";

		assert(std::holds_alternative<ch5::array_t>(root["/array"]));
		assert(std::holds_alternative<ch5::object_t>(root["/array/0"]));
		assert(std::holds_alternative<ch5::object_t>(root["/array/1"]));
		assert(std::holds_alternative<ch5::string_view_t>(root["/array/0/member1"]));
		assert(std::holds_alternative<ch5::string_view_t>(root["/array/1/member2"]));
		assert(std::get<ch5::string_view_t>(root["/array/0/member1"]) == "value1");
		assert(std::get<ch5::string_view_t>(root["/array/1/member2"]) == "value2");

		root["/level1/level2/level3/member"] = "value";

		assert(std::holds_alternative<ch5::object_t>(root["/level1"]));
		assert(std::holds_alternative<ch5::object_t>(root["/level1/level2"]));
		assert(std::holds_alternative<ch5::object_t>(root["/level1/level2/level3"]));
		assert(std::holds_alternative<ch5::string_view_t>(root["/level1/level2/level3/member"]));
		assert(std::get<ch5::string_view_t>(root["/level1/level2/level3/member"]) == "value");

		root["/pointer/3"] = &root["/array/2"];
		root["/pointer/3/member3"] = "value3";

		auto print1 = to_string(root, "{\"compact\" : false}");
		auto print2 = to_string(root, "{\"layout\" : {\"flatten\":true,\"compact\":false}}");

		assert(std::holds_alternative<ch5::pointer_t>(root["/pointer/3"]));

		root["/pointer/3"] = &root["/array"];

		auto print3 = to_string(root, "{\"compact\" : false}");
		auto print4 = to_string(root, "{\"layout\" : {\"flatten\":true,\"compact\":false}}");

		auto object = ch5::eval(
			"{"
			"\"root\":["
			"{"
			"\"info\":{"
			"\"id\":\"test\","
			"\"pointer\":\"$cppon-path:/root/1\""
			"}"
			"},"
			"{"
			"\"info\":{"
			"\"id\":\"test\","
			"\"pointer\":\"$cppon-path:/root/2\""
			"}"
			"},"
			"{"
			"\"info\":{"
			"\"id\":\"test\","
			"\"pointer\":\"$cppon-path:/root/2\""
			"}"
			"},"
			"[\"cppon-path:/root/0/info\"]"
			"],"
			"\"version\":false"
			"}");

		auto OldWhat = object["/root/0/info"];
		auto Absolute = object["/root/0"];
		auto Relative = Absolute["info"];
		auto What = Relative["id"];
		auto RootFromLeaf = Relative["/root/1"];

		std::string TestString = "test";

		object["/newObject/member1"] = "value1";
		object["/newObject/member2"] = "value2";

		object["new_object/label"] = "new_value";
		object["new_array/0"] = TestString;
		object["new_array/1"] = std::move(TestString);

		object["version"] = What;
		object["version"] = 1.0;
		object["version"] = ch5::eval("3.14", ch5::Quick);


		int intValue = ch5::get_cast<int>(object["version"]);
		double Value = ch5::get_strict<double>(object["version"]);

		object["version"] = ch5::eval("{\"blob\":\"$cppon-blob:SGVsbG8sIFdvcmxkIQ==\"}", ch5::Full);


		auto blob = get_blob(object["version/blob"]);

		auto printed1 = to_string(object, "{\"compact\" : false}");

		auto Map = resolve_paths(object);

		auto printed2 = to_string(object, &Map, "{\"layout\" : {\"flatten\":true,\"compact\":false}}");

		ch5::to_string(object);

	}
	catch (const std::exception& e) {
		std::cerr << "Test exception: " << e.what() << "\n";
	}
}

struct benchmark_options {
	std::string_view Filename = "Catalog.json";
	ch5::options parser_options = ch5::options::quick;
	int numIterations = 100;
	size_t InitialSize = 1024u;
	size_t ChunkSize = 512u;
	double growRate = 4.0;
	bool   Warmup = false;
};

static const char* option_name(ch5::options o) {
	switch (o) {
	case ch5::options::full:  return "full";
	case ch5::options::eval:  return "eval";
	case ch5::options::quick: return "quick";
	case ch5::options::parse: return "parse";
	default: return "?";
	}
}

static void print_compile_arch() {
#if CPPON_USE_SIMD
	std::cout << "SIMD support: enabled\n";
#else
	std::cout << "SIMD support: disabled\n";
#endif
	std::cout << "Compile-time ISA: ";
	// AVX-512 (any common subset implies AVX-512 codegen)
#if defined(__AVX512F__) || defined(__AVX512BW__) || defined(__AVX512DQ__) || defined(__AVX512VL__) || defined(__AVX512CD__)
	std::cout << "AVX-512";
	// AVX2
#elif defined(__AVX2__)
	std::cout << "AVX2";
	// AVX
#elif defined(__AVX__)
	std::cout << "AVX";
	// SSE4.2 if available, otherwise at least SSE2 on MSVC x64 or /arch:SSE2
#elif defined(__SSE4_2__)
	std::cout << "SSE4.2";
#elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
	std::cout << "SSE2";
#else
	std::cout << "baseline";
#endif
	std::cout << "\n\n";
}

void print_simd_level_with_requested(const char* requested) {
	std::cout << "Requested: " << requested << "\n";
	switch (ch5::effective_simd_level()) {
	case ch5::SimdLevel::SWAR:  std::cout << "Effective: SWAR\n\n"; break;
	case ch5::SimdLevel::SSE:   std::cout << "Effective: SSE\n\n"; break;
	case ch5::SimdLevel::AVX2:  std::cout << "Effective: AVX2\n\n"; break;
	case ch5::SimdLevel::AVX512:std::cout << "Effective: AVX-512\n\n"; break;
	}
}

static float mbps(size_t bytes, long long ns) {
	return ns > 0 ? (float(bytes) * 1000.0f) / float(ns) : 0.0f;
}

void run_benchmark(benchmark_options options) {
	typedef std::array<std::vector<long long>, 4> time_arrays_t;
	auto [Filename, parser_options, numIterations, InitialSize, ChunkSize, growRate, Warmup] = options;
	const bool measure_print = (parser_options != ch5::options::parse);

	auto median_ns = [](time_arrays_t& v, bool first)->long long {
		std::vector<long long> w; w.reserve(v[2].size());
		for (int i = 0; i < v[2].size(); ++i) {
			w.push_back(first ? v[2][i] : v[3][i]);
		}
		size_t mid = w.size() / 2;
		std::nth_element(w.begin(), w.begin() + mid, w.end());
		return w[mid];
		};
	auto p95_ns = [](time_arrays_t& v, bool first)->long long {
		std::vector<long long> w; w.reserve(v[2].size());
		for (int i = 0; i < v[2].size(); ++i) {
			w.push_back(first ? v[2][i] : v[3][i]);
		}
		size_t k = size_t(w.size() * 0.95f); if (k >= w.size()) k = w.size() - 1;
		std::nth_element(w.begin(), w.begin() + k, w.end());
		return w[k];
		};

	if (!Warmup) {
		std::cout << numIterations << " iterations\n";
		std::cout << "Mode: " << option_name(parser_options) << "\n\n";
	}

	ch5::cppon Obj;
	std::string Str;

	auto Opt = ch5::eval(R"({"layout" : {"flatten":false, "compact":true}})");

	SET_HIGH_PRIORITY();

	// Space for outliers (10 lowest + 10 highest)
	numIterations += 20;

	try {
		//auto PrintOptions = ch5::eval("{\"compact\":true,\"buffer\":\"reserve\"}");
		time_arrays_t BufferTimes{
			std::vector<long long>(numIterations),
			std::vector<long long>(numIterations),
			std::vector<long long>(numIterations),
			std::vector<long long>(numIterations) };

		// build | serialize | parse | print
		std::tuple<long long, long long, long long, long long> BufferMean = {};
		std::string_view Txt;
		size_t BufferSize = 0;
		size_t BufferChunks = 0;

		for (int iteration = 0; iteration < numIterations; ++iteration) {
			using std::chrono::steady_clock;
			using std::chrono::nanoseconds;
			using std::chrono::duration_cast;

			auto& build = BufferTimes[0][iteration];
			auto& serialize = BufferTimes[1][iteration];
			auto& parse = BufferTimes[2][iteration];
			auto& print = BufferTimes[3][iteration];

			auto [JSON, alignedOffset, StartBuild, StopBuild, StartSerialize, StopSerialize, Size, Chunks] =
				test_string_buffer(Filename, growRate, InitialSize, ChunkSize);

			Txt = std::string_view(std::data(JSON), std::size(JSON));
			BufferSize = Size;
			BufferChunks = Chunks;

			auto Start = std::chrono::steady_clock().now();
			Obj = ch5::eval(Txt, parser_options); // parse|quick|eval|full
			auto Stop = std::chrono::steady_clock().now();

			long long printDelta = 0;
			if (measure_print) {
				Str = ch5::to_string(Obj, Opt);
				auto PrintTime = steady_clock::now();
				printDelta = duration_cast<nanoseconds>(PrintTime - Stop).count();
			}

			build = duration_cast<nanoseconds>(StopBuild - StartBuild).count();
			serialize = duration_cast<nanoseconds>(StopSerialize - StartSerialize).count();
			parse = duration_cast<nanoseconds>(Stop - Start).count();
			print = measure_print ? printDelta : 0;
		}

		if (!Warmup) {
			// Remove outliers (10 lowest + 10 highest)
			numIterations -= 20;

			std::sort(BufferTimes[0].begin(), BufferTimes[0].end());
			BufferTimes[0].erase(BufferTimes[0].begin(), BufferTimes[0].begin() + 10);
			BufferTimes[0].resize(numIterations);
			for (auto BufferTime : BufferTimes[0]) {
				std::get<0>(BufferMean) += BufferTime;
			}
			std::get<0>(BufferMean) /= numIterations;

			std::cout << "file to buffer:\n";
			std::cout << "Buffer size: " << BufferSize << " bytes, " << BufferChunks << " chunks\n";
			std::cout << "min:" << float(BufferTimes[0].front()) / 1'000'000.f << " ms, "
				<< float(BufferSize) * 1000.0f / BufferTimes[0].front() << " MB/s\n";
			std::cout << "max:" << float(BufferTimes[0].back()) / 1'000'000.f << " ms, "
				<< float(BufferSize) * 1000.0f / BufferTimes[0].back() << " MB/s\n";
			std::cout << "avg:" << float(std::get<0>(BufferMean)) / 1'000'000.f << " ms, "
				<< float(BufferSize) * 1000.0f / std::get<0>(BufferMean) << " MB/s\n";

			std::sort(BufferTimes[1].begin(), BufferTimes[1].end());
			BufferTimes[1].erase(BufferTimes[1].begin(), BufferTimes[1].begin() + 10);
			BufferTimes[1].resize(numIterations);
			for (auto BufferTime : BufferTimes[1]) {
				std::get<1>(BufferMean) += BufferTime;
			}
			std::get<1>(BufferMean) /= numIterations;

			std::cout << "buffer to string:\n";
			std::cout << "min:" << float(BufferTimes[1].front()) / 1'000'000.f << " ms, "
				<< float(BufferSize) * 1000.0f / BufferTimes[1].front() << " MB/s\n";
			std::cout << "max:" << float(BufferTimes[1].back()) / 1'000'000.f << " ms, "
				<< float(BufferSize) * 1000.0f / BufferTimes[1].back() << " MB/s\n";
			std::cout << "avg:" << float(std::get<1>(BufferMean)) / 1'000'000.f << " ms, "
				<< float(BufferSize) * 1000.0f / std::get<1>(BufferMean) << " MB/s\n";

			std::sort(BufferTimes[2].begin(), BufferTimes[2].end());
			BufferTimes[2].erase(BufferTimes[2].begin(), BufferTimes[2].begin() + 10);
			BufferTimes[2].resize(numIterations);
			for (auto BufferTime : BufferTimes[2]) {
				std::get<2>(BufferMean) += BufferTime;
			}
			std::get<2>(BufferMean) /= numIterations;

			std::cout << "string to object: " << Txt.size() << "\n";
			std::cout << "min: " << float(BufferTimes[2].front()) / 1'000'000.f << " ms, "
				<< float(Txt.size()) * 1000.0f / BufferTimes[2].front() << " MB/s\n";
			std::cout << "max: " << float(BufferTimes[2].back()) / 1'000'000.f << " ms, "
				<< float(Txt.size()) * 1000.0f / BufferTimes[2].back() << " MB/s\n";
			std::cout << "avg: " << float(std::get<2>(BufferMean)) / 1'000'000.f << " ms, "
				<< float(Txt.size()) * 1000.0f / std::get<2>(BufferMean) << " MB/s\n";

			#if CPPON_BENCH_ADVANCED
			auto med_parse = median_ns(BufferTimes, /*first=*/true);
			auto p95_parse = p95_ns(BufferTimes, /*first=*/true);
			std::cout << "median: " << float(med_parse) / 1'000'000.f << " ms, "
				<< mbps(Txt.size(), med_parse) << " MB/s\n";
			std::cout << "p95: " << float(p95_parse) / 1'000'000.f << " ms, "
				<< mbps(Txt.size(), p95_parse) << " MB/s\n";
			#endif

			if (measure_print) {
				std::sort(BufferTimes[3].begin(), BufferTimes[3].end());
				BufferTimes[3].erase(BufferTimes[3].begin(), BufferTimes[3].begin() + 10);
				BufferTimes[3].resize(numIterations);
				for (auto BufferTime : BufferTimes[3]) {
					std::get<3>(BufferMean) += BufferTime;
				}
				std::get<3>(BufferMean) /= numIterations;

				std::cout << "object to string: " << Str.size() << "\n";
				std::cout << "min: " << float(BufferTimes[3].front()) / 1'000'000.f << " ms, "
					<< float(Str.size()) * 1000.0f / BufferTimes[3].front() << " MB/s\n";
				std::cout << "max: " << float(BufferTimes[3].back()) / 1'000'000.f << " ms, "
					<< float(Str.size()) * 1000.0f / BufferTimes[3].back() << " MB/s\n";
				std::cout << "avg: " << float(std::get<3>(BufferMean)) / 1'000'000.f << " ms, "
					<< float(Str.size()) * 1000.0f / std::get<3>(BufferMean) << " MB/s\n";

				#if CPPON_BENCH_ADVANCED
				std::cout << "\nResulting string size: " << Str.size() << "\n";
				auto med_print = median_ns(BufferTimes, /*first=*/false);
				auto p95_print = p95_ns(BufferTimes,    /*first=*/false);
				std::cout << "median: " << float(med_print) / 1'000'000.f << " ms, "
					<< mbps(Str.size(), med_print) << " MB/s\n";
				std::cout << "p95: " << float(p95_print) / 1'000'000.f << " ms, "
					<< mbps(Str.size(), p95_print) << " MB/s\n";
				#endif
			}
			else {
				std::cout << "parse mode selected: no serialization.\n";
			}
		}
	}
	catch (std::exception& Err) {
		std::cout << "error: " << Err.what() << "\n";
	}
	catch (const char* Err) {
		std::cout << "error: " << Err << "\n";
	}

	SET_NORMAL_PRIORITY();
}

int main()
{
	using namespace ch5;

	std::cout << "C++ON version: " << ch5::cppon_version_string()
		<< " (0x" << std::hex << ch5::cppon_version_hex() << std::dec << ")\n";

	benchmark_options bench_options{};
	bench_options.parser_options = options::quick;

	print_compile_arch();

	#ifdef CPPON_TRUSTED_INPUT
	std::cout << "Trusted input: ON\n";
	#else
	std::cout << "Trusted input: OFF\n";
	#endif

	#if defined(CPPON_ONLY_SCALAR_TESTS)
	// Force scalar
	set_thread_simd_override(SimdLevel::SWAR);
	print_simd_level_with_requested("Force scalar");
	#endif

	run_tests();

	std::cout << "\nWarmup: " << "\n";
	bench_options.Warmup = true;
	print_simd_level_with_requested("Default");
	run_benchmark(bench_options);
	bench_options.Warmup = false;

	std::cout << "\nBegin benchmarks: " << "\n";

	#ifndef CPPON_ENABLE_SIMD
	std::cout << "\nSwitch to Scalar: " << "\n";
	print_simd_level_with_requested("SWAR");

	run_benchmark(bench_options);

	run_benchmark(bench_options);

	run_benchmark(bench_options);

	run_benchmark(bench_options);

	#if CPPON_BENCH_EXPERIMENTS
	run_benchmark(bench_options); // extra comparaison finale
	#endif
	#else
	#if CPPON_BENCH_EXPERIMENTS
	int numIterations = 5;
	#else
	int numIterations = 4;
	#endif
	while(numIterations--) {
		std::cout << "\nSwitch to Scalar: " << "\n";
		set_global_simd_override(SimdLevel::SWAR);
		print_simd_level_with_requested("SWAR");
		run_benchmark(bench_options);

		std::cout << "\nSwitch to SSE: " << "\n";
		set_global_simd_override(SimdLevel::SSE);
		print_simd_level_with_requested("SSE");
		run_benchmark(bench_options);

		std::cout << "\nSwitch to AVX2: " << "\n";
		set_global_simd_override(SimdLevel::AVX2);
		print_simd_level_with_requested("AVX2");
		run_benchmark(bench_options);

		std::cout << "\nSwitch to AVX512: " << "\n";
		set_global_simd_override(SimdLevel::AVX512);
		print_simd_level_with_requested("AVX512");
		run_benchmark(bench_options);

		#if CPPON_BENCH_EXPERIMENTS
		std::cout << "\nSwitch to Default: " << "\n";
		clear_global_simd_override();
		print_simd_level_with_requested("Default");
		run_benchmark(bench_options); // extra comparaison finale
		#endif
	}
	#endif
	return 0;
}
