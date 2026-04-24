#include "zk/core/application.hpp"

#include "zk/log/logger.hpp"

#include <utility>  // std::exchange, std::move

namespace zk::core {

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::expected<Application, error::Error> Application::create(
    std::string_view class_name,
    PumpMode         mode) noexcept
{
    // Capture the calling thread as Main_Thread (Req 1.6).
    const DWORD tid = ::GetCurrentThreadId();

    // Convert UTF-8 class name to UTF-16 for Win32 (Req 1.2).
    const int wlen = ::MultiByteToWideChar(
        CP_UTF8, 0,
        class_name.data(), static_cast<int>(class_name.size()),
        nullptr, 0);

    if (wlen <= 0) {
        return std::unexpected(error::make_win32_error(
            error::ErrorCode::InitFailed,
            "Failed to convert class name to UTF-16"));
    }

    std::wstring wname(static_cast<std::size_t>(wlen), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0,
        class_name.data(), static_cast<int>(class_name.size()),
        wname.data(), wlen);

    // Register the Win32 window class (Req 1.2).
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ::DefWindowProcW;   // Applications supply their own WndProc
    wc.hInstance     = ::GetModuleHandleW(nullptr);
    wc.hCursor       = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)); // IDC_ARROW
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = wname.c_str();

    if (::RegisterClassExW(&wc) == 0) {
        // If the class is already registered that is acceptable — treat it as
        // success so that multiple Application instances in tests don't fail.
        const DWORD last_err = ::GetLastError();
        if (last_err != ERROR_CLASS_ALREADY_EXISTS) {
            return std::unexpected(error::Error{
                error::ErrorCode::WindowClassRegistrationFailed,
                "RegisterClassExW failed",
                last_err
            });
        }
    }

    Application app{};
    app.mode_             = mode;
    app.class_name_       = std::string{ class_name };
    app.wclass_name_      = std::move(wname);
    app.main_thread_id_   = tid;
    app.class_registered_ = true;

    ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
           "Application created");

    return app;
}

// ---------------------------------------------------------------------------
// Special members
// ---------------------------------------------------------------------------

Application::Application(Application&& other) noexcept
    : mode_             (other.mode_)
    , class_name_       (std::move(other.class_name_))
    , wclass_name_      (std::move(other.wclass_name_))
    , frame_cb_         (std::move(other.frame_cb_))
    , error_handler_    (std::move(other.error_handler_))
    , main_thread_id_   (other.main_thread_id_)
    , class_registered_ (std::exchange(other.class_registered_, false))
{}

Application& Application::operator=(Application&& other) noexcept
{
    if (this == &other) return *this;

    // Unregister the current class before taking ownership of the new one.
    if (class_registered_ && !wclass_name_.empty()) {
        ::UnregisterClassW(wclass_name_.c_str(), ::GetModuleHandleW(nullptr));
    }

    mode_             = other.mode_;
    class_name_       = std::move(other.class_name_);
    wclass_name_      = std::move(other.wclass_name_);
    frame_cb_         = std::move(other.frame_cb_);
    error_handler_    = std::move(other.error_handler_);
    main_thread_id_   = other.main_thread_id_;
    class_registered_ = std::exchange(other.class_registered_, false);

    return *this;
}

Application::~Application() noexcept
{
    if (class_registered_ && !wclass_name_.empty()) {
        ::UnregisterClassW(wclass_name_.c_str(), ::GetModuleHandleW(nullptr));
        ZK_LOG(zk::log::Level::Debug, zk::log::Domain::Core,
               "Application: window class unregistered");
    }
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void Application::set_frame_callback(std::function<void()> cb) noexcept
{
    assert_main_thread();
    frame_cb_ = std::move(cb);
}

void Application::set_error_handler(
    std::function<void(const error::Error&)> handler) noexcept
{
    assert_main_thread();
    error_handler_ = std::move(handler);
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

std::expected<int, error::Error> Application::run() noexcept
{
    assert_main_thread();

    ZK_LOG(zk::log::Level::Info, zk::log::Domain::Core,
           "Application::run() starting message loop");

    if (mode_ == PumpMode::NonBlocking) {
        // Delegate to the non-blocking pump (Req 1.3, 2.x).
        detail::NonBlockingPump pump{};
        return pump.run(frame_cb_);
    } else {
        // Delegate to the blocking pump (Req 1.4, 3.x).
        detail::BlockingPump pump{};
        return pump.run(error_handler_);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Application::assert_main_thread() const noexcept
{
#if !defined(NDEBUG)
    if (::GetCurrentThreadId() != main_thread_id_) {
        ::OutputDebugStringA(
            "[zketch] FATAL: Application method called from non-main thread!\n");
        __debugbreak();
    }
#endif
}

} // namespace zk::core
