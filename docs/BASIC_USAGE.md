# Basic Usage

## Parse and access

```cpp
// Fast parse with lazy numbers (no conversion yet)
auto doc = ch5::eval(json_text, ch5::quick);

// Path-based access
auto name = std::get<std::string_view>(doc["/user/name"]);
int age = ch5::get_cast<int>(doc["/user/age"]);     // converts if needed
bool verified = std::get<bool>(doc["/user/verified"]);

// Optional access
if (auto num = std::get_if<double>(&doc["/score"])) {
    // use *num
}
```

## Create and modify

```cpp
ch5::cppon cfg;
cfg["/server/host"] = "example.com";
cfg["/server/port"] = 8080;
cfg["/features/0"] = "a";
cfg["/features/1"] = "b";
```

## Arrays and objects iteration

```cpp
auto& items_node = doc["/items"];
for (auto& item : items_node.array()) {
    // item is cppon&
}

auto& user = doc["/user"];
for (auto& [key, val] : user.object()) {
    // key is std::string_view, val is cppon&
}
```

## Printing

```cpp
std::string pretty  = ch5::to_string(doc, R"({"pretty":true})");
std::string compact = ch5::to_string(doc, R"({"compact":true})");
std::string json    = ch5::to_string(doc, R"({"layout":"json"})");

// // Direct streaming (compact)
std::cout << doc << '\n';

// Stream with custom layout via to_string_view (no extra copy)
std::cout << ch5::to_string_view(doc, R"({"layout":{"json":true,"pretty":true}})");
```

## References (basics)

```cpp
// Pointer reference: link two subtrees in the same document
doc["/settings"] = &doc["/user/preferences"];
doc["/settings/theme"] = "dark"; // also updates /user/preferences/theme
```

## Path tokens and flattening

```cpp
// Path token stored as text, resolvable to a direct pointer
ch5::cppon cfg2 = ch5::eval(R"({
  "user": {"name":"Ada"},
  "ref":  "$cppon-path:/user"
})");

// Convert path_t -> pointer_t for faster dereferencing
auto refs = ch5::resolve_paths(cfg2);

// Flatten inlines referenced content where possible (cycles preserved as paths)
std::string flat = ch5::to_string(cfg2, &refs, R"({"layout":{"flatten":true}})");
```

### Absolute paths

```cpp
using namespace ch5::literals;
ch5::cppon root;
root["/a/b"] = 42;

// From a parsed string
auto p = ch5::eval(R"("$cppon-path:/a/b")");
int v = ch5::get_cast<int>(p); // 42
```

## Binary blobs (Base64 on demand)

```cpp
// Blob token stays as text until realized
ch5::cppon img = ch5::eval(R"({"data":"$cppon-blob:SGVsbG8="})");

// Decode Base64 only when needed (non-const access realizes blob_string_t -> blob_t)
auto& bin = ch5::get_blob(img["/data"]);
```

## Minimal error handling

```cpp
try {
    // Parse-only validation (no DOM materialization)
    auto ok = ch5::eval(R"({"k":1})", ch5::options::parse);

    // Invalid JSON → throws a scanner/parser exception
    (void)ch5::eval("not json", ch5::options::parse);
}
catch (const ch5::unexpected_symbol_error& e) {
    // Handle syntax error (unexpected token)
    std::cerr << "Parse error: " << e.what() << "\n";
}
catch (const ch5::unexpected_end_of_text_error& e) {
    // Handle truncated input
    std::cerr << "Truncated JSON: " << e.what() << "\n";
}
catch (const std::exception& e) {
    // Fallback
    std::cerr << "Error: " << e.what() << "\n";
}
```

```cpp
try {
    // Blob access: const access throws if the blob is not realized (still blob_string_t)
    const ch5::cppon img = ch5::eval(R"({"data":"$cppon-blob:SGVsbG8="})", ch5::options::eval);
    (void)ch5::get_blob(img["/data"]); // throws blob_not_realized_error (const)
}
catch (const ch5::blob_not_realized_error& e) {
    std::cerr << "Blob not realized: " << e.what() << "\n";
}
```