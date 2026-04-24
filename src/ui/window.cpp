#include "zk/ui/window.hpp"

#include "zk/log/logger.hpp"
#include "zk/ui/button.hpp"

#include <utility>     // std::move, std::exchange
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM

namespace zk::ui {

// ---------------------------------------------------------------------------
// Internal constants
// ---------------------------------------------------------------------------

namespace {

/// Default window dimensions used when size.w == 0 || size.h == 0 (Req 4.3).
constexpr uint16_t kDefaultWidth  = 800;
constexpr uint16_t kDefaultHeight = 600;

/// Win32 window class name used for all zketch windows.
constexpr const wchar_t* kWindowClassName = L"ZketchWindow";

/// Registers the Win32 window class if it has not been registered yet.
/// Returns true on success or if already registered.
bool ensure_window_class_registered() noexcept {
    // Check if already registered — GetClassInfoExW returns non-zero if found.
    WNDCLASSEXW existing{};
    existing.cbSize = sizeof(WNDCLASSEXW);
    if (::GetClassInfoExW(::GetModuleHandleW(nullptr), kWindowClassName, &existing)) {
        return true;
    }

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Window::wnd_proc;
    wc.hInstance     = ::GetModuleHandleW(nullptr);
    wc.hCursor       = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)); // IDC_ARROW
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWindowClassName;

    return ::RegisterClassExW(&wc) != 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Deserialization factory (no Win32 call)
// ---------------------------------------------------------------------------

Window Window::create_from_config(
    std::string_view       title,
    metric::Pos<int>       pos,
    metric::Size<uint16_t> size) noexcept
{
    const uint16_t w = (size.w == 0 || size.h == 0) ? kDefaultWidth  : size.w;
    const uint16_t h = (size.w == 0 || size.h == 0) ? kDefaultHeight : size.h;

    Window win{};
    win.title_ = std::string{ title };
    win.pos_   = pos;
    win.size_  = metric::Size<uint16_t>{ w, h };
    // hwnd_ remains default-constructed (invalid) — no OS window is created.
    return win;
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::expected<Window, error::Error> Window::create(
    std::string_view        title,
    metric::Pos<int>        pos,
    metric::Size<uint16_t>  size) noexcept
{
    // Apply default size if either dimension is zero (Req 4.3).
    const uint16_t w = (size.w == 0 || size.h == 0) ? kDefaultWidth  : size.w;
    const uint16_t h = (size.w == 0 || size.h == 0) ? kDefaultHeight : size.h;

    // Register the window class (idempotent).
    if (!ensure_window_class_registered()) {
        return std::unexpected(error::make_win32_error(
            error::ErrorCode::WindowCreationFailed,
            "Failed to register window class"));
    }

    // Convert UTF-8 title to UTF-16 for Win32.
    const int wlen = ::MultiByteToWideChar(
        CP_UTF8, 0, title.data(), static_cast<int>(title.size()), nullptr, 0);
    std::wstring wtitle(static_cast<std::size_t>(wlen), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0, title.data(), static_cast<int>(title.size()),
        wtitle.data(), wlen);

    // Create the Win32 window (Req 4.2).
    HWND hwnd = ::CreateWindowExW(
        0,                          // dwExStyle
        kWindowClassName,           // lpClassName
        wtitle.c_str(),             // lpWindowName
        WS_OVERLAPPEDWINDOW,        // dwStyle
        pos.x,                      // X
        pos.y,                      // Y
        static_cast<int>(w),        // nWidth
        static_cast<int>(h),        // nHeight
        nullptr,                    // hWndParent
        nullptr,                    // hMenu
        ::GetModuleHandleW(nullptr),// hInstance
        nullptr                     // lpParam — we set GWLP_USERDATA after construction
    );

    if (!hwnd) {
        return std::unexpected(error::make_win32_error(
            error::ErrorCode::WindowCreationFailed,
            "CreateWindowEx failed"));
    }

    Window win{};
    win.hwnd_  = util::HwndWrapper{ hwnd };
    win.title_ = std::string{ title };
    win.pos_   = pos;
    win.size_  = metric::Size<uint16_t>{ w, h };

    // Store a pointer to the Window instance in the HWND's user data so that
    // wnd_proc can route messages to the correct instance.
    ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(&win));

    return win;
}

// ---------------------------------------------------------------------------
// Special members
// ---------------------------------------------------------------------------

Window::Window(Window&& other) noexcept
    : hwnd_                  (std::move(other.hwnd_))
    , title_                 (std::move(other.title_))
    , pos_                   (other.pos_)
    , size_                  (other.size_)
    , panels_                (std::move(other.panels_))
    , renderer_              (std::move(other.renderer_))
    , dispatcher_            (std::move(other.dispatcher_))
    , resize_handler_id_     (other.resize_handler_id_)
    , close_handler_id_      (other.close_handler_id_)
    , key_down_handler_id_   (other.key_down_handler_id_)
    , key_up_handler_id_     (other.key_up_handler_id_)
    , renderer_initialised_  (other.renderer_initialised_)
{
    // Update GWLP_USERDATA to point to the new location of this object.
    if (hwnd_.valid()) {
        ::SetWindowLongPtrW(hwnd_.get(), GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(this));
    }
    // Invalidate the moved-from object's HWND user-data pointer.
    other.renderer_initialised_ = false;
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this == &other) return *this;

    hwnd_                 = std::move(other.hwnd_);
    title_                = std::move(other.title_);
    pos_                  = other.pos_;
    size_                 = other.size_;
    panels_               = std::move(other.panels_);
    renderer_             = std::move(other.renderer_);
    dispatcher_           = std::move(other.dispatcher_);
    resize_handler_id_    = other.resize_handler_id_;
    close_handler_id_     = other.close_handler_id_;
    key_down_handler_id_  = other.key_down_handler_id_;
    key_up_handler_id_    = other.key_up_handler_id_;
    renderer_initialised_ = other.renderer_initialised_;

    if (hwnd_.valid()) {
        ::SetWindowLongPtrW(hwnd_.get(), GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(this));
    }
    other.renderer_initialised_ = false;
    return *this;
}

// ---------------------------------------------------------------------------
// Visibility (Req 4.7)
// ---------------------------------------------------------------------------

void Window::show() noexcept
{
    if (!hwnd_.valid()) return;
    ensure_renderer();
    ::ShowWindow(hwnd_.get(), SW_SHOW);
    ::UpdateWindow(hwnd_.get());
}

void Window::hide() noexcept
{
    if (!hwnd_.valid()) return;
    ::ShowWindow(hwnd_.get(), SW_HIDE);
}

void Window::minimize() noexcept
{
    if (!hwnd_.valid()) return;
    ::ShowWindow(hwnd_.get(), SW_MINIMIZE);
}

void Window::maximize() noexcept
{
    if (!hwnd_.valid()) return;
    ::ShowWindow(hwnd_.get(), SW_MAXIMIZE);
}

// ---------------------------------------------------------------------------
// Child panels
// ---------------------------------------------------------------------------

Panel& Window::add_panel(std::unique_ptr<Panel> panel)
{
    Panel* raw = panel.get();
    panels_.push_back(std::move(panel));
    return *raw;
}

// ---------------------------------------------------------------------------
// Event registration (Req 9.3, 9.4, 9.5)
// ---------------------------------------------------------------------------

void Window::on_resize(std::function<void(metric::Size<uint16_t>)> handler)
{
    if (resize_handler_id_ != 0) {
        dispatcher_.unsubscribe(event::EventType::Resize, resize_handler_id_);
    }
    resize_handler_id_ = dispatcher_.subscribe<event::ResizePayload>(
        event::EventType::Resize,
        [h = std::move(handler)](const event::ResizePayload& p) {
            h(p.size);
        });
}

void Window::on_close(std::function<void()> handler)
{
    if (close_handler_id_ != 0) {
        dispatcher_.unsubscribe(event::EventType::Close, close_handler_id_);
    }
    close_handler_id_ = dispatcher_.subscribe<event::ClosePayload>(
        event::EventType::Close,
        [h = std::move(handler)](const event::ClosePayload&) {
            h();
        });
}

void Window::on_key_down(std::function<void(uint32_t)> handler)
{
    if (key_down_handler_id_ != 0) {
        dispatcher_.unsubscribe(event::EventType::KeyDown, key_down_handler_id_);
    }
    key_down_handler_id_ = dispatcher_.subscribe<event::KeyPayload>(
        event::EventType::KeyDown,
        [h = std::move(handler)](const event::KeyPayload& p) {
            h(p.vkey);
        });
}

void Window::on_key_up(std::function<void(uint32_t)> handler)
{
    if (key_up_handler_id_ != 0) {
        dispatcher_.unsubscribe(event::EventType::KeyUp, key_up_handler_id_);
    }
    key_up_handler_id_ = dispatcher_.subscribe<event::KeyPayload>(
        event::EventType::KeyUp,
        [h = std::move(handler)](const event::KeyPayload& p) {
            h(p.vkey);
        });
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Window::ensure_renderer() noexcept
{
    if (renderer_initialised_ || !hwnd_.valid()) return;

    auto result = render::Renderer::create(hwnd_.get());
    if (result) {
        renderer_ = std::make_unique<render::Renderer>(std::move(*result));
        renderer_initialised_ = true;
    } else {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Render,
               "Failed to create renderer for window");
    }
}

void Window::dispatch_mouse_move(int x, int y) noexcept
{
    // Walk all panels and their children to update button hover state.
    for (auto& panel : panels_) {
        if (!panel) continue;
        for (auto& child : panel->children()) {
            if (!child) continue;
            auto* btn = dynamic_cast<Button*>(child.get());
            if (!btn || !btn->is_visible()) continue;

            const auto& bpos  = btn->pos();
            const auto& bsize = btn->size();
            const bool  hit   = (x >= bpos.x && x < bpos.x + bsize.w &&
                                  y >= bpos.y && y < bpos.y + bsize.h);
            if (hit) {
                btn->on_mouse_enter();
            } else {
                btn->on_mouse_leave();
            }
        }
    }
}

void Window::dispatch_click(int x, int y) noexcept
{
    for (auto& panel : panels_) {
        if (!panel) continue;
        for (auto& child : panel->children()) {
            if (!child) continue;
            auto* btn = dynamic_cast<Button*>(child.get());
            if (!btn || !btn->is_visible()) continue;

            const auto& bpos  = btn->pos();
            const auto& bsize = btn->size();
            const bool  hit   = (x >= bpos.x && x < bpos.x + bsize.w &&
                                  y >= bpos.y && y < bpos.y + bsize.h);
            if (hit) {
                btn->on_click();
                dispatcher_.dispatch(event::EventType::Click, event::ClickPayload{});
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Win32 window procedure
// ---------------------------------------------------------------------------

// Public static WndProc registered with WNDCLASSEXW.
// Retrieves the Window* from GWLP_USERDATA and routes to handle_message.
LRESULT CALLBACK Window::wnd_proc(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam) noexcept
{
    Window* self = reinterpret_cast<Window*>(
        ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (self) {
        return self->handle_message(hwnd, msg, wparam, lparam);
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

// Private trampoline — delegates to the public wnd_proc.
LRESULT CALLBACK Window::wnd_proc_trampoline(HWND hwnd, UINT msg,
                                              WPARAM wparam, LPARAM lparam) noexcept
{
    return wnd_proc(hwnd, msg, wparam, lparam);
}

LRESULT Window::handle_message(HWND hwnd, UINT msg,
                                WPARAM wparam, LPARAM lparam) noexcept
{
    switch (msg) {
        // -------------------------------------------------------------------
        // WM_PAINT — render all panels and their children
        // -------------------------------------------------------------------
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            ::BeginPaint(hwnd, &ps);

            if (renderer_ && renderer_->is_valid()) {
                renderer_->begin_frame();
                renderer_->clear(ui::Color{ 245, 245, 245, 255 }); // light gray bg

                for (auto& panel : panels_) {
                    if (panel && panel->is_visible()) {
                        panel->render(*renderer_);
                    }
                }

                renderer_->end_frame();

                if (renderer_->needs_redraw()) {
                    renderer_->clear_redraw_flag();
                    ::InvalidateRect(hwnd, nullptr, FALSE);
                }
            }

            ::EndPaint(hwnd, &ps);
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_SIZE — window resized (Req 4.4, 9.3, 9.4)
        // -------------------------------------------------------------------
        case WM_SIZE: {
            const uint16_t new_w = LOWORD(lparam);
            const uint16_t new_h = HIWORD(lparam);
            size_ = metric::Size<uint16_t>{ new_w, new_h };

            dispatcher_.dispatch(event::EventType::Resize,
                                 event::ResizePayload{ size_ });

            // Trigger a repaint after resize
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_DESTROY — window is being destroyed (Req 4.5)
        // -------------------------------------------------------------------
        case WM_DESTROY: {
            dispatcher_.dispatch(event::EventType::Close, event::ClosePayload{});
            ::PostQuitMessage(0);
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_KEYDOWN — key pressed (Req 9.3, 9.5)
        // -------------------------------------------------------------------
        case WM_KEYDOWN: {
            const uint32_t vkey = static_cast<uint32_t>(wparam);
            dispatcher_.dispatch(event::EventType::KeyDown,
                                 event::KeyPayload{ vkey });
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_KEYUP — key released (Req 9.3, 9.5)
        // -------------------------------------------------------------------
        case WM_KEYUP: {
            const uint32_t vkey = static_cast<uint32_t>(wparam);
            dispatcher_.dispatch(event::EventType::KeyUp,
                                 event::KeyPayload{ vkey });
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_MOUSEMOVE — mouse moved; update button hover states
        // -------------------------------------------------------------------
        case WM_MOUSEMOVE: {
            const int mx = GET_X_LPARAM(lparam);
            const int my = GET_Y_LPARAM(lparam);
            dispatch_mouse_move(mx, my);
            ::InvalidateRect(hwnd, nullptr, FALSE); // repaint hover state
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_LBUTTONDOWN — left mouse button pressed (track for click)
        // -------------------------------------------------------------------
        case WM_LBUTTONDOWN: {
            ::SetCapture(hwnd);
            return 0;
        }

        // -------------------------------------------------------------------
        // WM_LBUTTONUP — left mouse button released; fire click (Req 8.2)
        // -------------------------------------------------------------------
        case WM_LBUTTONUP: {
            ::ReleaseCapture();
            const int cx = GET_X_LPARAM(lparam);
            const int cy = GET_Y_LPARAM(lparam);
            dispatch_click(cx, cy);
            ::InvalidateRect(hwnd, nullptr, FALSE); // repaint after state change
            return 0;
        }

        default:
            return ::DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

} // namespace zk::ui
