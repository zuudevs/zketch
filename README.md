# zketch

A C++23 GUI framework built from scratch for Windows, designed with a hybrid architecture that serves two fundamentally different use cases: high-performance game engines requiring continuous rendering, and traditional desktop applications that sleep when idle.

---

## Features

- **Dual message pump modes** — NonBlocking (`PeekMessage`) for game loops, Blocking (`GetMessage`) for desktop apps
- **Direct2D rendering backend** — hardware-accelerated 2D graphics with automatic render target recovery on device loss
- **DirectWrite text rendering** — configurable font family, size, and style per widget
- **Four essential widgets** — `Window`, `Panel`, `Label`, `Button`
- **Callback-based event system** — subscribe/unsubscribe by `HandlerId`, exception-safe dispatch
- **Structured error handling** — `std::expected<T, Error>` throughout; no exceptions as control flow
- **Widget serialization and deserialization** — human-readable indentation-based config format with round-trip stability
- **Structured logging** — 6 levels, 6 domains, compile-time level elimination, pluggable sinks
- **Thread-safe main-thread queue** — post work from any thread, flushed on the main thread each iteration
- **RAII resource management** — `NativeWrapper<T>` for Win32 handles, `ComPtr` for COM objects
- **Type-safe metric layer** — `Pos<T>`, `Size<T>`, `Rect<T>` with automatic clamping and full arithmetic support

---

## Quick Start

```cpp
#include "zk/core/application.hpp"
#include "zk/ui/window.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/label.hpp"

int main()
{
    auto app = zk::core::Application::create("MyApp", zk::core::PumpMode::Blocking);
    if (!app) return 1;

    auto win = zk::ui::Window::create("Hello, zketch!",
                                      zk::metric::Pos<int>{ 100, 100 },
                                      zk::metric::Size<uint16_t>{ 800, 600 });
    if (!win) return 1;

    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 800, 600 });

    auto label = std::make_unique<zk::ui::Label>(
        "Hello, zketch!",
        zk::metric::Pos<int>{ 20, 20 },
        zk::metric::Size<uint16_t>{ 400, 40 });

    panel->add_child(std::move(label));
    win->add_panel(std::move(panel));

    win->on_key_down([](uint32_t vkey) {
        if (vkey == VK_ESCAPE) PostQuitMessage(0);
    });

    win->show();
    app->run();
    return 0;
}
```

See [docs/QUICKSTART.md](docs/QUICKSTART.md) for a full walkthrough, or [BUILD.md](BUILD.md) for build instructions.

---

## Documentation

| Document | Description |
|---|---|
| [BUILD.md](BUILD.md) | How to configure, build, and run the project |
| [docs/QUICKSTART.md](docs/QUICKSTART.md) | 5-minute tutorial |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | High-level design and component overview |
| [docs/API.md](docs/API.md) | Full public API reference |
| [docs/CHANGELOG.md](docs/CHANGELOG.md) | Version history |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Planned features and future direction |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [SECURITY.md](SECURITY.md) | How to report security vulnerabilities |

---

## Platform

Windows only (v1.0). Requires Windows SDK with Direct2D and DirectWrite support.

---

## License

MIT License. See [LICENSE](LICENSE) for details.
