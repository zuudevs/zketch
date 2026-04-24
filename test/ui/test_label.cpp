#include <catch2/catch_test_macros.hpp>

#include "zk/ui/label.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

using namespace zk::ui;
using zk::metric::Pos;
using zk::metric::Size;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Returns a Label at a fixed position/size — avoids repeating boilerplate.
static Label make_label(std::string_view text = "hello")
{
    return Label{ text, Pos<int>{ 0, 0 }, Size<uint16_t>{ 100u, 20u } };
}

// ---------------------------------------------------------------------------
// Construction — Req 7.1
// ---------------------------------------------------------------------------

TEST_CASE("Label: construction with non-empty text stores text correctly",
          "[ui][label]")
{
    Label lbl = make_label("Hello, zketch!");
    REQUIRE(lbl.text() == "Hello, zketch!");
}

TEST_CASE("Label: construction with empty text is valid",
          "[ui][label]")
{
    // Req 7.1 — Label stores a single configurable UTF-8 string; empty is valid.
    REQUIRE_NOTHROW(make_label(""));

    Label lbl = make_label("");
    REQUIRE(lbl.text().empty());
}

TEST_CASE("Label: newly constructed label is dirty",
          "[ui][label]")
{
    // A freshly created widget must be dirty so the first render pass picks it up.
    Label lbl = make_label("init");
    REQUIRE(lbl.is_dirty());
}

// ---------------------------------------------------------------------------
// set_text marks widget dirty — Req 7.2
// ---------------------------------------------------------------------------

TEST_CASE("Label: set_text marks widget dirty",
          "[ui][label]")
{
    Label lbl = make_label("initial");

    // Clear the dirty flag to simulate a completed render pass.
    lbl.clear_dirty();
    REQUIRE_FALSE(lbl.is_dirty());

    // Changing the text must re-set the dirty flag (Req 7.2).
    lbl.set_text("updated");
    REQUIRE(lbl.is_dirty());
}

TEST_CASE("Label: set_text updates stored text",
          "[ui][label]")
{
    Label lbl = make_label("before");
    lbl.set_text("after");
    REQUIRE(lbl.text() == "after");
}

TEST_CASE("Label: set_text with empty string marks widget dirty",
          "[ui][label]")
{
    Label lbl = make_label("non-empty");
    lbl.clear_dirty();

    lbl.set_text("");
    REQUIRE(lbl.is_dirty());
    REQUIRE(lbl.text().empty());
}

TEST_CASE("Label: set_text with same text still marks widget dirty",
          "[ui][label]")
{
    // No short-circuit optimisation is required by the spec; dirty must be set
    // unconditionally so callers do not need to track previous values.
    Label lbl = make_label("same");
    lbl.clear_dirty();

    lbl.set_text("same");
    REQUIRE(lbl.is_dirty());
}

// ---------------------------------------------------------------------------
// set_font marks widget dirty
// ---------------------------------------------------------------------------

TEST_CASE("Label: set_font marks widget dirty",
          "[ui][label]")
{
    Label lbl = make_label();
    lbl.clear_dirty();

    FontConfig fc;
    fc.family  = "Arial";
    fc.size_pt = 14.0f;
    fc.bold    = true;

    lbl.set_font(fc);
    REQUIRE(lbl.is_dirty());
}

TEST_CASE("Label: set_font stores font configuration",
          "[ui][label]")
{
    Label lbl = make_label();

    FontConfig fc;
    fc.family  = "Consolas";
    fc.size_pt = 10.0f;
    fc.italic  = true;

    lbl.set_font(fc);

    REQUIRE(lbl.font().family  == "Consolas");
    REQUIRE(lbl.font().size_pt == 10.0f);
    REQUIRE(lbl.font().italic  == true);
}

// ---------------------------------------------------------------------------
// set_color marks widget dirty — Req 7.5
// ---------------------------------------------------------------------------

TEST_CASE("Label: set_color marks widget dirty",
          "[ui][label]")
{
    Label lbl = make_label();
    lbl.clear_dirty();

    lbl.set_color(Color{ 255, 0, 0, 255 });
    REQUIRE(lbl.is_dirty());
}

TEST_CASE("Label: set_color stores RGBA color",
          "[ui][label]")
{
    Label lbl = make_label();
    lbl.set_color(Color{ 10, 20, 30, 200 });

    REQUIRE(lbl.color().r == 10);
    REQUIRE(lbl.color().g == 20);
    REQUIRE(lbl.color().b == 30);
    REQUIRE(lbl.color().a == 200);
}

// ---------------------------------------------------------------------------
// Default color
// ---------------------------------------------------------------------------

TEST_CASE("Label: default text color is opaque black (0, 0, 0, 255)",
          "[ui][label]")
{
    Label lbl = make_label();
    REQUIRE(lbl.color().r == 0);
    REQUIRE(lbl.color().g == 0);
    REQUIRE(lbl.color().b == 0);
    REQUIRE(lbl.color().a == 255);
}
