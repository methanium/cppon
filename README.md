# C++ON

> A high-performance C++17 JSON parser with references, paths, and binary blobs

Current version: v0.1.0

**C++ON** is an ultra-fast header-only library for parsing and generating JSON in C++. Designed for performance, it leverages modern SIMD instructions while offering an elegant API and advanced features absent from traditional JSON parsers.

## ⚡ Performance

Benchmarks on a 4.7 MB JSON file show impressive speeds¹:

<table style="border-collapse: collapse; border: none;">
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Parsing</strong></td><td style="border: none;">Up to 750 MB/s  (average ~450 MB/s)</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Printing</strong></td><td style="border: none;">Up to 1200 MB/s (average ~750 MB/s)</td></tr>
</table>

These results are achieved without sacrificing features, making **C++ON** one of the fastest JSON solutions available.<br>
Its speed follows from a simple design and the effective use of modern C++, prioritizing an intuitive, easy, and pleasant API.

- SIMD runtime dispatch (SSE/AVX2/AVX‑512) accelerates hot scan paths.
- See docs/PERFORMANCE.md for a summary (Scalar vs SSE vs AVX2) and docs/BENCHMARKS.md for reproduction steps.

## 🌟 Features

<table style="border-collapse: collapse; border: none;">
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;C++17</strong></td><td style="border: none;">Clean, modern API leveraging std::variant and string_view</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Header-only</strong></td><td style="border: none;">Simple integration without external dependencies</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Extended types</strong></td><td style="border: none;">Typed numbers with C++ suffixes (i8, u16, …)</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Binary blobs</strong></td><td style="border: none;">Built‑in base64 encode/decode, realized on demand</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Compatibility</strong></td><td style="border: none;">Standards‑compliant JSON output mode</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Lazy evaluation</strong></td><td style="border: none;">Parse quickly and convert numbers on demand</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Access paths</strong></td><td style="border: none;">Navigate with <code>/parent/child/0/item</code> syntax</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Cross‑references</strong></td><td style="border: none;">Internal links via <code>$cppon-path:/path/to/target</code></td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;Thread‑safe</strong></td><td style="border: none;">Thread isolation via <code>thread_local</code> state</td></tr>
<tr><td style="border: none;">&nbsp;&nbsp;<strong>•&nbsp;&nbsp;SIMD detection</strong></td><td style="border: none;">Leverages SSE, AVX2 or AVX‑512 based on CPU capabilities</td></tr>
</table>

Advanced capabilities:
- Optional path optimization: resolve_paths() converts path_t → pointer_t for faster dereferencing.
- Reference flattening at print time with cycle detection (cycles are emitted as paths).
- Defensive bounds: max array delta, index validation, precise exceptions.
- Pointer safety guard which blocks unsafe pointer assignments.

## 🔧 Installation

**C++ON** is a header-only library - simply include the file(s) in your project:

```cpp
#include <cppon/c++on.h>
```

## 📚 Usage Examples

### Parsing and Data Access

```cpp
// Fast parsing without number conversion
auto doc = ch5::eval(json_text, ch5::options::quick);

// Path-based access
auto name = ch5::get_strict<std::string_view>(doc["/user/name"]);  // Strict type
auto age = ch5::get_cast<int>(doc["/user/age"]);                   // Numeric cast
auto verified = std::get<bool>(doc["/user/verified"]);             // Strict type, std::get

// Optional access
if (auto p = stf::get_if<float>(&doc["/user/score"])) {            // Optional(Polymorphic) type, std::get_if
	// process optional item
}

// Index-based access for arrays
auto first_item = doc["/items/0"];

// Arrays iteration with numeric index and range for
auto items = doc["/items"];
auto first = items[0];

for (const auto& item : items.array()) {
	// process item
}
```

### JSON Formatting Options

```cpp
// Pretty printing
std::string pretty = ch5::to_string(doc, R"({"pretty":true})");

// Compact output for network transmission
std::string compact = ch5::to_string(doc, R"({"compact":true})");

// JSON compatibility mode
std::string compatible = ch5::to_string(doc, R"({"layout":"json"})");

// Combined options with selective object compacting
std::string formatted = ch5::to_string(doc, R"({
  "pretty": true,
  "compact": ["metadata", "debug_info"],
  "layout": {"margin": 2}
})");
```

### Creation and Modification

```cpp
ch5::cppon config;

config["/server/host"] = "example.com";
config["/server/port"] = 8080;
config["/server/timeout"] = 30.5;
config["/server/secure"] = true;

// Creating an array
config["/allowed_origins/0"] = "https://app1.example.com";
config["/allowed_origins/1"] = "https://app2.example.com";

// JSON generation (compact by default)
std::string json = ch5::to_string(config);
```

### Using References

```cpp
// Creating a pointer reference
doc["/settings"] = &doc["/user/preferences"];

// Modification via pointer reference
doc["/settings/theme"] = "dark"; // Also modifies /user/preferences/theme
```

### Optimizing Paths and Flattening

```cpp
// Convert $cppon-path entries into direct pointers
auto refs = ch5::resolve_paths(doc);

// Flatten references during printing (cycles are preserved as paths)
std::string flat = ch5::to_string(doc, &refs, R"({"layout":{"flatten":true}})");
```

### Binary Data Storage

```cpp
ch5::cppon image;
image["width"] = 1920;
image["height"] = 1080;
image["format"] = "png";

// Store raw bytes; printing auto-encodes to base64
image["data"] = ch5::blob_t{/* binary data */};
```

## ⚙️ Configuration Macros

Configurable macros (see include/cppon/c++on.h):
- CPPON_ENABLE_SIMD (opt‑in)
- CPPON_ENABLE_STD_GET_INJECTION (opt‑in)
- CPPON_TRUSTED_INPUT (opt‑in)
- CPPON_PATH_PREFIX
- CPPON_BLOB_PREFIX
- CPPON_PRINTER_RESERVE_PER_ELEMENT
- CPPON_OBJECT_MIN_RESERVE
- CPPON_ARRAY_MIN_RESERVE
- CPPON_MAX_ARRAY_DELTA

Defaults are chosen for performance; CPPON_TRUSTED_INPUT is disabled by default (opt‑in).<br>
See Documentation > Configuration for details and caveats (including “Trusted input” and whitespace handling).

## 📋 Documentation

Complete documentation available in the [docs/](docs/) folder.
See the [Documentation index](docs/INDEX.md) for the complete documentation index (usage, paths, performance, SIMD, API, internals).
Quick start: see [Basic usage](docs/BASIC_USAGE.md).

## 🏆 Comparison

| Library | Performance | API | References | Paths | Thread-safety | Extended Types |
|--------------|:-----------:|:--------:|:----------:|:-----:|:-------------:|:--------------:|
| **C++ON** | <span style="color:yellow">★★★★★</span> | <span style="color:yellow">★★★★★</span> | ✅ | ✅ | ✅ | ✅ |
| RapidJSON | <span style="color:yellow">★★★★</span> | <span style="color:yellow">★★</span> | ❌ | ❌ | ❌ | ❌ |
| nlohmann | <span style="color:yellow">★★</span> | <span style="color:yellow">★★★★★</span> | ❌ | ❌ | ❌ | ❌ |
| simdjson | <span style="color:yellow">★★★★★</span> | <span style="color:yellow">★★</span> | ❌ | ❌ | ❌ | ❌ |

## 🔄 Compatibility

- **Compilers :** GCC 7+, Clang 6+, MSVC 2019+
- **Platforms :** Windows, Linux, macOS
- **Standard  :** C++17 required

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👥 Contributions

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

### Notes

**¹ Benchmark Environment:**  
Intel Core Ultra 9 285K (3.70 GHz) | ASUS ROG Maximus Hero | DDR5/XMP-6400 | SSD

*Compiled with Visual Studio 2022 (Release mode) using C++17. Tests performed with AVX2 (/arch:AVX2) instructions and “Trusted input: ON”  on Windows 11.*
