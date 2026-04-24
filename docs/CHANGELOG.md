# Changelog

All notable changes to zketch are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/). Versions follow [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

No unreleased changes at this time.

---

## [0.1.0] - 2026-04-24

Initial release of zketch v1.0 MVP.

### Added

**Core**
- `Application` class — single entry point for the framework. Registers a Win32 window class and manages the message loop lifecycle.
- `PumpMode` enum — `NonBlocking` (PeekMessage) and `Blocking` (GetMessage) pump strategies.
- `NonBlockingPump` — continuous frame loop for game engines and real-time rendering.
- `BlockingPump` — event-driven loop for desktop applications; thread sleeps when idle.
- `MainThreadQueue` — thread-safe queue for posting work from any thread to be executed on the main thread.

**Widget Layer**
- `Window` — top-level Win32 window widget. Owns the HWND (via RAII `HwndWrapper`), the `Renderer`, and a list of `Panel` children. Default size 800x600.
- `Panel` — rectangular container widget. Manages child widget ownership and computes absolute screen positions recursively.
- `Label` — static text widget with configurable font family, size, style, and RGBA color.
- `Button` — interactive widget with a `Normal` / `Hovered` / `Disabled` state machine and a click callback.
- `WidgetBase` — abstract base class with position, size, visibility, and dirty-flag management.
- `Color` struct — RGBA color with common color constants.
- `FontConfig` struct — font family, point size, bold, and italic flags.

**Rendering**
- `Renderer` — Direct2D backend. Wraps `ID2D1Factory`, `ID2D1HwndRenderTarget`, and `IDWriteFactory` via `ComPtr`.
- Frame lifecycle: `begin_frame()` / draw calls / `end_frame()`.
- Draw primitives: `clear()`, `draw_rect()`, `draw_text()`.
- Automatic render target recreation on `D2DERR_RECREATE_TARGET` (device loss).
- Text clipping to widget bounding box.

**Event System**
- `EventDispatcher` — O(1) handler lookup via `std::array` indexed by `EventType`. Supports multiple subscribers per event type. Exception-safe dispatch.
- `EventType` enum: `Click`, `Resize`, `Close`, `KeyDown`, `KeyUp`, `MouseEnter`, `MouseLeave`.
- Payload structs: `ClickPayload`, `ResizePayload`, `KeyPayload`, `ClosePayload`, `MouseEnterPayload`, `MouseLeavePayload`.
- `HandlerId` for selective unsubscribe.

**Metric Layer**
- `Pos<T>` — 2D position with automatic clamping to `[-MAX, MAX]`.
- `Size<T>` — 2D dimensions with automatic clamping to `[0, MAX]`.
- `Rect<T>` — flat rectangle with `contains()`, `right()`, and `bottom()`.
- `basic_pair<Derived>` — CRTP base providing all arithmetic operators with clamping.

**Error Handling**
- `ErrorCode` enum with 10 distinct codes.
- `Error` struct with code, message, and native Win32 error code.
- `is_fatal()` classification for unrecoverable errors.
- `make_error()` and `make_win32_error()` factory helpers.
- `std::expected<T, Error>` used throughout all fallible public API.

**Logging**
- `Logger` singleton with `Level` (Trace–Fatal) and `Domain` (Core, UI, Render, Event, IO, Parser) filtering.
- Compile-time level elimination via `if constexpr`.
- Pluggable sinks via `std::function<void(const LogEntry&)>`.
- `ZK_LOG(level, domain, msg)` macro with automatic file and line capture.

**Serialization / Deserialization**
- `WidgetSerializer` — converts a widget hierarchy to a human-readable indentation-based text format.
- `WidgetParser` — reconstructs a widget hierarchy from the text format. Returns `ParseError` with line and column on malformed input.
- Round-trip stable format.

**Utilities**
- `NativeWrapper<Handle, Deleter>` — RAII wrapper for Win32 handles. Move-only.
- `HwndWrapper` — pre-defined alias for `NativeWrapper<HWND, &DestroyWindow>`.
- `Arithmetic` concept for template constraints.
- `clamp()` utility function.
- `to_string()` overloads for all framework `enum class` types.
- `to_underlying()` for explicit enum-to-numeric conversion.

**Examples**
- `example_hello_window` — minimal blocking-mode application with a Label.
- `example_game_loop` — non-blocking continuous-render loop with live FPS counter.

**Tests**
- 16 test binaries covering all modules: error, log, metric, util, event, ui, render, io, core.
- Property-based tests for `Pos`/`Size` clamping, arithmetic safety, and widget I/O round-trips.
