# Roadmap

This document outlines the planned direction for zketch beyond the v1.0 MVP. Items are grouped by theme and roughly ordered by priority, but nothing here is a firm commitment or timeline.

> **Status:** v1.0.0 was released on 2026-04-24. All items below target future versions.

---

## v1.1 — Widget and Rendering Improvements

These are natural extensions of the v1.0 foundation that do not require architectural changes.

- **TextInput widget** — single-line editable text field with cursor, selection, and clipboard support.
- **ScrollPanel widget** — scrollable container with vertical and horizontal scroll bars.
- **Image widget** — display bitmaps loaded from file (PNG, BMP) using WIC (Windows Imaging Component).
- **Checkbox and RadioButton widgets** — common form controls.
- **Layout system** — declarative row/column layout helpers to reduce manual position arithmetic.
- **Widget theming** — a centralized theme object that widgets query for colors, fonts, and spacing, replacing per-widget style properties.
- **Renderer: rounded rectangles and ellipses** — additional Direct2D draw primitives.
- **Renderer: opacity and layering** — per-widget alpha and compositing support.

---

## v1.2 — Input and Interaction

- **Mouse position tracking** — expose cursor position to widgets for hover detection without requiring WndProc changes per widget.
- **Drag and drop** — basic drag-and-drop support between widgets using Win32 OLE drag-and-drop.
- **Keyboard focus management** — tab order, focus ring, and keyboard navigation between interactive widgets.
- **Tooltip support** — hover-triggered text tooltips.
- **Context menus** — right-click popup menus.

---

## v1.3 — Serialization and Tooling

- **Hot reload** — watch the widget config file for changes and reload the widget hierarchy at runtime without restarting the application.
- **Widget config format v2** — extend the format to support theming, layout hints, and event bindings.
- **Schema validation** — validate widget config files against a schema before parsing, with structured error reporting.
- **Code generation** — generate C++ widget construction code from a widget config file.

---

## v1.4 — Performance and Diagnostics

- **Dirty region rendering** — only redraw widgets that have changed, rather than clearing and redrawing the entire frame.
- **Frame profiler** — built-in frame timing overlay showing CPU time per widget render call.
- **Memory diagnostics** — optional leak detection and allocation tracking in debug builds.
- **Benchmark suite** — automated benchmarks for rendering throughput, event dispatch latency, and serialization speed.

---

## Future Considerations (No Version Assigned)

These items require significant design work or have dependencies that are not yet resolved.

- **Multi-window support** — multiple top-level windows within a single application instance.
- **Multi-threaded rendering** — offload rendering to a dedicated render thread while keeping UI logic on the main thread.
- **Accessibility** — Microsoft UI Automation (UIA) provider implementation for screen reader support.
- **High-DPI support** — per-monitor DPI awareness and automatic scaling.
- **Animation system** — time-based property animations with easing functions.
- **Cross-platform support** — Linux (Wayland/X11) and macOS (Metal/Cocoa) backends. This is explicitly out of scope for v1.0 and would require significant architectural changes to the rendering and windowing layers.
- **Scripting integration** — embed a scripting language (Lua or Python) for runtime widget logic.
- **Visual designer** — a graphical tool for building widget hierarchies and generating widget config files.

---

## Constraints (v1.0 — Shipped)

These were explicitly out of scope for v1.0 and remain deferred until a future version:

- Multiple rendering backends — Direct2D is the only backend.
- Cross-platform support — Windows only.
- Multi-threaded UI operations — all UI runs on the main thread.
- Web or mobile targets.

---

## Contributing to the Roadmap

If you have a feature request or want to work on something listed here, open an issue on GitHub to discuss the design before starting implementation. See [CONTRIBUTING.md](../CONTRIBUTING.md) for the contribution process.
