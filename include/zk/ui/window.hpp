#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM

#include "zk/error/error.hpp"
#include "zk/event/event_dispatcher.hpp"
#include "zk/event/event_types.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"
#include "zk/render/renderer.hpp"
#include "zk/ui/panel.hpp"
#include "zk/util/native_wrapper.hpp"

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace zk::ui {

/// Top-level Win32 window widget.
///
/// ## Lifecycle
///   1. Call `Window::create(title, pos, size)` — registers the window class
///      (if not already registered) and calls `CreateWindowEx`.
///   2. Call `show()` to make the window visible.
///   3. Register event handlers via `on_resize`, `on_close`, `on_key_down`,
///      `on_key_up` before entering the message loop.
///   4. The static `WndProc` dispatches Win32 messages to the `EventDispatcher`
///      stored inside the Window instance.
///
/// ## Ownership
///   - `add_panel()` transfers `unique_ptr<Panel>` ownership into the Window.
///   - The `Renderer` is owned by the Window and created lazily on first show.
///
/// ## Default size
///   If `size.w == 0 || size.h == 0` the window is created at 800×600 (Req 4.3).
///
/// ## HWND lifetime
///   The `HWND` is stored in an `HwndWrapper` (RAII via `DestroyWindow`).
///   On `WM_DESTROY` the WndProc calls `PostQuitMessage(0)` (Req 4.5).
///
/// Satisfies Requirements: 4.1–4.7, 9.3–9.5, 21.4, 21.5
class Window {
public:
    // -----------------------------------------------------------------------
    // Factory — the only way to construct a valid Window (Req 4.6)
    // -----------------------------------------------------------------------

    /// Creates a Win32 window with the given title, position, and size.
    ///
    /// If `size.w == 0 || size.h == 0` the window is created at 800×600 (Req 4.3).
    /// Returns an error if `CreateWindowEx` fails (Req 4.6).
    [[nodiscard]] static std::expected<Window, error::Error> create(
        std::string_view        title,
        metric::Pos<int>        pos  = metric::Pos<int>{ CW_USEDEFAULT, CW_USEDEFAULT },
        metric::Size<uint16_t>  size = metric::Size<uint16_t>{ 0, 0 }
    ) noexcept;

    // -----------------------------------------------------------------------
    // Deserialization factory — constructs a logical Window without a real HWND.
    // Used by WidgetParser to reconstruct a widget hierarchy from config text.
    // The returned Window has no HWND; calling show() on it will fail gracefully.
    // -----------------------------------------------------------------------

    /// Constructs a Window with the given title, position, and size without
    /// calling CreateWindowEx.  Intended exclusively for deserialization.
    [[nodiscard]] static Window create_from_config(
        std::string_view       title,
        metric::Pos<int>       pos,
        metric::Size<uint16_t> size) noexcept;

    // -----------------------------------------------------------------------
    // Special members — move-only (HWND ownership is unique)
    // -----------------------------------------------------------------------
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;

    ~Window() noexcept = default;

    // -----------------------------------------------------------------------
    // Visibility (Req 4.7)
    // -----------------------------------------------------------------------

    /// Shows the window (SW_SHOW) and initialises the renderer on first call.
    void show() noexcept;

    /// Hides the window (SW_HIDE).
    void hide() noexcept;

    /// Minimises the window (SW_MINIMIZE).
    void minimize() noexcept;

    /// Maximises the window (SW_MAXIMIZE).
    void maximize() noexcept;

    // -----------------------------------------------------------------------
    // Child panels — Req 6.1, 17.5
    // -----------------------------------------------------------------------

    /// Transfers ownership of `panel` into this Window.
    /// Returns a non-owning reference to the added panel.
    Panel& add_panel(std::unique_ptr<Panel> panel);

    // -----------------------------------------------------------------------
    // Event registration — Req 9.3, 9.4, 9.5
    // -----------------------------------------------------------------------

    /// Registers a callback invoked when the window is resized.
    /// The new size is passed as the argument (Req 9.4).
    void on_resize(std::function<void(metric::Size<uint16_t>)> handler);

    /// Registers a callback invoked when the window is about to close.
    void on_close(std::function<void()> handler);

    /// Registers a callback invoked when a key is pressed.
    /// The Win32 virtual key code is passed as the argument (Req 9.5).
    void on_key_down(std::function<void(uint32_t vkey)> handler);

    /// Registers a callback invoked when a key is released.
    /// The Win32 virtual key code is passed as the argument (Req 9.5).
    void on_key_up(std::function<void(uint32_t vkey)> handler);

    // -----------------------------------------------------------------------
    // State queries
    // -----------------------------------------------------------------------

    /// Returns the window title.
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

    /// Returns the current window size.
    [[nodiscard]] metric::Size<uint16_t> size() const noexcept { return size_; }

    /// Returns the current window position.
    [[nodiscard]] metric::Pos<int> pos() const noexcept { return pos_; }

    /// Returns true if the underlying HWND is valid.
    [[nodiscard]] bool is_valid() const noexcept { return hwnd_.valid(); }

    // -----------------------------------------------------------------------
    // Win32 window procedure — public so it can be passed to WNDCLASSEXW
    // -----------------------------------------------------------------------

    /// Static WndProc registered with the window class.
    /// Delegates to wnd_proc_trampoline which routes to the instance handler.
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg,
                                     WPARAM wparam, LPARAM lparam) noexcept;

    // -----------------------------------------------------------------------
    // Advanced — native handle access (Req 21.4, 21.5)
    // -----------------------------------------------------------------------

    /// Returns the underlying HWND for advanced/custom Win32 usage.
    /// WARNING: Manipulating the HWND directly can violate framework invariants.
    [[nodiscard]] HWND native_handle() const noexcept { return hwnd_.get(); }

    // -----------------------------------------------------------------------
    // Renderer access (used by the render loop)
    // -----------------------------------------------------------------------

    /// Returns a pointer to the renderer, or nullptr if not yet initialised.
    [[nodiscard]] render::Renderer* renderer() noexcept { return renderer_.get(); }

    /// Returns a read-only view of the top-level panels owned by this Window.
    /// Used by WidgetSerializer to traverse the widget hierarchy.
    [[nodiscard]] const std::vector<std::unique_ptr<Panel>>& panels() const noexcept {
        return panels_;
    }
private:
    // Private constructor — use create()
    Window() noexcept = default;

    // -----------------------------------------------------------------------
    // Win32 window procedure (private implementation details)
    // -----------------------------------------------------------------------

    /// Internal trampoline — retrieves the Window* from GWLP_USERDATA and
    /// calls handle_message.
    static LRESULT CALLBACK wnd_proc_trampoline(HWND hwnd, UINT msg,
                                                WPARAM wparam, LPARAM lparam) noexcept;

    /// Instance-level message handler called from wnd_proc.
    LRESULT handle_message(HWND hwnd, UINT msg,
                           WPARAM wparam, LPARAM lparam) noexcept;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// Ensures the renderer is created for this window's HWND.
    /// Called on first show(). Logs an error if creation fails.
    void ensure_renderer() noexcept;

    /// Dispatches a mouse position to all child panels to update button hover
    /// state. Called on WM_MOUSEMOVE.
    void dispatch_mouse_move(int x, int y) noexcept;

    /// Dispatches a left-button click to all child panels.
    /// Called on WM_LBUTTONUP.
    void dispatch_click(int x, int y) noexcept;

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------
    util::HwndWrapper                        hwnd_{};
    std::string                              title_{};
    metric::Pos<int>                         pos_{};
    metric::Size<uint16_t>                   size_{};

    std::vector<std::unique_ptr<Panel>>      panels_{};
    std::unique_ptr<render::Renderer>        renderer_{};

    event::EventDispatcher                   dispatcher_{};

    // Stored handler IDs so we can unsubscribe if needed in the future.
    event::HandlerId resize_handler_id_   { 0 };
    event::HandlerId close_handler_id_    { 0 };
    event::HandlerId key_down_handler_id_ { 0 };
    event::HandlerId key_up_handler_id_   { 0 };

    // Track whether the renderer has been initialised.
    bool renderer_initialised_{ false };
};

} // namespace zk::ui
