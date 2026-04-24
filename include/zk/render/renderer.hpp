#pragma once

// Windows headers must come before Direct2D/DirectWrite
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Direct2D and DirectWrite
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <expected>
#include <string_view>

#include "zk/error/error.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"

namespace zk::render {

/// Direct2D-backed renderer for a single HWND render target.
///
/// Lifecycle:
///   1. Call `Renderer::create(hwnd)` once after the window is created.
///   2. Each frame: `begin_frame()` → draw calls → `end_frame()`.
///   3. `end_frame()` handles `D2DERR_RECREATE_TARGET` transparently by
///      recreating the render target and setting a pending-redraw flag.
///
/// All methods must be called from the Main Thread (Req 11.1).
///
/// Satisfies Requirements: 10.1–10.7, 7.4
class Renderer {
public:
    // -----------------------------------------------------------------------
    // Factory
    // -----------------------------------------------------------------------

    /// Creates a Renderer for the given HWND.
    /// Initialises ID2D1Factory, ID2D1HwndRenderTarget, and IDWriteFactory.
    /// Returns an error if any COM object fails to initialise.
    [[nodiscard]] static std::expected<Renderer, error::Error> create(HWND hwnd) noexcept;

    // -----------------------------------------------------------------------
    // Special members — move-only (COM objects are not copyable)
    // -----------------------------------------------------------------------
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept            = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    virtual ~Renderer() noexcept = default;

    // -----------------------------------------------------------------------
    // Frame lifecycle
    // -----------------------------------------------------------------------

    /// Begins a new render frame by calling BeginDraw on the render target.
    /// Must be called before any draw_* methods.
    void begin_frame() noexcept;

    /// Ends the current render frame by calling EndDraw.
    /// If EndDraw returns D2DERR_RECREATE_TARGET the render target is
    /// recreated automatically and `needs_redraw()` is set to true so the
    /// caller can schedule an immediate repaint.
    void end_frame() noexcept;

    // -----------------------------------------------------------------------
    // Draw primitives
    // -----------------------------------------------------------------------

    /// Fills the entire render target with `color`.
    void clear(ui::Color color) noexcept;

    /// Draws a filled rectangle at `pos` with dimensions `size` in `color`.
    void draw_rect(metric::Pos<int>        pos,
                   metric::Size<uint16_t>  size,
                   ui::Color               color) noexcept;

    /// Draws `text` inside the clip rectangle defined by `pos`/`clip_size`.
    /// Text is clipped to the widget bounds (Req 7.4).
    void draw_text(std::string_view        text,
                   metric::Pos<int>        pos,
                   metric::Size<uint16_t>  clip_size,
                   const ui::FontConfig&   font,
                   ui::Color               color) noexcept;

    // -----------------------------------------------------------------------
    // State queries
    // -----------------------------------------------------------------------

    /// Returns true if the render target was recreated during the last
    /// end_frame() call and the caller should schedule a full repaint.
    [[nodiscard]] bool needs_redraw() const noexcept { return needs_redraw_; }

    /// Clears the needs_redraw flag — call after scheduling the repaint.
    void clear_redraw_flag() noexcept { needs_redraw_ = false; }

    /// Returns true if the renderer has a valid render target.
    [[nodiscard]] bool is_valid() const noexcept {
        return render_target_ != nullptr;
    }

protected:
    // Protected constructor — use create() for normal usage.
    // Exposed as protected to allow test subclasses to inject mock behaviour.
    Renderer() noexcept = default;

    // -----------------------------------------------------------------------
    // Testable seams — overridable in unit tests (non-hot-path)
    // -----------------------------------------------------------------------

    /// Returns true when the render target is ready to accept draw calls.
    /// Overridable so tests can bypass the COM pointer null-check without GPU.
    /// Default implementation checks render_target_ != nullptr.
    [[nodiscard]] virtual bool has_valid_target() const noexcept;

    /// Calls BeginDraw on the render target.
    /// Overridable so tests can stub out the GPU-dependent call.
    /// Default implementation delegates to render_target_->BeginDraw().
    virtual void do_begin_draw() noexcept;

    /// Calls EndDraw on the render target and returns the HRESULT.
    /// Overridable so tests can simulate D2DERR_RECREATE_TARGET without GPU.
    /// Default implementation delegates to render_target_->EndDraw().
    virtual HRESULT do_end_draw() noexcept;

    /// Recreates the HwndRenderTarget after D2DERR_RECREATE_TARGET.
    /// Overridable so tests can stub out the GPU-dependent creation step.
    virtual std::expected<void, error::Error> do_recreate_target(HWND hwnd) noexcept;

private:
    /// Recreates the HwndRenderTarget after D2DERR_RECREATE_TARGET.
    [[nodiscard]] std::expected<void, error::Error> recreate_target(HWND hwnd) noexcept;

    /// Converts a zk::ui::Color to a D2D1_COLOR_F.
    [[nodiscard]] static D2D1_COLOR_F to_d2d_color(ui::Color c) noexcept;

    // -----------------------------------------------------------------------
    // COM objects
    // -----------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ID2D1Factory>          factory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> render_target_;
    Microsoft::WRL::ComPtr<IDWriteFactory>        dwrite_factory_;

    /// The HWND this renderer is bound to — needed for target recreation.
    HWND hwnd_{ nullptr };

    /// Set to true when the render target was recreated; cleared by caller.
    bool needs_redraw_{ false };
};

} // namespace zk::render
