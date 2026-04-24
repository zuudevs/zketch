#include "zk/render/renderer.hpp"

#include "zk/error/error.hpp"
#include "zk/log/logger.hpp"

// Link Direct2D and DirectWrite at compile time.
// These pragmas are equivalent to adding d2d1.lib / dwrite.lib in CMake,
// but serve as a belt-and-suspenders fallback for IDE builds.
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace zk::render {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Converts a UTF-8 std::string_view to a null-terminated wide string.
/// Returns an empty wstring on conversion failure.
std::wstring utf8_to_wide(std::string_view utf8) {
    if (utf8.empty()) return {};

    const int len = ::MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        nullptr, 0);

    if (len <= 0) return {};

    std::wstring wide(static_cast<std::size_t>(len), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()),
        wide.data(), len);

    return wide;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::expected<Renderer, error::Error> Renderer::create(HWND hwnd) noexcept {
    Renderer r;
    r.hwnd_ = hwnd;

    // --- Create ID2D1Factory ---
    HRESULT hr = ::D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        r.factory_.GetAddressOf());

    if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Render,
               "D2D1CreateFactory failed");
        return std::unexpected(error::make_error(
            error::ErrorCode::RenderTargetCreationFailed,
            "D2D1CreateFactory failed",
            static_cast<uint32_t>(hr)));
    }

    // --- Create IDWriteFactory ---
    hr = ::DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(r.dwrite_factory_.GetAddressOf()));

    if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Render,
               "DWriteCreateFactory failed");
        return std::unexpected(error::make_error(
            error::ErrorCode::RenderTargetCreationFailed,
            "DWriteCreateFactory failed",
            static_cast<uint32_t>(hr)));
    }

    // --- Create ID2D1HwndRenderTarget ---
    auto result = r.recreate_target(hwnd);
    if (!result) {
        return std::unexpected(result.error());
    }

    ZK_LOG(zk::log::Level::Info, zk::log::Domain::Render,
           "Renderer created successfully");

    return r;
}

// ---------------------------------------------------------------------------
// Frame lifecycle
// ---------------------------------------------------------------------------

void Renderer::begin_frame() noexcept {
    if (!has_valid_target()) return;
    do_begin_draw();
}

void Renderer::end_frame() noexcept {
    if (!has_valid_target()) return;

    const HRESULT hr = do_end_draw();

    if (hr == D2DERR_RECREATE_TARGET) {
        ZK_LOG(zk::log::Level::Warn, zk::log::Domain::Render,
               "D2DERR_RECREATE_TARGET — recreating render target");

        // Release the stale target before recreating.
        render_target_.Reset();

        auto result = do_recreate_target(hwnd_);
        if (!result) {
            ZK_LOG(zk::log::Level::Error, zk::log::Domain::Render,
                   "Failed to recreate render target after D2DERR_RECREATE_TARGET");
        }

        // Signal the caller to schedule a full repaint.
        needs_redraw_ = true;

    } else if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Render,
               "EndDraw failed with unexpected HRESULT");
    }
}

// ---------------------------------------------------------------------------
// Draw primitives
// ---------------------------------------------------------------------------

void Renderer::clear(ui::Color color) noexcept {
    if (!has_valid_target()) return;
    render_target_->Clear(to_d2d_color(color));
}

void Renderer::draw_rect(metric::Pos<int>       pos,
                         metric::Size<uint16_t> size,
                         ui::Color              color) noexcept {
    if (!has_valid_target()) return;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = render_target_->CreateSolidColorBrush(
        to_d2d_color(color), brush.GetAddressOf());

    if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Warn, zk::log::Domain::Render,
               "draw_rect: CreateSolidColorBrush failed");
        return;
    }

    const D2D1_RECT_F rect = D2D1::RectF(
        static_cast<FLOAT>(pos.x),
        static_cast<FLOAT>(pos.y),
        static_cast<FLOAT>(pos.x + size.w),
        static_cast<FLOAT>(pos.y + size.h));

    render_target_->FillRectangle(rect, brush.Get());
}

void Renderer::draw_text(std::string_view       text,
                         metric::Pos<int>        pos,
                         metric::Size<uint16_t>  clip_size,
                         const ui::FontConfig&   font,
                         ui::Color               color) noexcept {
    if (!has_valid_target() || !dwrite_factory_) return;
    if (text.empty()) return;

    // --- Build IDWriteTextFormat ---
    const std::wstring family_wide = utf8_to_wide(font.family);

    Microsoft::WRL::ComPtr<IDWriteTextFormat> text_format;
    HRESULT hr = dwrite_factory_->CreateTextFormat(
        family_wide.c_str(),
        nullptr,                                                    // font collection (nullptr = system)
        font.bold   ? DWRITE_FONT_WEIGHT_BOLD   : DWRITE_FONT_WEIGHT_NORMAL,
        font.italic ? DWRITE_FONT_STYLE_ITALIC  : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        font.size_pt * (96.0f / 72.0f),                            // pt → DIPs (96 DPI baseline)
        L"",                                                        // locale
        text_format.GetAddressOf());

    if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Warn, zk::log::Domain::Render,
               "draw_text: CreateTextFormat failed");
        return;
    }

    // --- Create solid colour brush ---
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    hr = render_target_->CreateSolidColorBrush(
        to_d2d_color(color), brush.GetAddressOf());

    if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Warn, zk::log::Domain::Render,
               "draw_text: CreateSolidColorBrush failed");
        return;
    }

    // --- Clip rectangle — text is clipped to widget bounds (Req 7.4) ---
    const D2D1_RECT_F layout_rect = D2D1::RectF(
        static_cast<FLOAT>(pos.x),
        static_cast<FLOAT>(pos.y),
        static_cast<FLOAT>(pos.x + clip_size.w),
        static_cast<FLOAT>(pos.y + clip_size.h));

    // Convert UTF-8 text to wide for DirectWrite.
    const std::wstring wide_text = utf8_to_wide(text);
    if (wide_text.empty()) return;

    // Push an axis-aligned clip layer so text never bleeds outside the widget.
    render_target_->PushAxisAlignedClip(layout_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    // ID2D1RenderTarget::DrawText is shadowed by the Win32 DrawText macro.
    // Cast to the base interface and call through a pointer-to-member to
    // bypass the macro expansion entirely.
    ID2D1RenderTarget* rt = render_target_.Get();
    rt->DrawText(
        wide_text.c_str(),
        static_cast<UINT32>(wide_text.size()),
        text_format.Get(),
        layout_rect,
        brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP);

    render_target_->PopAxisAlignedClip();
}

// ---------------------------------------------------------------------------
// Testable seam — default implementations
// ---------------------------------------------------------------------------

bool Renderer::has_valid_target() const noexcept {
    return render_target_ != nullptr;
}

void Renderer::do_begin_draw() noexcept {
    render_target_->BeginDraw();
}

HRESULT Renderer::do_end_draw() noexcept {
    return render_target_->EndDraw();
}

std::expected<void, error::Error> Renderer::do_recreate_target(HWND hwnd) noexcept {
    return recreate_target(hwnd);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::expected<void, error::Error> Renderer::recreate_target(HWND hwnd) noexcept {
    if (!factory_) {
        return std::unexpected(error::make_error(
            error::ErrorCode::RenderTargetCreationFailed,
            "recreate_target called before factory was initialised"));
    }

    // Query the client area size for the render target.
    RECT client_rect{};
    ::GetClientRect(hwnd, &client_rect);

    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT32>(client_rect.right  - client_rect.left),
        static_cast<UINT32>(client_rect.bottom - client_rect.top));

    const D2D1_RENDER_TARGET_PROPERTIES props =
        D2D1::RenderTargetProperties();

    const D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props =
        D2D1::HwndRenderTargetProperties(hwnd, size);

    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> new_target;
    const HRESULT hr = factory_->CreateHwndRenderTarget(
        props, hwnd_props, new_target.GetAddressOf());

    if (FAILED(hr)) {
        ZK_LOG(zk::log::Level::Error, zk::log::Domain::Render,
               "CreateHwndRenderTarget failed");
        return std::unexpected(error::make_error(
            error::ErrorCode::RenderTargetCreationFailed,
            "CreateHwndRenderTarget failed",
            static_cast<uint32_t>(hr)));
    }

    render_target_ = std::move(new_target);
    return {};
}

D2D1_COLOR_F Renderer::to_d2d_color(ui::Color c) noexcept {
    constexpr float inv255 = 1.0f / 255.0f;
    return D2D1::ColorF(
        c.r * inv255,
        c.g * inv255,
        c.b * inv255,
        c.a * inv255);
}

} // namespace zk::render
