#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "zk/io/widget_parser.hpp"
#include "zk/io/widget_serializer.hpp"
#include "zk/ui/button.hpp"
#include "zk/ui/label.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/window.hpp"

#include <functional>
#include <memory>
#include <string>

// ===========================================================================
// Helpers
// ===========================================================================

/// Builds a minimal Window (no HWND) with one Panel containing a Label and
/// a Button.  Used as the canonical test fixture.
static std::unique_ptr<zk::ui::Window> make_test_window()
{
    using namespace zk;

    auto win = std::make_unique<ui::Window>(
        ui::Window::create_from_config(
            "Test Window",
            metric::Pos<int>{ 100, 200 },
            metric::Size<uint16_t>{ 800, 600 }));

    auto panel = std::make_unique<ui::Panel>(
        metric::Pos<int>{ 10, 20 },
        metric::Size<uint16_t>{ 400, 300 });

    auto label = std::make_unique<ui::Label>(
        "Hello, zketch!",
        metric::Pos<int>{ 5, 5 },
        metric::Size<uint16_t>{ 180, 30 });
    label->set_color(ui::Color{ 0, 0, 0, 255 });

    auto button = std::make_unique<ui::Button>(
        "OK",
        metric::Pos<int>{ 10, 50 },
        metric::Size<uint16_t>{ 80, 30 });

    panel->add_child(std::move(label));
    panel->add_child(std::move(button));
    win->add_panel(std::move(panel));

    return win;
}

// ===========================================================================
// WidgetSerializer tests
// ===========================================================================

TEST_CASE("WidgetSerializer: output is non-empty for a valid Window", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE_FALSE(out.empty());
}

TEST_CASE("WidgetSerializer: output starts with Window keyword", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.rfind("Window", 0) == 0);
}

TEST_CASE("WidgetSerializer: output contains window title", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("Test Window") != std::string::npos);
}

TEST_CASE("WidgetSerializer: output contains Panel keyword", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("Panel") != std::string::npos);
}

TEST_CASE("WidgetSerializer: output contains Label keyword", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("Label") != std::string::npos);
}

TEST_CASE("WidgetSerializer: output contains Button keyword", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("Button") != std::string::npos);
}

TEST_CASE("WidgetSerializer: output contains label text", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("Hello, zketch!") != std::string::npos);
}

TEST_CASE("WidgetSerializer: output contains button label", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("OK") != std::string::npos);
}

TEST_CASE("WidgetSerializer: output ends with newline", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE_FALSE(out.empty());
    REQUIRE(out.back() == '\n');
}

TEST_CASE("WidgetSerializer: pos and size properties are present", "[io][serializer]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.find("pos:") != std::string::npos);
    REQUIRE(out.find("size:") != std::string::npos);
}

TEST_CASE("WidgetSerializer: Window with no panels serializes cleanly", "[io][serializer]")
{
    auto win = std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "Empty",
            zk::metric::Pos<int>{},
            zk::metric::Size<uint16_t>{ 640, 480 }));

    zk::io::WidgetSerializer ser;
    const std::string out = ser.serialize(*win);

    REQUIRE(out.rfind("Window", 0) == 0);
    REQUIRE(out.find("Panel") == std::string::npos);
}

// ===========================================================================
// WidgetParser tests
// ===========================================================================

TEST_CASE("WidgetParser: empty input returns ParseError", "[io][parser]")
{
    zk::io::WidgetParser parser;
    auto result = parser.parse("");

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().line >= 1);
}

TEST_CASE("WidgetParser: missing Window keyword returns ParseError", "[io][parser]")
{
    zk::io::WidgetParser parser;
    auto result = parser.parse("Panel \"root\"\n");

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().line == 1);
    REQUIRE(result.error().column >= 1);
    REQUIRE_FALSE(result.error().message.empty());
}

TEST_CASE("WidgetParser: Window without quoted title returns ParseError", "[io][parser]")
{
    zk::io::WidgetParser parser;
    auto result = parser.parse("Window no_quotes\n");

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().line == 1);
}

TEST_CASE("WidgetParser: minimal valid Window parses successfully", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"main\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
    REQUIRE((*result)->title() == "main");
    REQUIRE((*result)->size().w == 800);
    REQUIRE((*result)->size().h == 600);
}

TEST_CASE("WidgetParser: Window with Panel parses successfully", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 10 20\n"
        "    size: 400 300\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
    REQUIRE((*result)->panels().size() == 1);
}

TEST_CASE("WidgetParser: Panel with Label parses successfully", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: 400 300\n"
        "    Label \"lbl\"\n"
        "      pos: 5 5\n"
        "      size: 180 30\n"
        "      text: \"Hello\"\n"
        "      color: 0 0 0 255\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
    REQUIRE((*result)->panels().size() == 1);
    REQUIRE((*result)->panels()[0]->children().size() == 1);

    const auto* label = dynamic_cast<const zk::ui::Label*>(
        (*result)->panels()[0]->children()[0].get());
    REQUIRE(label != nullptr);
    REQUIRE(label->text() == "Hello");
}

TEST_CASE("WidgetParser: Panel with Button parses successfully", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: 400 300\n"
        "    Button \"btn\"\n"
        "      pos: 10 50\n"
        "      size: 80 30\n"
        "      label: \"OK\"\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
    const auto* btn = dynamic_cast<const zk::ui::Button*>(
        (*result)->panels()[0]->children()[0].get());
    REQUIRE(btn != nullptr);
    REQUIRE(btn->label() == "OK");
}

TEST_CASE("WidgetParser: comment lines are ignored", "[io][parser]")
{
    constexpr std::string_view input =
        "# This is a comment\n"
        "Window \"w\"\n"
        "  # another comment\n"
        "  pos: 0 0\n"
        "  size: 640 480\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
    REQUIRE((*result)->title() == "w");
}

TEST_CASE("WidgetParser: empty lines are ignored", "[io][parser]")
{
    constexpr std::string_view input =
        "\n"
        "Window \"w\"\n"
        "\n"
        "  pos: 0 0\n"
        "  size: 640 480\n"
        "\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
}

TEST_CASE("WidgetParser: invalid pos value returns ParseError with correct line", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: abc def\n"
        "  size: 800 600\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().line == 2);
    REQUIRE_FALSE(result.error().message.empty());
}

TEST_CASE("WidgetParser: invalid size value returns ParseError with correct line", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: bad\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().line == 3);
}

TEST_CASE("WidgetParser: unknown widget type as Window child returns ParseError", "[io][parser]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Label \"lbl\"\n"
        "    pos: 0 0\n"
        "    size: 100 20\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    // Direct children of Window must be Panels
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().line == 4);
}

TEST_CASE("WidgetParser: ParseError has non-empty message", "[io][parser]")
{
    zk::io::WidgetParser parser;
    auto result = parser.parse("garbage input\n");

    REQUIRE_FALSE(result.has_value());
    REQUIRE_FALSE(result.error().message.empty());
}

TEST_CASE("WidgetParser: Window with default size when size omitted", "[io][parser]")
{
    // No size property -- parser should use default 800x600 (Req 4.3)
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n";

    zk::io::WidgetParser parser;
    auto result = parser.parse(input);

    REQUIRE(result.has_value());
    REQUIRE((*result)->size().w == 800);
    REQUIRE((*result)->size().h == 600);
}

// ===========================================================================
// Serializer + Parser integration
// ===========================================================================

TEST_CASE("Serialize then parse: Window title is preserved", "[io][roundtrip]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    const std::string serialized = ser.serialize(*win);
    auto parsed = parser.parse(serialized);

    REQUIRE(parsed.has_value());
    REQUIRE((*parsed)->title() == win->title());
}

TEST_CASE("Serialize then parse: Window size is preserved", "[io][roundtrip]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    const std::string serialized = ser.serialize(*win);
    auto parsed = parser.parse(serialized);

    REQUIRE(parsed.has_value());
    REQUIRE((*parsed)->size().w == win->size().w);
    REQUIRE((*parsed)->size().h == win->size().h);
}

TEST_CASE("Serialize then parse: Panel count is preserved", "[io][roundtrip]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    const std::string serialized = ser.serialize(*win);
    auto parsed = parser.parse(serialized);

    REQUIRE(parsed.has_value());
    REQUIRE((*parsed)->panels().size() == win->panels().size());
}

TEST_CASE("Serialize then parse: child count inside Panel is preserved", "[io][roundtrip]")
{
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    const std::string serialized = ser.serialize(*win);
    auto parsed = parser.parse(serialized);

    REQUIRE(parsed.has_value());
    REQUIRE((*parsed)->panels()[0]->children().size() ==
            win->panels()[0]->children().size());
}

TEST_CASE("Serialize-parse-serialize: output is identical (round-trip property)", "[io][roundtrip]")
{
    // Req 12.5: serialize(parse(serialize(w))) == serialize(w)
    auto win = make_test_window();
    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    const std::string first_pass = ser.serialize(*win);

    auto parsed = parser.parse(first_pass);
    REQUIRE(parsed.has_value());

    const std::string second_pass = ser.serialize(**parsed);

    REQUIRE(first_pass == second_pass);
}

// ===========================================================================
// Property 4 — Serialize-deserialize-serialize round-trip (Req 12.5)
//
// **Validates: Requirements 12.5**
//
// For ALL valid widget hierarchies w:
//   serialize(parse(serialize(w))) == serialize(w)
//
// The test is parameterised over six representative hierarchy shapes using
// Catch2 GENERATE so each shape is an independent test case.
// ===========================================================================

namespace {

/// Factory type: returns a freshly-built Window hierarchy.
using HierarchyFactory = std::function<std::unique_ptr<zk::ui::Window>()>;

// -----------------------------------------------------------------------
// Hierarchy builders
// -----------------------------------------------------------------------

/// 1. Minimal — Window only, no panels.
static std::unique_ptr<zk::ui::Window> make_minimal_window()
{
    return std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "Minimal",
            zk::metric::Pos<int>{ 0, 0 },
            zk::metric::Size<uint16_t>{ 800, 600 }));
}

/// 2. Window + one empty Panel.
static std::unique_ptr<zk::ui::Window> make_window_with_panel()
{
    auto win = std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "WithPanel",
            zk::metric::Pos<int>{ 10, 20 },
            zk::metric::Size<uint16_t>{ 1024, 768 }));

    win->add_panel(std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 5, 5 },
        zk::metric::Size<uint16_t>{ 200, 150 }));

    return win;
}

/// 3. Window + Panel + Label.
static std::unique_ptr<zk::ui::Window> make_window_panel_label()
{
    auto win = std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "WithLabel",
            zk::metric::Pos<int>{ 0, 0 },
            zk::metric::Size<uint16_t>{ 640, 480 }));

    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 300, 200 });

    auto label = std::make_unique<zk::ui::Label>(
        "Hello",
        zk::metric::Pos<int>{ 4, 4 },
        zk::metric::Size<uint16_t>{ 100, 20 });
    label->set_color(zk::ui::Color{ 255, 0, 0, 255 });

    panel->add_child(std::move(label));
    win->add_panel(std::move(panel));
    return win;
}

/// 4. Window + Panel + Button.
static std::unique_ptr<zk::ui::Window> make_window_panel_button()
{
    auto win = std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "WithButton",
            zk::metric::Pos<int>{ 50, 50 },
            zk::metric::Size<uint16_t>{ 800, 600 }));

    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 10, 10 },
        zk::metric::Size<uint16_t>{ 400, 300 });

    panel->add_child(std::make_unique<zk::ui::Button>(
        "Submit",
        zk::metric::Pos<int>{ 20, 20 },
        zk::metric::Size<uint16_t>{ 120, 40 }));

    win->add_panel(std::move(panel));
    return win;
}

/// 5. Deeply nested — Panel containing a Panel containing a Label.
static std::unique_ptr<zk::ui::Window> make_deeply_nested()
{
    auto win = std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "DeepNest",
            zk::metric::Pos<int>{ 0, 0 },
            zk::metric::Size<uint16_t>{ 800, 600 }));

    auto outer = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 600, 400 });

    auto inner = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 10, 10 },
        zk::metric::Size<uint16_t>{ 200, 100 });

    auto label = std::make_unique<zk::ui::Label>(
        "Nested",
        zk::metric::Pos<int>{ 2, 2 },
        zk::metric::Size<uint16_t>{ 80, 20 });
    label->set_color(zk::ui::Color{ 0, 128, 0, 255 });

    inner->add_child(std::move(label));
    outer->add_child(std::move(inner));
    win->add_panel(std::move(outer));
    return win;
}

/// 6. Multiple children — Panel with a Label, a Button, and another Label.
static std::unique_ptr<zk::ui::Window> make_multiple_children()
{
    auto win = std::make_unique<zk::ui::Window>(
        zk::ui::Window::create_from_config(
            "MultiChild",
            zk::metric::Pos<int>{ 0, 0 },
            zk::metric::Size<uint16_t>{ 1280, 720 }));

    auto panel = std::make_unique<zk::ui::Panel>(
        zk::metric::Pos<int>{ 0, 0 },
        zk::metric::Size<uint16_t>{ 500, 400 });

    auto lbl1 = std::make_unique<zk::ui::Label>(
        "Title",
        zk::metric::Pos<int>{ 5, 5 },
        zk::metric::Size<uint16_t>{ 200, 30 });
    lbl1->set_color(zk::ui::Color{ 0, 0, 0, 255 });

    auto btn = std::make_unique<zk::ui::Button>(
        "Cancel",
        zk::metric::Pos<int>{ 5, 40 },
        zk::metric::Size<uint16_t>{ 80, 30 });

    auto lbl2 = std::make_unique<zk::ui::Label>(
        "Footer",
        zk::metric::Pos<int>{ 5, 80 },
        zk::metric::Size<uint16_t>{ 200, 20 });
    lbl2->set_color(zk::ui::Color{ 128, 128, 128, 255 });

    panel->add_child(std::move(lbl1));
    panel->add_child(std::move(btn));
    panel->add_child(std::move(lbl2));
    win->add_panel(std::move(panel));
    return win;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Property test
// ---------------------------------------------------------------------------

// **Validates: Requirements 12.5**
TEST_CASE("Property 4: serialize-parse-serialize round-trip holds for all valid widget hierarchies",
          "[io][roundtrip][property]")
{
    // Each entry is a (description, factory) pair.  GENERATE picks one per
    // test run so Catch2 reports each hierarchy as a separate sub-case.
    auto [description, factory] = GENERATE(table<const char*, HierarchyFactory>({
        { "minimal Window only",                  make_minimal_window      },
        { "Window + Panel",                       make_window_with_panel   },
        { "Window + Panel + Label",               make_window_panel_label  },
        { "Window + Panel + Button",              make_window_panel_button },
        { "deeply nested Panel",                  make_deeply_nested       },
        { "Panel with multiple children",         make_multiple_children   },
    }));

    INFO("Hierarchy: " << description);

    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    // Build the hierarchy
    auto w = factory();
    REQUIRE(w != nullptr);

    // s1 = serialize(w)
    const std::string s1 = ser.serialize(*w);
    REQUIRE_FALSE(s1.empty());

    // w2 = parse(s1)  — must succeed
    auto w2 = parser.parse(s1);
    REQUIRE(w2.has_value());

    // s2 = serialize(w2)
    const std::string s2 = ser.serialize(**w2);

    // Core property: s1 == s2
    REQUIRE(s1 == s2);
}

// ===========================================================================
// Property 5 — Deserialize-serialize-deserialize round-trip (Req 12.6)
//
// **Validates: Requirements 12.6**
//
// For ALL valid Widget_Config_Format inputs `s`:
//   parse(serialize(parse(s))) is structurally equivalent to parse(s)
//
// "Structurally equivalent" means:
//   - Same Window title, pos, size
//   - Same number of top-level Panels
//   - For each Panel: same pos, size, and child count
//   - For each child: same concrete type, pos, size, and type-specific data
//     (Label.text / Label.color / Button.label)
//
// The test is parameterised over six representative Widget_Config_Format
// strings using Catch2 GENERATE so each input is an independent sub-case.
// ===========================================================================

namespace {

// ---------------------------------------------------------------------------
// Structural equality helpers
// ---------------------------------------------------------------------------

/// Compares two WidgetBase children for structural equality.
/// Dispatches to the concrete type (Label, Button, Panel).
bool widgets_structurally_equal(const zk::ui::WidgetBase& a,
                                const zk::ui::WidgetBase& b);

bool panels_structurally_equal(const zk::ui::Panel& a,
                               const zk::ui::Panel& b)
{
    if (a.pos().x  != b.pos().x  || a.pos().y  != b.pos().y)  return false;
    if (a.size().w != b.size().w || a.size().h != b.size().h)  return false;
    if (a.children().size() != b.children().size())             return false;

    for (std::size_t i = 0; i < a.children().size(); ++i) {
        if (!widgets_structurally_equal(*a.children()[i], *b.children()[i]))
            return false;
    }
    return true;
}

bool widgets_structurally_equal(const zk::ui::WidgetBase& a,
                                const zk::ui::WidgetBase& b)
{
    // Both must be the same concrete type
    if (typeid(a) != typeid(b)) return false;

    if (const auto* pa = dynamic_cast<const zk::ui::Panel*>(&a)) {
        const auto* pb = dynamic_cast<const zk::ui::Panel*>(&b);
        return pb && panels_structurally_equal(*pa, *pb);
    }

    if (const auto* la = dynamic_cast<const zk::ui::Label*>(&a)) {
        const auto* lb = dynamic_cast<const zk::ui::Label*>(&b);
        if (!lb) return false;
        return la->pos().x  == lb->pos().x  &&
               la->pos().y  == lb->pos().y  &&
               la->size().w == lb->size().w &&
               la->size().h == lb->size().h &&
               la->text()   == lb->text()   &&
               la->color().r == lb->color().r &&
               la->color().g == lb->color().g &&
               la->color().b == lb->color().b &&
               la->color().a == lb->color().a;
    }

    if (const auto* ba = dynamic_cast<const zk::ui::Button*>(&a)) {
        const auto* bb = dynamic_cast<const zk::ui::Button*>(&b);
        if (!bb) return false;
        return ba->pos().x  == bb->pos().x  &&
               ba->pos().y  == bb->pos().y  &&
               ba->size().w == bb->size().w &&
               ba->size().h == bb->size().h &&
               ba->label()  == bb->label();
    }

    return false;
}

bool windows_structurally_equal(const zk::ui::Window& a,
                                const zk::ui::Window& b)
{
    if (a.title()  != b.title())  return false;
    if (a.pos().x  != b.pos().x  || a.pos().y  != b.pos().y)  return false;
    if (a.size().w != b.size().w || a.size().h != b.size().h)  return false;
    if (a.panels().size() != b.panels().size())                 return false;

    for (std::size_t i = 0; i < a.panels().size(); ++i) {
        if (!panels_structurally_equal(*a.panels()[i], *b.panels()[i]))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Canonical Widget_Config_Format strings used as test inputs
// ---------------------------------------------------------------------------

/// 1. Minimal — Window only, no panels.
constexpr std::string_view kMinimalWindow =
    "Window \"Minimal\"\n"
    "  pos: 0 0\n"
    "  size: 800 600\n";

/// 2. Window + one empty Panel.
constexpr std::string_view kWindowWithPanel =
    "Window \"WithPanel\"\n"
    "  pos: 10 20\n"
    "  size: 1024 768\n"
    "  Panel \"panel\"\n"
    "    pos: 5 5\n"
    "    size: 200 150\n";

/// 3. Window + Panel + Label.
constexpr std::string_view kWindowPanelLabel =
    "Window \"WithLabel\"\n"
    "  pos: 0 0\n"
    "  size: 640 480\n"
    "  Panel \"panel\"\n"
    "    pos: 0 0\n"
    "    size: 300 200\n"
    "    Label \"label\"\n"
    "      pos: 4 4\n"
    "      size: 100 20\n"
    "      text: \"Hello\"\n"
    "      color: 255 0 0 255\n"
    "      font_family: Segoe UI\n"
    "      font_size: 12\n"
    "      font_bold: false\n"
    "      font_italic: false\n";

/// 4. Window + Panel + Button.
constexpr std::string_view kWindowPanelButton =
    "Window \"WithButton\"\n"
    "  pos: 50 50\n"
    "  size: 800 600\n"
    "  Panel \"panel\"\n"
    "    pos: 10 10\n"
    "    size: 400 300\n"
    "    Button \"button\"\n"
    "      pos: 20 20\n"
    "      size: 120 40\n"
    "      label: \"Submit\"\n";

/// 5. Deeply nested — Panel containing a Panel containing a Label.
constexpr std::string_view kDeeplyNested =
    "Window \"DeepNest\"\n"
    "  pos: 0 0\n"
    "  size: 800 600\n"
    "  Panel \"panel\"\n"
    "    pos: 0 0\n"
    "    size: 600 400\n"
    "    Panel \"panel\"\n"
    "      pos: 10 10\n"
    "      size: 200 100\n"
    "      Label \"label\"\n"
    "        pos: 2 2\n"
    "        size: 80 20\n"
    "        text: \"Nested\"\n"
    "        color: 0 128 0 255\n"
    "        font_family: Segoe UI\n"
    "        font_size: 12\n"
    "        font_bold: false\n"
    "        font_italic: false\n";

/// 6. Multiple children — Panel with a Label, a Button, and another Label.
constexpr std::string_view kMultipleChildren =
    "Window \"MultiChild\"\n"
    "  pos: 0 0\n"
    "  size: 1280 720\n"
    "  Panel \"panel\"\n"
    "    pos: 0 0\n"
    "    size: 500 400\n"
    "    Label \"label\"\n"
    "      pos: 5 5\n"
    "      size: 200 30\n"
    "      text: \"Title\"\n"
    "      color: 0 0 0 255\n"
    "      font_family: Segoe UI\n"
    "      font_size: 12\n"
    "      font_bold: false\n"
    "      font_italic: false\n"
    "    Button \"button\"\n"
    "      pos: 5 40\n"
    "      size: 80 30\n"
    "      label: \"Cancel\"\n"
    "    Label \"label\"\n"
    "      pos: 5 80\n"
    "      size: 200 20\n"
    "      text: \"Footer\"\n"
    "      color: 128 128 128 255\n"
    "      font_family: Segoe UI\n"
    "      font_size: 12\n"
    "      font_bold: false\n"
    "      font_italic: false\n";

} // anonymous namespace

// ---------------------------------------------------------------------------
// Property test
// ---------------------------------------------------------------------------

// **Validates: Requirements 12.6**
TEST_CASE("Property 5: parse-serialize-parse round-trip produces structurally equivalent hierarchies",
          "[io][roundtrip][property]")
{
    // Each entry is a (description, Widget_Config_Format string) pair.
    // GENERATE picks one per test run so Catch2 reports each input as a
    // separate sub-case.
    auto [description, input] = GENERATE(table<const char*, std::string_view>({
        { "minimal Window only",              kMinimalWindow      },
        { "Window + Panel",                   kWindowWithPanel    },
        { "Window + Panel + Label",           kWindowPanelLabel   },
        { "Window + Panel + Button",          kWindowPanelButton  },
        { "deeply nested Panel",              kDeeplyNested       },
        { "Panel with multiple children",     kMultipleChildren   },
    }));

    INFO("Input: " << description);

    zk::io::WidgetSerializer ser;
    zk::io::WidgetParser     parser;

    // w1 = parse(input)  — first parse must succeed
    auto w1 = parser.parse(input);
    REQUIRE(w1.has_value());

    // s1 = serialize(w1)
    const std::string s1 = ser.serialize(**w1);
    REQUIRE_FALSE(s1.empty());

    // w2 = parse(s1)  — second parse must also succeed
    auto w2 = parser.parse(s1);
    REQUIRE(w2.has_value());

    // Core property: w1 and w2 are structurally equivalent (Req 12.6)
    REQUIRE(windows_structurally_equal(**w1, **w2));
}
