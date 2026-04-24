# Changelog

All notable changes to zketch are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/). Versions follow [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

_No unreleased changes. See [ROADMAP.md](ROADMAP.md) for planned future work._

---

## [1.0.0] - 2026-04-24

Initial release of zketch v1.0 MVP.

### Added

**Core**
- `Application` class — single entry point for the framework. Registers a Win32 window class and manages the message loop lifecycle. Move-only; constructed via `Application::create(class_name, mode)`.
- `PumpMode` enum — `NonBlocking` (PeekMessage) and `Blocking` (GetMessage) pump strategies.
- `NonBlockingPump` — continuous frame loop for game engines and real-time rendering. Drains all pending messages via `PeekMessage(PM_REMOVE)`, then invokes the frame callback once per iteration.
- `BlockingPump` — event-driven loop for desktop applications; thread sleeps when idle via `GetMessage`.
- `MainThreadQueue` — thread-safe singleton queue (`std::mutex` + `std::queue`) for posting work from any thread to be executed on the main thread during the next `flush()` call.

**Widget Layer**
- `Window` — top-level Win32 window widget. Owns the HWND (via RAII `HwndWrapper`), the `Renderer`, and a list of `Panel` children. Default size 800×600. Move-only; constructed via `Window::create()`.
- `Panel` — rectangular container widget. Manages child widget ownership and computes absolute screen positions recursively. Exposes `abs_pos()` for the derived screen coordinate.
- `Label` — static text widget with configurable font family, size, style, and RGBA color. Marks itself dirty on any property change.
- `Button` — interactive widget with a `Normal` / `Hovered` / `Disabled` state machine and a click callback.
- `WidgetBase` — abstract base class with position, size, visibility (`is_visible()`), and dirty-flag management.
- `Color` struct — RGBA color with common color constants in `zk::ui::colors` namespace (`Black`, `White`, `Red`, `Green`, `Blue`, `Yellow`, `Cyan`, `Magenta`, `Transparent`, `Gray`, `LightGray`, `DarkGray`).
- `FontConfig` struct — font family (default `"Segoe UI"`), point size (default `12.0f`), bold, and italic flags.

**Rendering**
- `Renderer` — Direct2D backend. Wraps `ID2D1Factory`, `ID2D1HwndRenderTarget`, and `IDWriteFactory` via `ComPtr`. Move-only; constructed via `Renderer::create(hwnd)`.
- Frame lifecycle: `begin_frame()` / draw calls / `end_frame()`.
- Draw primitives: `clear(Color)`, `draw_rect(Pos, Size, Color)`, `draw_text(string_view, Pos, Size, FontConfig, Color)`.
- Automatic render target recreation on `D2DERR_RECREATE_TARGET` (device loss); `needs_redraw()` flag signals the caller to schedule a repaint.
- Text clipping to widget bounding box.

**Event System**
- `EventDispatcher` — O(1) handler lookup via `std::array` indexed by `EventType`. Supports multiple subscribers per event type. Exception-safe dispatch (exceptions caught, logged, and swallowed). Non-copyable, movable.
- `EventType` enum: `Click`, `Resize`, `Close`, `KeyDown`, `KeyUp`, `MouseEnter`, `MouseLeave`. Backed by `uint32_t`; `_Count` sentinel used to size the handler array.
- Payload structs: `ClickPayload`, `ResizePayload` (`Size<uint16_t>`), `KeyPayload` (`uint32_t vkey`), `ClosePayload`, `MouseEnterPayload`, `MouseLeavePayload`.
- `HandlerId` (`uint32_t`) for selective unsubscribe; zero is reserved as invalid.

**Metric Layer**
- `Pos<T>` — 2D position `{x, y}` with automatic clamping to `[-MAX, MAX]`. Supports CTAD (`Pos{1, 2}` → `Pos<int>`).
- `Size<T>` — 2D dimensions `{w, h}` with automatic clamping to `[0, MAX]`. Supports CTAD.
- `Rect<T>` — flat rectangle `{x, y, w, h}` with `contains()`, `right()`, and `bottom()`.
- `basic_pair<Derived>` — CRTP base providing all arithmetic operators (`+`, `-`, `*`, `/`, `+=`, `-=`, `*=`, `/=`) with clamping. `MAX` is `65535` (`~uint16_t{0}`).

**Error Handling**
- `ErrorCode` enum with 10 distinct codes covering initialization, window creation, rendering, parsing, and thread safety failures.
- `Error` struct with `code`, `message` (human-readable string), and `native_error` (Win32 `GetLastError()` or HRESULT).
- `is_fatal()` — returns true for unrecoverable errors (`InitFailed`, `WindowClassRegistrationFailed`, `WindowCreationFailed`, `RenderTargetCreationFailed`, `WrongThread`).
- `make_error()` and `make_win32_error()` factory helpers.
- `std::expected<T, Error>` used throughout all fallible public API.

**Logging**
- `Logger` singleton with `Level` (`Trace`, `Debug`, `Info`, `Warn`, `Error`, `Fatal`) and `Domain` (`Core`, `UI`, `Render`, `Event`, `IO`, `Parser`) filtering.
- Compile-time level elimination via `if constexpr` in the `log<Level, Domain>()` template method.
- Pluggable sinks via `std::function<void(const LogEntry&)>`.
- `ZK_LOG(level, domain, msg)` macro with automatic `__FILE__` and `__LINE__` capture.
- `reset()` method for test isolation.

**Serialization / Deserialization**
- `WidgetSerializer` — converts a widget hierarchy to a human-readable indentation-based text format (2 spaces per level). The returned string always ends with a newline.
- `WidgetParser` — reconstructs a widget hierarchy from the text format. Returns `std::expected<unique_ptr<Window>, ParseError>`. Comments (`#`) and empty lines are silently skipped.
- `ParseError` — carries a 1-based `line`, 1-based `column`, and a human-readable `message`.
- Round-trip stable format: `serialize(parse(serialize(w))) == serialize(w)`.

**Utilities**
- `NativeWrapper<Handle, Deleter>` — RAII wrapper for Win32 handles. Move-only; deleter called exactly once on destruction.
- `HwndWrapper` — pre-defined alias `NativeWrapper<HWND, &DestroyWindow>`.
- `Arithmetic` concept (`std::is_arithmetic_v<T>`) in `zk::meta`.
- `clamp<T>(val, lo, hi)` utility function.
- `arithmetic_op.hpp` — `add`, `sub`, `mul`, `div` with mixed-type overloads using `std::common_type_t`.
- `to_string()` overloads for all framework `enum class` types (`ErrorCode`, `Level`, `Domain`, `EventType`, `ButtonState`, `PumpMode`).
- `to_underlying()` for explicit enum-to-numeric conversion.

**Examples**
- `example_hello_window` — minimal blocking-mode application with a Label. Build: `cmake --build build --target example_hello_window`.
- `example_game_loop` — non-blocking continuous-render loop with live FPS counter. Build: `cmake --build build --target example_game_loop`.

**Tests**
- 16 test binaries covering all modules: error, log, metric, util, event, ui, render, io, core.
- Property-based tests for `Pos`/`Size` clamping, arithmetic safety, and widget I/O round-trips.
