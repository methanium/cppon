# Contributing to C++ON

First off, thank you for considering contributing to C++ON! It's people like you that make this library better for everyone.

### SIMD `'\0'` digits end-of-buffer invariant

- The `_parallel_digits` functions read one extra scalar byte in the epilogue
  (`tail+1`) to detect `'\0'` and accept numbers at the end of the buffer.
- This is a deliberate choice based on the invariant that `std::string_view` arguments
  are backed by null-terminated buffers (either `std::string` or string literals).
- Do not `fix` this behavior without prior discussion. Quote searchers remain strictly bounded.

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report. Following these guidelines helps maintainers understand your report and reproduce the issue.

- **Use the GitHub issue tracker** - Report bugs by opening a new issue in the GitHub repository.
- **Describe the bug** - Provide a clear and concise description of what the bug is.
- **Provide specific examples** - Include steps to reproduce the bug, ideally with minimal code examples.
- **Describe the expected behavior** - Clearly describe what you expected to happen.
- **Include system details** - Specify your OS, compiler version, and CPU architecture.

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion, including completely new features and minor improvements to existing functionality.

- **Use the GitHub issue tracker** - Enhancement suggestions are tracked as GitHub issues.
- **Provide a clear use case** - Explain why this enhancement would be useful to most C++ON users.
- **Consider scope** - Suggest how the enhancement could be implemented, keeping in mind the library's focus on performance and simplicity.

### Pull Requests

- **Fork the repository** and create your branch from `main`.
- **Follow the coding style** described below.
- **Include tests** for new features or bug fixes.
- **Update documentation** for any API changes.
- **Submit a pull request** to the `main` branch.

## Development Setup

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 6+, MSVC 2019+)
- CMake (for building tests)
- Git

### Building Tests

```cpp
mkdir build && cd build cmake .. cmake --build . ctest
```

## Coding Guidelines

### C++ Style

- Follow modern C++17 practices and idioms.
- Keep dependencies minimal.
- Use `auto` where it improves readability.
- Use meaningful variable and function names.
- Keep functions short and focused on a single task.

### Documentation

- Document all public APIs using Doxygen-style comments.
- Include examples for complex functionality.
- Update README.md if adding significant features.

### Performance Considerations

- Consider SIMD optimizations for performance-critical code.
- Avoid unnecessary allocations and copies.
- Design APIs that allow for zero-copy operation where possible.
- Always measure performance impact of changes.

### Cross-Platform Compatibility

- Test on multiple platforms when possible (Windows, Linux, macOS).
- Use conditional compilation appropriately for platform-specific code.
- Consider both GCC/Clang and MSVC compatibility.

## License

By contributing, you agree that your contributions will be licensed under the project's MIT License.

## Code of Conduct

This project adheres to a Contributor Code of Conduct. By participating, you agree to abide by its terms.
Please read the [CODE_OF_CONDUCT.md](./docs/CODE_OF_CONDUCT.md) for more details.

## Contact

If you have any questions or need further assistance, feel free to open an issue or reach out via email at methanium.ch5@gmail.com

If you have any questions or need further assistance, feel free to open an issue on GitHub or contact the maintainer directly (email available on my GitHub profile).

