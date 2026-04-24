# Build Guide

This document covers everything needed to configure, build, test, and run zketch on Windows.

---

## Prerequisites

| Requirement | Version | Notes |
|---|---|---|
| Windows | 10 or later | Win32 API, Direct2D, DirectWrite |
| LLVM / Clang | 17+ | `clang-cl` must be on `PATH` |
| CMake | 4.3+ | |
| Ninja | Any recent | Recommended generator |
| Windows SDK | 10.0.19041+ | Includes Direct2D and DirectWrite headers |

### Installing LLVM

Download the Windows installer from [releases.llvm.org](https://releases.llvm.org) and ensure `clang-cl.exe` is available on your `PATH`.

Verify:

```bash
clang-cl --version
```

### Installing CMake and Ninja

The easiest way is via [winget](https://learn.microsoft.com/en-us/windows/package-manager/winget/):

```bash
winget install Kitware.CMake
winget install Ninja-build.Ninja
```

Or download directly from [cmake.org](https://cmake.org/download/) and [ninja-build.org](https://ninja-build.org/).

---

## Configure

From the repository root:

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang-cl
```

CMake will automatically fetch Catch2 (the test framework) via `FetchContent` on first configure. An internet connection is required for this step.

---

## Build

Build all targets (library, executable, examples, tests):

```bash
cmake --build build
```

Build a specific target:

```bash
cmake --build build --target example_hello_window
cmake --build build --target example_game_loop
```

Output locations:

- Executables: `bin/`
- Static libraries: `lib/`

---

## Run

Main executable:

```bash
./bin/zketch
```

Hello Window example (blocking / event-driven mode):

```bash
./bin/example_hello_window
```

Game Loop example (non-blocking / continuous rendering mode):

```bash
./bin/example_game_loop
```

---

## Run Tests

```bash
ctest --test-dir build
```

Run with verbose output to see individual test results:

```bash
ctest --test-dir build --output-on-failure -V
```

Run a specific test binary directly (useful for debugging):

```bash
./bin/test_label
./bin/test_event_dispatcher
./bin/test_widget_io
```

---

## Compiler Flags

The following flags are applied automatically by `CMakeLists.txt`:

| Flag | Purpose |
|---|---|
| `/W4` | High warning level |
| `/permissive-` | Strict conformance mode |
| `/Zc:__cplusplus` | Correct `__cplusplus` macro value |
| `/EHsc` | Standard C++ exception handling |
| `/utf-8` | UTF-8 source and execution character set |
| `/clang:-fcolor-diagnostics` | Colored compiler output (clang-cl only) |
| `/clang:-Wno-unused-command-line-argument` | Suppress clang-cl argument warnings |

---

## Clean Build

To start fresh:

```bash
cmake --build build --target clean
```

Or delete the build directory entirely:

```bash
Remove-Item -Recurse -Force build
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang-cl
cmake --build build
```

---

## Troubleshooting

**`clang-cl` not found**
Ensure LLVM is installed and `clang-cl.exe` is on your `PATH`. You can also pass the compiler path explicitly:

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe"
```

**Direct2D or DirectWrite headers not found**
Ensure the Windows SDK is installed. In Visual Studio Installer, select "Desktop development with C++" and include the latest Windows 10/11 SDK component.

**Catch2 fetch fails**
CMake fetches Catch2 from GitHub during configure. If you are behind a proxy or have no internet access, download Catch2 v3.7.1 manually and place it in `build/_deps/catch2-src`, then re-run configure.

**Linker errors for `d2d1.lib` or `dwrite.lib`**
These are part of the Windows SDK. Verify your SDK installation includes the `lib/` directory for your target architecture (typically `x64`).
