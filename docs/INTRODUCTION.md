# Introduction

C++ON is a header‑only C++17 library for high‑performance JSON parsing and printing, with practical extensions for real‑world data models.

## Key features

- Extended numeric types (i8/u8, i16/u16, i32/u32, i64/u64, float/double)
- Binary blobs with on‑demand Base64 (blob_string_t ↔ blob_t)
- Internal references: path_t (text) and pointer_t (in‑memory)
- Path navigation with "/a/b/0", navigation paths can be relatives, eg.: "b/0"
- Strict absolute paths: path_t must start with '/', malformed paths throw invalid_path_error
- JSON‑compatible or enriched output modes (flatten, compact/pretty)
- Runtime SIMD detection (SSE/AVX2/AVX‑512) with global/thread overrides
- Precise exceptions, zero external dependencies, thread‑local state
- Header-only distribution, optional single-header (single_include/cppon/c++on.h)

## Design goals

- Performance without sacrificing ergonomics
- Clear, modern C++17 API (std::variant, string_view)
- Predictable hot paths; use SIMD where it pays
- Portable, header‑only integration

## Quickstart

```cpp
#include <cppon/c++on.h>

using namespace ch5;

int main() {
	// Fast parse (lazy numbers)
	
	auto doc = eval(R"({
		"user": {
			"name": "Ada",
			"age": 37
		}
	})", options::quick);

	// Access via paths
	auto name = std::get<string_view_t>(doc["/user/name"]);
	int age   = get_cast<int>(doc["/user/age"]);

	// Print (pretty / compact / JSON-compatible)
	std::string pretty = to_string(doc, R"({"pretty":true})");
	std::string json   = to_string(doc, R"({"layout":{"json":true}})");

	// Stream (compact by default)
	std::cout << doc << '\n';
}
```

## Path invariants

- path_t is always absolute: it must start with '/'. Empty or relative paths throw invalid_path_error at construction.
- Strings with the path prefix (CPPON_PATH_PREFIX, default "$cppon-path:") must be followed by an absolute path.
  - OK: "$cppon-path:/a/b", "$cppon-path:/"
  - Error: "$cppon-path:a/b", "$cppon-path:" → invalid_path_error
- The "_path" literal enforces the same rule:
  - OK: "/a/b"_path; Error: "a/b"_path

## Version helpers
```cpp
#include <cppon/c++on.h>
using namespace ch5;
const char* version = cppon_version_string();
unsigned hex = cppon_version_hex(); // cppon_version_major() / cppon_version_minor() | cppon_version_patch() also available
```

- Paths in navigation can be relatives

```cpp
auto& user = doc["/user"];
int age = get_cast<int>(user["age"]);
```

## Where to next?

- Basic usage: ./BASIC_USAGE.md
- Paths and references: ./PATHS.md
- Configuration (macros, SIMD, trusted input): ./CONFIGURATION.md
- API reference: ./API_REFERENCE.md
- Internals (design notes): ./internals/INDEX.md