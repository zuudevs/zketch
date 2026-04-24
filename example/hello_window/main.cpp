/// hello_window — zketch minimal example
///
/// Demonstrates the simplest possible zketch application:
///   - Application in Blocking (event-driven) mode
///   - A single 800×600 Window
///   - A Label displaying "Hello, zketch!"
///   - ESC key closes the window
///
/// Build:  cmake --build build --target example_hello_window
/// Run:    ./bin/example_hello_window
///
/// Requirements: 1.1, 4.1, 7.1

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

int main()
{
    // -----------------------------------------------------------------------
    // 1. Create Application in Blocking mode (Req 1.1, 1.4)
    //    Blocking mode uses GetMessage — the thread sleeps when idle,
    //    making it ideal for desktop applications.
    // -----------------------------------------------------------------------
    auto app_result = zk::core::Application::create(
        "HelloWindowClass",
        zk::core::PumpMode::Blocking
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
    // 2. Create Window 800×600 (Req 4.1)
    // -----------------------------------------------------------------------
    auto win_result = zk::ui::Window::create(
        "Hello, zketch!",
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
    //    Panel (full client area) → Label "Hello, zketch!" (Req 7.1)
    // -----------------------------------------------------------------------
    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 800, 600 }
    );

    auto label = std::make_unique<zk::ui::Label>(
        "Hello, zketch!",
        zk::metric::Pos<int>{ 0, 260 },          // vertically centred in 600px
        zk::metric::Size<uint16_t>{ 800, 80 }
    );
    label->set_font(zk::ui::FontConfig{ "Segoe UI", 36.0f, true, false });
    label->set_color(zk::ui::colors::DarkGray);

    panel->add_child(std::move(label));
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
    // 5. Show window and run the blocking message loop
    // -----------------------------------------------------------------------
    win.show();

    auto result = app.run();
    return result.value_or(1);
}
