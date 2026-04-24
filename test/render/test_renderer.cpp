// test/render/test_renderer.cpp
//
// Unit tests for Renderer logic that can be verified without a real GPU.
//
// Strategy: A thin test subclass (RendererStub) overrides the three virtual
// seam methods introduced in Renderer:
//   - has_valid_target()   — returns true so frame methods don't early-exit
//   - do_end_draw()        — returns a configurable HRESULT (no COM call)
//   - do_recreate_target() — counts calls and returns a configurable result
//
// This lets us test the state-machine logic inside end_frame() and the
// render-order guards without initialising Direct2D or creating a window.
//
// NOTE: Full integration tests that exercise the real Direct2D path
// (ID2D1HwndRenderTarget creation, actual pixel output, etc.) require a
// real HWND and GPU and must be run as manual / CI integration tests.
// TODO: Add integration tests in test/render/test_renderer_integration.cpp
//       once a headless Win32 window fixture is available.

#include <catch2/catch_test_macros.hpp>

// Windows headers must come before Direct2D
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d2d1.h>

#include "zk/render/renderer.hpp"
#include "zk/error/error.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"

// ---------------------------------------------------------------------------
// RendererStub — test double that bypasses GPU
// ---------------------------------------------------------------------------

class RendererStub : public zk::render::Renderer {
public:
    // Configurable behaviour
    HRESULT end_draw_result = S_OK;
    bool    recreate_should_fail = false;

    // Observation counters
    int begin_frame_count   = 0;
    int end_frame_count     = 0;
    int recreate_call_count = 0;

    // Expose public frame methods for counting
    void begin_frame() noexcept {
        ++begin_frame_count;
        Renderer::begin_frame();
    }

    void end_frame() noexcept {
        ++end_frame_count;
        Renderer::end_frame();
    }

protected:
    // Seam 1: pretend we always have a valid render target
    bool has_valid_target() const noexcept override {
        return true;
    }

    // Seam 2: no-op BeginDraw (render_target_ COM ptr is null)
    void do_begin_draw() noexcept override {
        // intentional no-op — no real GPU target in tests
    }

    // Seam 3: return the configured HRESULT instead of calling COM
    HRESULT do_end_draw() noexcept override {
        return end_draw_result;
    }

    // Seam 4: count calls and return success/failure without GPU
    std::expected<void, zk::error::Error> do_recreate_target(HWND /*hwnd*/) noexcept override {
        ++recreate_call_count;
        if (recreate_should_fail) {
            return std::unexpected(zk::error::make_error(
                zk::error::ErrorCode::RenderTargetCreationFailed,
                "stub: recreate_target forced failure"));
        }
        return {};
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static RendererStub make_stub() {
    return RendererStub{};
}

// ---------------------------------------------------------------------------
// Req 10.4 — D2DERR_RECREATE_TARGET handling
// ---------------------------------------------------------------------------

TEST_CASE("Renderer: end_frame triggers recreate_target when EndDraw returns D2DERR_RECREATE_TARGET",
          "[render][renderer][req-10.4]")
{
    // Req 10.4 — IF EndDraw returns D2DERR_RECREATE_TARGET, THEN the Renderer
    // SHALL discard the old target, create a new one, and schedule a repaint.
    RendererStub r = make_stub();
    r.end_draw_result = D2DERR_RECREATE_TARGET;

    REQUIRE(r.recreate_call_count == 0);
    REQUIRE_FALSE(r.needs_redraw());

    r.end_frame();

    REQUIRE(r.recreate_call_count == 1);
    REQUIRE(r.needs_redraw());
}

TEST_CASE("Renderer: end_frame does NOT call recreate_target when EndDraw succeeds",
          "[render][renderer][req-10.4]")
{
    RendererStub r = make_stub();
    r.end_draw_result = S_OK;

    r.end_frame();

    REQUIRE(r.recreate_call_count == 0);
    REQUIRE_FALSE(r.needs_redraw());
}

TEST_CASE("Renderer: end_frame does NOT call recreate_target on other HRESULT failures",
          "[render][renderer][req-10.4]")
{
    // A generic failure (e.g. E_FAIL) should NOT trigger target recreation.
    RendererStub r = make_stub();
    r.end_draw_result = E_FAIL;

    r.end_frame();

    REQUIRE(r.recreate_call_count == 0);
    REQUIRE_FALSE(r.needs_redraw());
}

TEST_CASE("Renderer: needs_redraw is set after D2DERR_RECREATE_TARGET and cleared by clear_redraw_flag",
          "[render][renderer][req-10.4]")
{
    RendererStub r = make_stub();
    r.end_draw_result = D2DERR_RECREATE_TARGET;

    r.end_frame();
    REQUIRE(r.needs_redraw());

    r.clear_redraw_flag();
    REQUIRE_FALSE(r.needs_redraw());
}

TEST_CASE("Renderer: recreate_target is called again on each D2DERR_RECREATE_TARGET",
          "[render][renderer][req-10.4]")
{
    // Each end_frame() that returns D2DERR_RECREATE_TARGET must trigger
    // exactly one recreate attempt.
    RendererStub r = make_stub();
    r.end_draw_result = D2DERR_RECREATE_TARGET;

    r.end_frame();
    REQUIRE(r.recreate_call_count == 1);

    r.clear_redraw_flag();
    r.end_frame();
    REQUIRE(r.recreate_call_count == 2);
}

TEST_CASE("Renderer: needs_redraw is set even when recreate_target fails",
          "[render][renderer][req-10.4]")
{
    // Even if recreation fails, needs_redraw_ must be set so the caller
    // knows a repaint is required (it will fail again, but the flag is set).
    RendererStub r = make_stub();
    r.end_draw_result      = D2DERR_RECREATE_TARGET;
    r.recreate_should_fail = true;

    r.end_frame();

    REQUIRE(r.recreate_call_count == 1);
    REQUIRE(r.needs_redraw());
}

// ---------------------------------------------------------------------------
// Render order — begin_frame before draw_* before end_frame
// ---------------------------------------------------------------------------

TEST_CASE("Renderer: begin_frame and end_frame can be called in sequence without error",
          "[render][renderer][order]")
{
    // Verifies the basic happy-path frame lifecycle does not crash or assert.
    RendererStub r = make_stub();
    r.end_draw_result = S_OK;

    REQUIRE_NOTHROW(r.begin_frame());
    REQUIRE_NOTHROW(r.end_frame());
}

TEST_CASE("Renderer: begin_frame increments call count before end_frame",
          "[render][renderer][order]")
{
    RendererStub r = make_stub();

    REQUIRE(r.begin_frame_count == 0);
    REQUIRE(r.end_frame_count   == 0);

    r.begin_frame();
    REQUIRE(r.begin_frame_count == 1);
    REQUIRE(r.end_frame_count   == 0);  // end_frame not yet called

    r.end_frame();
    REQUIRE(r.begin_frame_count == 1);
    REQUIRE(r.end_frame_count   == 1);
}

TEST_CASE("Renderer: multiple frames execute in correct begin/end order",
          "[render][renderer][order]")
{
    RendererStub r = make_stub();

    for (int i = 1; i <= 3; ++i) {
        r.begin_frame();
        REQUIRE(r.begin_frame_count == i);
        REQUIRE(r.end_frame_count   == i - 1);  // end_frame lags by one

        r.end_frame();
        REQUIRE(r.begin_frame_count == i);
        REQUIRE(r.end_frame_count   == i);
    }
}

TEST_CASE("Renderer: draw_rect is a no-op when called without a valid target",
          "[render][renderer][order]")
{
    // When has_valid_target() returns false (default Renderer, no GPU),
    // draw_rect must not crash — it silently returns.
    //
    // We use the base Renderer directly (default-constructed, no target).
    // The default has_valid_target() returns false because render_target_ is null.
    //
    // NOTE: We cannot call Renderer() directly (protected), so we use a
    // minimal stub that does NOT override has_valid_target().
    class NullTargetStub : public zk::render::Renderer {
    public:
        // has_valid_target() NOT overridden — returns false (null COM ptr)
        // do_end_draw() NOT overridden — would crash if called, but won't be
    };

    NullTargetStub r;
    REQUIRE_NOTHROW(r.draw_rect(
        zk::metric::Pos<int>{0, 0},
        zk::metric::Size<uint16_t>{100u, 100u},
        zk::ui::Color{255, 0, 0, 255}));
}

TEST_CASE("Renderer: clear is a no-op when called without a valid target",
          "[render][renderer][order]")
{
    class NullTargetStub : public zk::render::Renderer {};

    NullTargetStub r;
    REQUIRE_NOTHROW(r.clear(zk::ui::Color{0, 0, 0, 255}));
}

TEST_CASE("Renderer: draw_text is a no-op when called without a valid target",
          "[render][renderer][order]")
{
    class NullTargetStub : public zk::render::Renderer {};

    NullTargetStub r;
    REQUIRE_NOTHROW(r.draw_text(
        "hello",
        zk::metric::Pos<int>{0, 0},
        zk::metric::Size<uint16_t>{100u, 30u},
        zk::ui::FontConfig{},
        zk::ui::Color{0, 0, 0, 255}));
}

TEST_CASE("Renderer: begin_frame is a no-op when called without a valid target",
          "[render][renderer][order]")
{
    class NullTargetStub : public zk::render::Renderer {};

    NullTargetStub r;
    REQUIRE_NOTHROW(r.begin_frame());
}

TEST_CASE("Renderer: end_frame is a no-op when called without a valid target",
          "[render][renderer][order]")
{
    class NullTargetStub : public zk::render::Renderer {};

    NullTargetStub r;
    REQUIRE_NOTHROW(r.end_frame());
    REQUIRE_FALSE(r.needs_redraw());  // no target → no recreate → no redraw flag
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

TEST_CASE("Renderer: is_valid returns false for default-constructed stub",
          "[render][renderer][state]")
{
    class NullTargetStub : public zk::render::Renderer {};
    NullTargetStub r;
    REQUIRE_FALSE(r.is_valid());
}

TEST_CASE("Renderer: needs_redraw is false initially",
          "[render][renderer][state]")
{
    RendererStub r = make_stub();
    REQUIRE_FALSE(r.needs_redraw());
}

TEST_CASE("Renderer: clear_redraw_flag is idempotent",
          "[render][renderer][state]")
{
    RendererStub r = make_stub();
    r.clear_redraw_flag();
    r.clear_redraw_flag();
    REQUIRE_FALSE(r.needs_redraw());
}
