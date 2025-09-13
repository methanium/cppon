# Document class

The `document` class provides a high-level interface for managing the entire lifecycle of a JSON document, from parsing to manipulation and serialization. It encapsulates the root `value` and offers convenient methods for common operations.

## Overview

While the core `cppon` class handles JSON data structures, the `document` class adds:

- Ownership of the input buffer, ensuring that `string_view` and `blob_string_t` references remain valid as long as the `document` exists.
- File I/O capabilities for loading from and saving to files.
- Serialization methods for converting the document back to a JSON string.
- Lifecycle management, making it easier to work with JSON data in a safe and efficient manner.
- Integration with thread-local root context for path resolution.
- Error handling specific to file operations.
- A clear separation between the low-level JSON representation and high-level document management.
- An extended ABI that does not affect existing code using the `cppon` class directly.
- Example usage:

```cpp
#include <cppon/c++on.h>

using namespace cppon;

int main() {
	try {
		// Load a JSON document from a file
		document doc = document::from_file("example.json");

		// Access the document
		doc["/new_key"] = "new_value";
		
		// Serialize the document back to a string
		std::string json = doc.to_string({"layout":{"json":true, "pretty":true}});
		std::cout << json << std::endl;
		
		// Save the modified document to a new file
		doc.to_file("modified_example.json"); // keep the same pretty json formatting
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	return 0;
}
```

## Advantages over Direct `cppon` Usage

Using `document` instead of raw `cppon` objects offers several benefits:

1.	String Stability: All parsed string_view references point to the internal buffer, eliminating dangling references
2.	Simplified I/O: Built-in methods for file operations without manual stream handling
3.	Fluent API: Method chaining for concise operation sequences
4.	Consistent Root: Automatically registers as the root for path resolution
5.	Memory Safety: Clear ownership semantics for the backing buffer

## Creating Documents

### Empty Document

```cpp
// Create an empty document
ch5::document doc;

// Populate it manually
doc["/name"] = "C++ON";
doc["/version"] = "0.1.0";
```

### From String

```cpp
// From C-style string
ch5::document doc1("{ \"name\": \"C++ON\" }");

// From string_view
std::string_view json_text = R"({ "version": "0.1.0" })";
ch5::document doc2(json_text);

// From string (moved)
std::string json_str = R"({ "license": "MIT" })";
ch5::document doc3(std::move(json_str));
```

### From File

```cpp
// Load from file with exception handling
try {
    auto doc = ch5::document::from_file("config.json");
    // Use document...
} catch (const std::exception& e) {
    std::cerr << "Error loading file: " << e.what() << std::endl;
}
```

### From File with Error Reporting

```cpp
std::string error;
auto doc = ch5::document::from_file("config.json", error);
if (doc.is_null()) {
	std::cerr << "Error loading file: " << error << std::endl;
}
```

## Manipulating Data

```cpp
// Access and modify data
doc["/server/host"] = "example.com";
doc["/server/port"] = 8080;
doc["/enabled"] = true;

// Use paths for deep access
doc["/config/database/username"] = "admin";
doc["/config/database/connection_timeout"] = 30.5f;

// Array manipulation
doc["/tags/0"] = "json";
doc["/tags/1"] = "c++";
doc["/tags/2"] = "performance";

// Use standard cppon accessors
auto host = ch5::get_strict<std::string_view>(doc["/server/host"]);
auto port = ch5::get_cast<int>(doc["/server/port"]);
```

## Serialization and File I/O

### Serialization

```cpp
// Serialize to string (compact by default)
std::string json = doc.serialize();

// With formatting options
std::string pretty = doc.serialize(R"({ "pretty": true })");
std::string compatible = doc.serialize(R"({ "layout": "json" })");
```

### Saving to File
```cpp
// Exception-based version
try {
    doc.to_file("config.json");
    doc.to_file("config_pretty.json", R"({ "pretty": true })");
} catch (const ch5::file_operation_error& e) {
    std::cerr << "Error saving file: " << e.what() << std::endl;
}

// Return-code version with error message
std::string error;
if (!doc.to_file("readonly/config.json", error)) {
    std::cerr << "Failed to save: " << error << std::endl;
}
```

## Document Operations

### Refreshing Content

```cpp
// Re-evaluate the current buffer with different parsing options
doc.reparse(ch5::options::full);  // Convert numbers, decode blobs

// Regenerate the buffer from the current object state
doc.rematerialize();  // Serialize then re-parse (useful for normalizing)

// Rematerialize with specific options
doc.rematerialize(R"({ "pretty": true })", ch5::options::quick);
```

### Clearing and Resetting

```cpp
// Clear both the buffer and object state
doc.clear();
```

## Source Access

```cpp
// Access the underlying buffer
std::string_view source = doc.source_view();

// Check if document is empty
if (doc.empty()) {
    // Initialize with default content
}
```

## Error Handling

The `document` class provides two error handling styles:

1.	Exception-based: Methods like to_file() and from_file() throw exceptions
2.	Return-code based: Overloads that accept an error string and return boolean success flags

Common exceptions:

- `file_operation_error`: Thrown for file I/O errors (opening, reading, writing)
- Parser exceptions (e.g., `unexpected_symbol_error`): When parsing invalid JSON

Complete Example

```cpp
#include <cppon/c++on.h>
using namespace ch5;

int main() {
    try {
        // Create a new document
        document config;
        
        // Populate with configuration
        config["/app/name"] = "MyApp";
        config["/app/version"] = "1.0";
        config["/app/settings/theme"] = "dark";
        config["/app/settings/notifications"] = true;
        
        // Add an array
        config["/app/plugins/0/name"] = "Logger";
        config["/app/plugins/0/enabled"] = true;
        config["/app/plugins/1/name"] = "Analytics";
        config["/app/plugins/1/enabled"] = false;
        
        // Save with pretty printing
        config.to_file("config.json", R"({ "pretty": true })");
        
        // Load it back
        auto loaded = document::from_file("config.json");
        
        // Modify and save with different options
        loaded["/app/version"] = "1.1";
        loaded["/app/updated"] = true;
        
        // Serialize to string
        std::string json = loaded.serialize();
        std::cout << "Config JSON: " << json << std::endl;
        
        // Save with JSON compatibility
        loaded.to_file("config_compat.json", R"({ "layout": "json" })");
        
    } catch (const file_operation_error& e) {
        std::cerr << "File error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## Best Practices

1.	Use `document` for complete files: When working with entire JSON files, prefer `document` over raw `cppon`
2.	Let it own the buffer: Don't modify the underlying buffer directly; use the API
3.	Choose appropriate error handling: Use exceptions for application flow, return codes for recoverable errors
4.	Reuse documents: Clear and repopulate rather than creating new instances for performance
5.	Consider parsing options: Use options::quick for initial load, options::full when complete conversion is needed

Notes

- `document` is non-copyable but movable, ensuring clear ownership of the buffer
- Moving a document transfers ownership of the buffer and the object state
- The `document` class is thread-compatible but not thread-safe; protect shared instances with appropriate synchronization