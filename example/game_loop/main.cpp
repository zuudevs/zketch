/// game_loop — zketch continuous-render example
///
/// Demonstrates the NonBlocking (game engine) message pump:
///   - Application in NonBlocking mode — frame callback fires every iteration
///   - FPS counter computed from wall-clock time
///   - FPS value displayed live in a Label widget
///   - ESC key closes the window
///
/// Build:  cmake --build build --target example_game_loop
/// Run:    ./bin/example_game_loop
///
/// Requirements: 2.1, 2.2

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>   // QueryPerformanceCounter / QueryPerformanceFrequency

#include "zk/core/application.hpp"
#include "zk/log/logger.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"
#include "zk/ui/label.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/window.hpp"

#include <memory>
#include <string>

// ---------------------------------------------------------------------------
// Simple high-resolution frame timer using Win32 QPC
// ---------------------------------------------------------------------------
namespace {

struct FrameTimer {
    LARGE_INTEGER freq{};
    LARGE_INTEGER last{};
    int           frame_count{ 0 };
    double        elapsed_sec{ 0.0 };
    double        fps{ 0.0 };

    FrameTimer() noexcept {
        ::QueryPerformanceFrequency(&freq);
        ::QueryPerformanceCounter(&last);
    }

    /// Call once per frame.  Returns true when the FPS value has been updated
    /// (every ~0.5 s) so the caller knows to refresh the label.
    bool tick() noexcept {
        LARGE_INTEGER now{};
        ::QueryPerformanceCounter(&now);

        const double delta =
            static_cast<double>(now.QuadPart - last.QuadPart) /
            static_cast<double>(freq.QuadPart);

        last = now;
        ++frame_count;
        elapsed_sec += delta;

        if (elapsed_sec >= 0.5) {
            fps         = static_cast<double>(frame_count) / elapsed_sec;
            frame_count = 0;
            elapsed_sec = 0.0;
            return true;   // FPS value refreshed
        }
        return false;
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    // -----------------------------------------------------------------------
    // 1. Create Application in NonBlocking mode (Req 2.1)
    //    NonBlocking mode uses PeekMessage — the loop runs continuously
    //    every frame without waiting for OS events.
    // -----------------------------------------------------------------------
    auto app_result = zk::core::Application::create(
        "GameLoopClass",
        zk::core::PumpMode::NonBlocking
    );
    if (!app_result) {
        ZK_LOG(zk::log::Level::Fatal, zk::log::Domain::Core,
               "Failed to create Application");
        return 1;
    }
    auto& app = *app_result;

    app.set_error_handler([](const zk::error::Error& err) {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Core, err.message);
    });

    // -----------------------------------------------------------------------
    // 2. Create Window 800×600
    // -----------------------------------------------------------------------
    auto win_result = zk::ui::Window::create(
        "zketch — Game Loop (NonBlocking)",
        zk::metric::Pos<int>{ 200, 150 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );
    if (!win_result) {
        ZK_LOG(zk::log::Level::Fatal, zk::log::Domain::Core,
               "Failed to create Window");
        return 1;
    }
    auto& win = *win_result;

    // -----------------------------------------------------------------------
    // 3. Build widget hierarchy
    //    Panel → fps_label (live FPS counter)
    //          → hint_label (static instructions)
    // -----------------------------------------------------------------------
    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );

    // FPS display label — updated every ~0.5 s from the frame callback
    auto fps_label = std::make_unique<zk::ui::Label>(
        "FPS: --",
        zk::metric::Pos<int>{ 20, 20 },
        zk::metric::Size<uint16_t>{ 400, 60 }
    );
    fps_label->set_font(zk::ui::FontConfig{ "Segoe UI", 32.0f, true, false });
    fps_label->set_color(zk::ui::colors::DarkGray);

    // Static hint label
    auto hint_label = std::make_unique<zk::ui::Label>(
        "Press ESC to quit",
        zk::metric::Pos<int>{ 20, 100 },
        zk::metric::Size<uint16_t>{ 400, 30 }
    );
    hint_label->set_font(zk::ui::FontConfig{ "Segoe UI", 14.0f, false, true });
    hint_label->set_color(zk::ui::colors::Gray);

    // Keep a raw pointer to fps_label before transferring ownership
    zk::ui::Label* fps_ptr = fps_label.get();

    panel->add_child(std::move(fps_label));
    panel->add_child(std::move(hint_label));
    win.add_panel(std::move(panel));

    // -----------------------------------------------------------------------
    // 4. Register window events
    // -----------------------------------------------------------------------
    win.on_close([]() {
        ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core, "Window closed");
    });

    win.on_key_down([](uint32_t vkey) {
        if (vkey == VK_ESCAPE) {
            ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
                   "ESC pressed — quitting");
            ::PostQuitMessage(0);
        }
    });

    // -----------------------------------------------------------------------
    // 5. Register frame callback (Req 2.2)
    //    Called every iteration of the NonBlocking pump, regardless of
    //    whether any Win32 messages were pending.
    // -----------------------------------------------------------------------
    FrameTimer timer{};

    app.set_frame_callback([&timer, fps_ptr]() {
        // Tick the timer; update the label only when FPS is refreshed
        if (timer.tick()) {
            const std::string text =
                "FPS: " + std::to_string(static_cast<int>(timer.fps + 0.5));
            fps_ptr->set_text(text);
        }
    });

    // -----------------------------------------------------------------------
    // 6. Show window and run the non-blocking message loop
    // -----------------------------------------------------------------------
    win.show();

    auto result = app.run();
    return result.value_or(1);
}
