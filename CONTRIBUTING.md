# Contributing to zketch

Thank you for your interest in contributing. This document explains how to get started, what conventions to follow, and how the review process works.

---

## Getting Started

1. Fork the repository and clone your fork locally.
2. Follow [BUILD.md](BUILD.md) to configure and build the project.
3. Run the test suite to confirm everything passes before making changes:

```bash
ctest --test-dir build
```

4. Create a new branch for your change:

```bash
git checkout -b feature/your-feature-name
```

---

## What to Contribute

Good areas to contribute:

- Bug fixes with a corresponding test that reproduces the issue
- New widgets or rendering primitives that fit the v1.0 architecture
- Improvements to the widget config format parser or serializer
- Documentation corrections or additions
- Performance improvements with measurable benchmarks

Before starting a large feature, open an issue to discuss the design. This avoids wasted effort if the direction does not align with the project roadmap.

---

## Code Conventions

zketch follows strict conventions. Please read them before writing code.

### Language and Standard

- C++23. Use `std::expected<T, zk::error::Error>` for all fallible public API. Do not use exceptions as control flow.
- No raw `new`/`delete`. Use `std::unique_ptr` for ownership and `NativeWrapper<T>` for Win32 handles.
- All COM objects use `Microsoft::WRL::ComPtr`.

### Naming

| Element | Convention | Example |
|---|---|---|
| Files | `snake_case` | `widget_base.hpp` |
| Classes and structs | `PascalCase` | `WidgetBase` |
| Functions and methods | `snake_case` | `set_text()` |
| Template type parameters | `PascalCase` | `Ty`, `Handle` |
| Enum values | `PascalCase` | `ButtonState::Hovered` |
| Private members | trailing underscore | `pos_`, `dirty_` |

### File Layout

- One class or closely related group of types per file.
- Headers go in `include/zk/<module>/`, implementations in `src/<module>/`.
- Namespace must match the folder path: `include/zk/metric/pos.hpp` → `namespace zk::metric`.
- All headers use `#pragma once`.
- Prefer forward declarations over full includes in headers.

### Dependency Order

Do not introduce cyclic dependencies. The allowed dependency order is:

```
meta/util -> error -> log -> metric -> event -> ui -> render -> io -> core
```

### Error Handling

Return `std::expected<T, zk::error::Error>` from any function that can fail. Use `make_error()` or `make_win32_error()` from `zk/error/error.hpp` to construct errors. Do not throw.

### Thread Safety

All widget and message loop operations must run on the Main Thread. If you add cross-thread functionality, use `MainThreadQueue::post()` to marshal work to the main thread.

---

## Writing Tests

Every bug fix and new feature must include tests. Tests live in `test/` and mirror the `src/` structure.

zketch uses Catch2 v3. Add your test file to the appropriate subdirectory and register it in `CMakeLists.txt`.

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MyWidget sets dirty flag on set_text", "[ui][label]")
{
    zk::ui::Label label("hello", {0, 0}, {100, 20});
    label.clear_dirty();
    label.set_text("world");
    REQUIRE(label.is_dirty());
}
```

Run tests with:

```bash
ctest --test-dir build
```

---

## Submitting a Pull Request

1. Ensure all tests pass locally.
2. Keep commits focused. One logical change per commit.
3. Write a clear commit message:
   - First line: imperative mood, under 72 characters (`fix: correct Panel abs_pos clamping`)
   - Body (optional): explain why, not what
4. Open a pull request against the `main` branch.
5. Fill in the PR description: what changed, what was tested, any known limitations.

Pull requests that break the build, fail tests, or violate the conventions above will not be merged until the issues are resolved.

---

## Code of Conduct

All contributors are expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
