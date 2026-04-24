#include <catch2/catch_test_macros.hpp>

#include "zk/ui/panel.hpp"
#include "zk/ui/label.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

#include <memory>

using namespace zk::ui;
using zk::metric::Pos;
using zk::metric::Size;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Creates a Panel at the given position with a fixed 200×200 size.
static Panel make_panel(int x = 0, int y = 0,
                        uint16_t w = 200u, uint16_t h = 200u)
{
    return Panel{ Pos<int>{ x, y }, Size<uint16_t>{ w, h } };
}

/// Creates a heap-allocated Label with the given relative position.
/// Used to add children to a Panel (ownership transfer).
static std::unique_ptr<Label> make_label_child(int rel_x, int rel_y,
                                               std::string_view text = "child")
{
    return std::make_unique<Label>(
        text,
        Pos<int>{ rel_x, rel_y },
        Size<uint16_t>{ 50u, 20u }
    );
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("Panel: initial abs_pos equals construction pos",
          "[ui][panel]")
{
    Panel p = make_panel(10, 20);
    REQUIRE(p.abs_pos().x == 10);
    REQUIRE(p.abs_pos().y == 20);
}

TEST_CASE("Panel: newly constructed panel is dirty",
          "[ui][panel]")
{
    Panel p = make_panel();
    REQUIRE(p.is_dirty());
}

TEST_CASE("Panel: newly constructed panel has no children",
          "[ui][panel]")
{
    Panel p = make_panel();
    REQUIRE(p.children().empty());
}

// ---------------------------------------------------------------------------
// Req 6.3 — absolute position of child is parent_abs + child_rel
// ---------------------------------------------------------------------------

TEST_CASE("Panel: child absolute position is parent_abs + child_rel",
          "[ui][panel][layout][req6.3]")
{
    // Panel at absolute (50, 30).
    Panel p = make_panel(50, 30);

    // Child with relative position (10, 20).
    p.add_child(make_label_child(10, 20));

    // Expected absolute: (50+10, 30+20) = (60, 50).
    const auto& child = p.children().front();
    REQUIRE(child->pos().x == 60);
    REQUIRE(child->pos().y == 50);
}

TEST_CASE("Panel: multiple children each get correct absolute positions",
          "[ui][panel][layout][req6.3]")
{
    Panel p = make_panel(100, 100);

    p.add_child(make_label_child(0,  0,  "a"));
    p.add_child(make_label_child(10, 5,  "b"));
    p.add_child(make_label_child(50, 75, "c"));

    REQUIRE(p.children()[0]->pos().x == 100);
    REQUIRE(p.children()[0]->pos().y == 100);

    REQUIRE(p.children()[1]->pos().x == 110);
    REQUIRE(p.children()[1]->pos().y == 105);

    REQUIRE(p.children()[2]->pos().x == 150);
    REQUIRE(p.children()[2]->pos().y == 175);
}

// ---------------------------------------------------------------------------
// Req 6.4 — moving the Panel updates all children's absolute positions
// ---------------------------------------------------------------------------

TEST_CASE("Panel: set_pos updates child absolute positions",
          "[ui][panel][layout][req6.4]")
{
    Panel p = make_panel(0, 0);
    p.add_child(make_label_child(10, 20));

    // Move the panel to (50, 60).
    p.set_pos(Pos<int>{ 50, 60 });

    // Child absolute should now be (50+10, 60+20) = (60, 80).
    REQUIRE(p.children().front()->pos().x == 60);
    REQUIRE(p.children().front()->pos().y == 80);
}

TEST_CASE("Panel: set_pos updates all children, not just the first",
          "[ui][panel][layout][req6.4]")
{
    Panel p = make_panel(0, 0);
    p.add_child(make_label_child(5,  5,  "first"));
    p.add_child(make_label_child(15, 25, "second"));

    p.set_pos(Pos<int>{ 100, 200 });

    REQUIRE(p.children()[0]->pos().x == 105);
    REQUIRE(p.children()[0]->pos().y == 205);

    REQUIRE(p.children()[1]->pos().x == 115);
    REQUIRE(p.children()[1]->pos().y == 225);
}

TEST_CASE("Panel: set_pos multiple times always reflects latest position",
          "[ui][panel][layout][req6.4]")
{
    Panel p = make_panel(0, 0);
    p.add_child(make_label_child(10, 10));

    p.set_pos(Pos<int>{ 50, 50 });
    REQUIRE(p.children().front()->pos().x == 60);
    REQUIRE(p.children().front()->pos().y == 60);

    p.set_pos(Pos<int>{ 200, 300 });
    REQUIRE(p.children().front()->pos().x == 210);
    REQUIRE(p.children().front()->pos().y == 310);
}

// ---------------------------------------------------------------------------
// Req 6.4 — nested Panel: recursive propagation
// ---------------------------------------------------------------------------

TEST_CASE("Panel: nested panel child positions update recursively on set_pos",
          "[ui][panel][layout][req6.4][nested]")
{
    // Outer panel at (10, 10).
    Panel outer = make_panel(10, 10);

    // Inner panel with relative pos (20, 20) → abs (30, 30).
    auto inner_ptr = std::make_unique<Panel>(
        Pos<int>{ 20, 20 }, Size<uint16_t>{ 100u, 100u }
    );
    Panel* inner = inner_ptr.get();

    // Grandchild with relative pos (5, 5) inside inner → abs (35, 35).
    inner->add_child(make_label_child(5, 5, "grandchild"));

    outer.add_child(std::move(inner_ptr));

    // Verify initial grandchild absolute position.
    REQUIRE(inner->children().front()->pos().x == 35);
    REQUIRE(inner->children().front()->pos().y == 35);

    // Move outer to (100, 100).
    outer.set_pos(Pos<int>{ 100, 100 });

    // Inner abs should now be (120, 120); grandchild abs = (125, 125).
    REQUIRE(inner->abs_pos().x == 120);
    REQUIRE(inner->abs_pos().y == 120);
    REQUIRE(inner->children().front()->pos().x == 125);
    REQUIRE(inner->children().front()->pos().y == 125);
}

// ---------------------------------------------------------------------------
// Req 6.5 — absolute position is never negative (clamped to 0)
// ---------------------------------------------------------------------------

TEST_CASE("Panel: child absolute x is clamped to 0 when result would be negative",
          "[ui][panel][layout][req6.5]")
{
    // Panel at (5, 5), child at relative (-20, 0) → raw abs x = -15 → clamped to 0.
    Panel p = make_panel(5, 5);
    p.add_child(make_label_child(-20, 0));

    REQUIRE(p.children().front()->pos().x == 0);
    REQUIRE(p.children().front()->pos().y == 5);
}

TEST_CASE("Panel: child absolute y is clamped to 0 when result would be negative",
          "[ui][panel][layout][req6.5]")
{
    Panel p = make_panel(3, 3);
    p.add_child(make_label_child(0, -10));

    REQUIRE(p.children().front()->pos().x == 3);
    REQUIRE(p.children().front()->pos().y == 0);
}

TEST_CASE("Panel: both x and y clamped to 0 when both would be negative",
          "[ui][panel][layout][req6.5]")
{
    Panel p = make_panel(1, 1);
    p.add_child(make_label_child(-100, -100));

    REQUIRE(p.children().front()->pos().x == 0);
    REQUIRE(p.children().front()->pos().y == 0);
}

TEST_CASE("Panel: after set_pos, child absolute position is still clamped to 0",
          "[ui][panel][layout][req6.5]")
{
    // Panel at (50, 50), child at relative (-10, -10) → abs (40, 40) — fine.
    Panel p = make_panel(50, 50);
    p.add_child(make_label_child(-10, -10));

    REQUIRE(p.children().front()->pos().x == 40);
    REQUIRE(p.children().front()->pos().y == 40);

    // Move panel to (5, 5): abs = (5-10, 5-10) = (-5, -5) → clamped to (0, 0).
    p.set_pos(Pos<int>{ 5, 5 });

    REQUIRE(p.children().front()->pos().x == 0);
    REQUIRE(p.children().front()->pos().y == 0);
}

TEST_CASE("Panel: child at exact zero relative pos has abs equal to panel abs",
          "[ui][panel][layout][req6.5]")
{
    Panel p = make_panel(30, 40);
    p.add_child(make_label_child(0, 0));

    REQUIRE(p.children().front()->pos().x == 30);
    REQUIRE(p.children().front()->pos().y == 40);
}

// ---------------------------------------------------------------------------
// Req 6.6 — remove_child releases ownership; widget no longer in children list
// ---------------------------------------------------------------------------

TEST_CASE("Panel: remove_child returns the widget and removes it from children",
          "[ui][panel][ownership][req6.6]")
{
    Panel p = make_panel();
    p.add_child(make_label_child(0, 0, "target"));

    REQUIRE(p.children().size() == 1);
    WidgetBase* raw = p.children().front().get();

    auto released = p.remove_child(raw);

    // Ownership transferred back to caller.
    REQUIRE(released != nullptr);
    REQUIRE(released.get() == raw);

    // Panel no longer holds the widget.
    REQUIRE(p.children().empty());
}

TEST_CASE("Panel: remove_child with unknown pointer returns nullptr",
          "[ui][panel][ownership][req6.6]")
{
    Panel p = make_panel();
    p.add_child(make_label_child(0, 0));

    // A pointer that was never added.
    Label stranger{ "x", Pos<int>{ 0, 0 }, Size<uint16_t>{ 10u, 10u } };
    auto result = p.remove_child(&stranger);

    REQUIRE(result == nullptr);
    // Original child is still present.
    REQUIRE(p.children().size() == 1);
}

TEST_CASE("Panel: remove_child on empty panel returns nullptr",
          "[ui][panel][ownership][req6.6]")
{
    Panel p = make_panel();
    Label stranger{ "x", Pos<int>{ 0, 0 }, Size<uint16_t>{ 10u, 10u } };
    REQUIRE(p.remove_child(&stranger) == nullptr);
}

TEST_CASE("Panel: remove_child removes only the targeted widget",
          "[ui][panel][ownership][req6.6]")
{
    Panel p = make_panel();
    p.add_child(make_label_child(0,  0,  "keep_a"));
    p.add_child(make_label_child(10, 10, "remove"));
    p.add_child(make_label_child(20, 20, "keep_b"));

    REQUIRE(p.children().size() == 3);
    WidgetBase* target = p.children()[1].get();

    auto released = p.remove_child(target);

    REQUIRE(released != nullptr);
    REQUIRE(p.children().size() == 2);

    // Verify the remaining children are the ones we expected to keep.
    // After removal the two survivors are at indices 0 and 1.
    bool found_target = false;
    for (const auto& child : p.children()) {
        if (child.get() == target) {
            found_target = true;
        }
    }
    REQUIRE_FALSE(found_target);
}

TEST_CASE("Panel: remove_child marks panel dirty",
          "[ui][panel][ownership][req6.6]")
{
    Panel p = make_panel();
    p.add_child(make_label_child(0, 0));
    p.clear_dirty();

    WidgetBase* raw = p.children().front().get();
    [[maybe_unused]] auto released = p.remove_child(raw);

    REQUIRE(p.is_dirty());
}

// ---------------------------------------------------------------------------
// add_child marks panel dirty
// ---------------------------------------------------------------------------

TEST_CASE("Panel: add_child marks panel dirty",
          "[ui][panel]")
{
    Panel p = make_panel();
    p.clear_dirty();

    p.add_child(make_label_child(0, 0));
    REQUIRE(p.is_dirty());
}

TEST_CASE("Panel: add_child with nullptr is a no-op",
          "[ui][panel]")
{
    Panel p = make_panel();
    p.clear_dirty();

    p.add_child(nullptr);

    REQUIRE(p.children().empty());
    // Dirty flag should not have been set by a no-op add.
    REQUIRE_FALSE(p.is_dirty());
}
