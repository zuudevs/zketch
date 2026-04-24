#include <catch2/catch_test_macros.hpp>

#include "zk/ui/button.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

using namespace zk::ui;
using zk::metric::Pos;
using zk::metric::Size;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Returns a Button at a fixed position/size — avoids repeating boilerplate.
static Button make_button(std::string_view label = "OK")
{
    return Button{ label, Pos<int>{ 0, 0 }, Size<uint16_t>{ 80u, 30u } };
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Button: initial state is Normal",
          "[ui][button]")
{
    Button btn = make_button();
    REQUIRE(btn.state() == ButtonState::Normal);
}

TEST_CASE("Button: label is stored correctly",
          "[ui][button]")
{
    Button btn = make_button("Click me");
    REQUIRE(btn.label() == "Click me");
}

// ---------------------------------------------------------------------------
// State transitions — Req 8.3, 8.4
// ---------------------------------------------------------------------------

TEST_CASE("Button: on_mouse_enter transitions Normal -> Hovered",
          "[ui][button][state]")
{
    // Req 8.3 — entering the button area sets state to Hovered.
    Button btn = make_button();
    REQUIRE(btn.state() == ButtonState::Normal);

    btn.on_mouse_enter();
    REQUIRE(btn.state() == ButtonState::Hovered);
}

TEST_CASE("Button: on_mouse_leave transitions Hovered -> Normal",
          "[ui][button][state]")
{
    // Req 8.4 — leaving the button area returns state to Normal.
    Button btn = make_button();
    btn.on_mouse_enter();
    REQUIRE(btn.state() == ButtonState::Hovered);

    btn.on_mouse_leave();
    REQUIRE(btn.state() == ButtonState::Normal);
}

TEST_CASE("Button: on_mouse_enter does not change Disabled state",
          "[ui][button][state]")
{
    // Req 8.7 — Disabled button ignores hover transitions.
    Button btn = make_button();
    btn.set_disabled(true);
    REQUIRE(btn.state() == ButtonState::Disabled);

    btn.on_mouse_enter();
    REQUIRE(btn.state() == ButtonState::Disabled);
}

TEST_CASE("Button: on_mouse_leave does not change Disabled state",
          "[ui][button][state]")
{
    // Req 8.7 — Disabled button ignores leave transitions.
    Button btn = make_button();
    btn.set_disabled(true);

    btn.on_mouse_leave();
    REQUIRE(btn.state() == ButtonState::Disabled);
}

TEST_CASE("Button: on_mouse_enter is idempotent when already Hovered",
          "[ui][button][state]")
{
    Button btn = make_button();
    btn.on_mouse_enter();
    btn.on_mouse_enter();  // second call — should stay Hovered
    REQUIRE(btn.state() == ButtonState::Hovered);
}

TEST_CASE("Button: on_mouse_leave from Normal does not change state",
          "[ui][button][state]")
{
    // Leaving without having entered should be a no-op.
    Button btn = make_button();
    REQUIRE(btn.state() == ButtonState::Normal);

    btn.on_mouse_leave();
    REQUIRE(btn.state() == ButtonState::Normal);
}

// ---------------------------------------------------------------------------
// Click handler — Req 8.2, 8.6
// ---------------------------------------------------------------------------

TEST_CASE("Button: on_click invokes registered handler",
          "[ui][button][click]")
{
    // Req 8.2 — handler is called when button is clicked in Normal state.
    Button btn = make_button();

    int call_count = 0;
    btn.set_on_click([&call_count]() { ++call_count; });

    btn.on_click();
    REQUIRE(call_count == 1);
}

TEST_CASE("Button: on_click invokes handler when Hovered",
          "[ui][button][click]")
{
    // Req 8.2 — handler fires regardless of Normal vs Hovered.
    Button btn = make_button();
    btn.on_mouse_enter();
    REQUIRE(btn.state() == ButtonState::Hovered);

    int call_count = 0;
    btn.set_on_click([&call_count]() { ++call_count; });

    btn.on_click();
    REQUIRE(call_count == 1);
}

TEST_CASE("Button: on_click does not invoke handler when Disabled",
          "[ui][button][click]")
{
    // Req 8.7 — click on a Disabled button must not fire the handler.
    Button btn = make_button();
    btn.set_disabled(true);

    int call_count = 0;
    btn.set_on_click([&call_count]() { ++call_count; });

    btn.on_click();
    REQUIRE(call_count == 0);
}

TEST_CASE("Button: on_click without registered handler does not error",
          "[ui][button][click]")
{
    // Req 8.6 — missing handler is silently ignored; no crash or exception.
    Button btn = make_button();
    REQUIRE_NOTHROW(btn.on_click());
}

TEST_CASE("Button: on_click without handler in Hovered state does not error",
          "[ui][button][click]")
{
    // Req 8.6 — same guarantee applies when Hovered.
    Button btn = make_button();
    btn.on_mouse_enter();
    REQUIRE_NOTHROW(btn.on_click());
}

// ---------------------------------------------------------------------------
// set_disabled — Req 8.7
// ---------------------------------------------------------------------------

TEST_CASE("Button: set_disabled(true) sets state to Disabled",
          "[ui][button][disabled]")
{
    Button btn = make_button();
    btn.set_disabled(true);
    REQUIRE(btn.state() == ButtonState::Disabled);
}

TEST_CASE("Button: set_disabled(false) returns state to Normal",
          "[ui][button][disabled]")
{
    Button btn = make_button();
    btn.set_disabled(true);
    btn.set_disabled(false);
    REQUIRE(btn.state() == ButtonState::Normal);
}

TEST_CASE("Button: set_disabled(true) from Hovered sets state to Disabled",
          "[ui][button][disabled]")
{
    Button btn = make_button();
    btn.on_mouse_enter();
    REQUIRE(btn.state() == ButtonState::Hovered);

    btn.set_disabled(true);
    REQUIRE(btn.state() == ButtonState::Disabled);
}

TEST_CASE("Button: re-enabling after disable allows hover transitions again",
          "[ui][button][disabled]")
{
    Button btn = make_button();
    btn.set_disabled(true);
    btn.set_disabled(false);

    btn.on_mouse_enter();
    REQUIRE(btn.state() == ButtonState::Hovered);

    btn.on_mouse_leave();
    REQUIRE(btn.state() == ButtonState::Normal);
}

// ---------------------------------------------------------------------------
// dirty flag — state changes must mark widget dirty
// ---------------------------------------------------------------------------

TEST_CASE("Button: on_mouse_enter marks widget dirty",
          "[ui][button][dirty]")
{
    Button btn = make_button();
    btn.clear_dirty();

    btn.on_mouse_enter();
    REQUIRE(btn.is_dirty());
}

TEST_CASE("Button: on_mouse_leave marks widget dirty",
          "[ui][button][dirty]")
{
    Button btn = make_button();
    btn.on_mouse_enter();
    btn.clear_dirty();

    btn.on_mouse_leave();
    REQUIRE(btn.is_dirty());
}

TEST_CASE("Button: set_disabled marks widget dirty",
          "[ui][button][dirty]")
{
    Button btn = make_button();
    btn.clear_dirty();

    btn.set_disabled(true);
    REQUIRE(btn.is_dirty());
}
