# vACDM-Plugin

---

# Developer Guide

This project uses **C++20**, **MSVC**, and **Conan** for dependency management.

## Prerequisites

Make sure the following tools are installed before building:
- [CMake](https://cmake.org/) (>= 3.20)
- [Conan](https://conan.io/) (>= 2.x)
- [Docker](https://www.docker.com/) (for integration tests)
- MSVC / Visual Studio 2022 on Windows (or an equivalent compiler on Linux/Mac)

## Building

```bash
conan install . --output-folder=build --build=missing
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -A Win32
cmake --build . --config Release

```

## Testing

All tests are located under the `tests/` directory:  
- **Unit tests:** `tests/`  
- **Integration tests:** `tests/integration/`

To run all tests: ```ctest``` (in build directory)

### Unit Tests

To run **unit tests only**:

```bash
cd build
ctest -L unit
```

### Integration Tests

Integration tests require docker to provide external dependencies (e.g. NATS, HTTP/WebSocket services).

```bash
docker compose integration-tests.yml up -d
cd build
ctest -L integration
```