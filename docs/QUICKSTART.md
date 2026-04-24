# Quickstart

This guide walks through building two complete zketch applications in about 5 minutes: a simple "Hello, zketch!" window and a live FPS counter using the game loop mode.

---

## Prerequisites

Make sure you have built the project first. See [BUILD.md](../BUILD.md) for full instructions. The short version:

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang-cl
cmake --build build
```

---

## Example 1 — Hello Window (Blocking Mode)

This is the simplest possible zketch application. It creates a window, displays a label, and exits when you press Escape.

```cpp
#include "zk/core/application.hpp"
#include "zk/ui/window.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/label.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"
#include "zk/log/logger.hpp"

int main()
{
    // 1. Create the application in Blocking mode.
    //    Blocking mode uses GetMessage — the thread sleeps when idle.
    //    This is the right choice for desktop applications.
    auto app_result = zk::core::Application::create(
        "HelloWindowClass",
        zk::core::PumpMode::Blocking
    );
    if (!app_result) {
        return 1;
    }
    auto& app = *app_result;

    // 2. Create a window.
    //    If size is {0, 0}, the default 800x600 is used.
    auto win_result = zk::ui::Window::create(
        "Hello, zketch!",
        zk::metric::Pos<int>{ 100, 100 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );
    if (!win_result) {
        return 1;
    }
    auto& win = *win_result;

    // 3. Build the widget hierarchy.
    //    Panel is a container; Label is a child of the Panel.
    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );

    auto label = std::make_unique<zk::ui::Label>(
        "Hello, zketch!",
        zk::metric::Pos<int>{ 20, 20 },
        zk::metric::Size<uint16_t>{ 400, 40 }
    );

    panel->add_child(std::move(label));
    win.add_panel(std::move(panel));

    // 4. Register event handlers.
    win.on_key_down([](uint32_t vkey) {
        if (vkey == VK_ESCAPE) {
            PostQuitMessage(0);
        }
    });

    // 5. Show the window and run the message loop.
    win.show();
    app.run();

    return 0;
}
```

Run it:

```bash
./bin/example_hello_window
```

---

## Example 2 — Game Loop (NonBlocking Mode)

This example uses the non-blocking pump to run a continuous frame loop. It measures FPS using the Win32 high-resolution timer and updates a Label every half second.

```cpp
#include "zk/core/application.hpp"
#include "zk/ui/window.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/label.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

#include <windows.h>
#include <memory>
#include <string>

int main()
{
    // 1. Create the application in NonBlocking mode.
    //    NonBlocking mode uses PeekMessage — the loop runs every frame
    //    even when no input events are pending.
    auto app_result = zk::core::Application::create(
        "GameLoopClass",
        zk::core::PumpMode::NonBlocking
    );
    if (!app_result) return 1;
    auto& app = *app_result;

    // 2. Create the window.
    auto win_result = zk::ui::Window::create(
        "Game Loop — zketch",
        zk::metric::Pos<int>{ 100, 100 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );
    if (!win_result) return 1;
    auto& win = *win_result;

    // 3. Build the widget hierarchy.
    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );

    auto fps_label = std::make_unique<zk::ui::Label>(
        "FPS: --",
        zk::metric::Pos<int>{ 10, 10 },
        zk::metric::Size<uint16_t>{ 200, 30 }
    );
    auto* fps_ptr = fps_label.get();  // non-owning pointer for the callback

    panel->add_child(std::move(fps_label));
    win.add_panel(std::move(panel));

    // 4. Set up a simple frame timer.
    LARGE_INTEGER freq{}, last{};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    int frame_count = 0;

    // 5. Register the frame callback.
    //    This is called once per loop iteration in NonBlocking mode.
    app.set_frame_callback([&]() {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        double elapsed = static_cast<double>(now.QuadPart - last.QuadPart)
                       / static_cast<double>(freq.QuadPart);
        ++frame_count;

        if (elapsed >= 0.5) {
            double fps = frame_count / elapsed;
            fps_ptr->set_text("FPS: " + std::to_string(static_cast<int>(fps)));
            frame_count = 0;
            last = now;
        }
    });

    // 6. Register ESC to quit.
    win.on_key_down([](uint32_t vkey) {
        if (vkey == VK_ESCAPE) PostQuitMessage(0);
    });

    // 7. Show and run.
    win.show();
    app.run();

    return 0;
}
```

Run it:

```bash
./bin/example_game_loop
```

---

## Key Concepts

### Pump Modes

| Mode | Win32 API | Use Case |
|---|---|---|
| `PumpMode::Blocking` | `GetMessage` | Desktop apps — thread sleeps when idle |
| `PumpMode::NonBlocking` | `PeekMessage` | Game loops — runs every frame |

### Error Handling

All factory methods return `std::expected<T, zk::error::Error>`. Check the result before using the value:

```cpp
auto result = zk::ui::Window::create("Title", pos, size);
if (!result) {
    const auto& err = result.error();
    // err.code         — ErrorCode enum
    // err.message      — human-readable string
    // err.native_error — GetLastError() or HRESULT
    return 1;
}
auto& win = *result;  // dereference to get the Window value
```

### Widget Ownership

Ownership transfers into the parent on `add_child()` and `add_panel()`. After the call, the `unique_ptr` you passed is empty. The parent is responsible for the lifetime of all its children.

```cpp
auto label = std::make_unique<zk::ui::Label>("text", pos, size);
auto* raw = label.get();   // save a non-owning pointer if you need it later
panel->add_child(std::move(label));
// label is now empty; use raw to access the widget
```

### Logging

```cpp
ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core, "Application started");

// Add a console sink
zk::log::Logger::instance().add_sink([](const zk::log::LogEntry& e) {
    std::cout << e.message << "\n";
});
```

---

## Next Steps

- Read [docs/API.md](API.md) for the full public API reference.
- Read [docs/ARCHITECTURE.md](ARCHITECTURE.md) to understand how the layers fit together.
- Browse the `example/` folder for the full source of both examples.
