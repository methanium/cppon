# Literals

User-defined literals (UDL) to make C++ON code more readable. Enable them in scope:

```cpp
using namespace ch5::literals;
```

Available
- "_cppon"   → parse JSON in quick mode (options::quick)
- "_json"    → parse JSON in quick mode (options::quick)
- "_cpponf"  → parse JSON in full/eval mode (options::eval)
- "_jsonf"   → parse JSON in full/eval mode (options::eval)
- "_options" → parse a print-options object for `to_string(...)`
- "_opts"    → parse a print-options object for `to_string(...)`
- "_path"    → build a `path_t` from an absolute path "/..."
- "_blob64"  → build a `blob_string_t` (Base64 text, not decoded)
- "_b64"     → build a `blob_string_t` (Base64 text, not decoded)
- "_blob"    → build a binary `blob_t` from the literal bytes

Examples
```cpp
using namespace ch5;
using namespace ch5::literals;

auto doc = R"({
	"user": {
		"name": "Ada",
		"age": 37,
		"data": "$cppon-blob:SGVsbG8gd29ybGQh"  // "Hello world!"
	}
})"_json;  // parse in quick mode (lazy numbers), use _jsonf for full/eval mode

// Printing options
auto opts = R"({"pretty":true, "layout":{"json":true}})"_opts;

// Path and blob literals
auto path = "/user/name"_path;       // path_t
auto b64  = "SGVsbG8gd29ybGQh"_b64;  // blob_string_t (Base64 text), or _blob64
auto bin  = "Hello world!"_blob;     // blob_t (binary data)

// Serialize any sub-tree (compact by default)

std::string sub = to_string(doc["/user/data"]);
```

Notes
- Visibility: UDLs are found only if `using namespace ch5::literals;` is in scope.
- Paths: `"_path"` requires a path starting with '/' (throws if malformed).
- Parser inputs must be NUL-terminated: the JSON/options UDLs receive a `const char*` literal and are safe.
- Binary: use `"_blob"` for data with 0x00 bytes; `"_b64"` remains Base64 text (decode via `get_blob(non_const)`).
- Signatures: string UDLs are defined as `(const char*, size_t)`; `"_blob"` is also `(const char*, size_t)`.

See also
- [BASIC_USAGE.md](BASIC_USAGE.md) for a quick tour
- [CONFIGURATION.md](CONFIGURATION.md) for macros (TRUSTED_INPUT, ENABLE_SIMD…)
- [API_REFERENCE.md](API_REFERENCE.md) for the full function list