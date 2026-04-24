/// test_widget_parser_errors.cpp
///
/// Unit tests for WidgetParser error handling (Requirement 12.3).
///
/// Each test verifies that a specific malformed Widget_Config_Format input:
///   1. Returns a ParseError (not a valid Window).
///   2. Reports the correct 1-based line number.
///   3. Reports a plausible 1-based column number (>= 1).
///   4. Provides a non-empty, human-readable error message.
///
/// A separate section verifies that empty / whitespace-only input is handled
/// gracefully without crashing.

#include <catch2/catch_test_macros.hpp>

#include "zk/io/widget_parser.hpp"
#include "zk/ui/window.hpp"   // required: unique_ptr<Window> destructor needs complete type

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Convenience: parse `input` and assert it fails, then return the error.
static zk::io::ParseError must_fail(std::string_view input)
{
    zk::io::WidgetParser parser;
    auto result = parser.parse(input);
    REQUIRE_FALSE(result.has_value());
    return result.error();
}

// ===========================================================================
// Section 1 - Empty / whitespace-only input
// ===========================================================================

TEST_CASE("WidgetParser error: empty string returns ParseError", "[io][parser][error]")
{
    const auto err = must_fail("");

    // Line must be reported (>= 1 is the minimum sensible value)
    CHECK(err.line >= 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: whitespace-only input returns ParseError", "[io][parser][error]")
{
    const auto err = must_fail("   \n   \n   \n");

    CHECK(err.line >= 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: only comments returns ParseError", "[io][parser][error]")
{
    constexpr std::string_view input =
        "# just a comment\n"
        "# another comment\n";

    const auto err = must_fail(input);

    CHECK(err.line >= 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: only empty lines returns ParseError", "[io][parser][error]")
{
    const auto err = must_fail("\n\n\n");

    CHECK(err.line >= 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 2 - Wrong root keyword
// ===========================================================================

TEST_CASE("WidgetParser error: root keyword is Panel (not Window), line 1", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Panel \"root\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n";

    const auto err = must_fail(input);

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: root keyword is Label (not Window), line 1", "[io][parser][error]")
{
    const auto err = must_fail("Label \"lbl\"\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: root keyword is Button (not Window), line 1", "[io][parser][error]")
{
    const auto err = must_fail("Button \"btn\"\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: completely unknown root keyword, line 1", "[io][parser][error]")
{
    const auto err = must_fail("Foobar \"x\"\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 3 - Missing or malformed Window title
// ===========================================================================

TEST_CASE("WidgetParser error: Window with no title at all, line 1", "[io][parser][error]")
{
    // 'Window' keyword present but nothing follows
    const auto err = must_fail("Window\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: Window title not quoted, line 1", "[io][parser][error]")
{
    const auto err = must_fail("Window main_window\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: Window title quote not closed, line 1", "[io][parser][error]")
{
    const auto err = must_fail("Window \"unclosed\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 4 - Window indented (must start at column 1)
// ===========================================================================

TEST_CASE("WidgetParser error: Window declaration indented, line 1", "[io][parser][error]")
{
    const auto err = must_fail("  Window \"w\"\n");

    CHECK(err.line == 1);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 5 - Malformed property values
// ===========================================================================

TEST_CASE("WidgetParser error: invalid pos value on Window, line 2", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: abc def\n"   // line 2 — non-integer values
        "  size: 800 600\n";

    const auto err = must_fail(input);

    CHECK(err.line == 2);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: pos with only one integer, line 2", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 100\n"       // line 2 — missing second integer
        "  size: 800 600\n";

    const auto err = must_fail(input);

    CHECK(err.line == 2);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: invalid size value on Window, line 3", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: bad\n";     // line 3 — non-integer

    const auto err = must_fail(input);

    CHECK(err.line == 3);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: size with only one integer, line 3", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800\n";     // line 3 — missing height

    const auto err = must_fail(input);

    CHECK(err.line == 3);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: invalid pos value inside Panel, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: x y\n"     // line 5 — non-integer
        "    size: 200 100\n";

    const auto err = must_fail(input);

    CHECK(err.line == 5);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: invalid size value inside Panel, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: oops\n";  // line 6

    const auto err = must_fail(input);

    CHECK(err.line == 6);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: invalid color value inside Label, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: 400 300\n"
        "    Label \"lbl\"\n"
        "      pos: 0 0\n"
        "      size: 100 20\n"
        "      color: red green blue alpha\n";  // line 10 — non-integer RGBA

    const auto err = must_fail(input);

    CHECK(err.line == 10);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 6 - Wrong widget type as direct child of Window
// ===========================================================================

TEST_CASE("WidgetParser error: Label as direct child of Window, line 4", "[io][parser][error]")
{
    // Direct children of Window must be Panels (Req 6.1)
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Label \"lbl\"\n"  // line 4 — Label not allowed here
        "    pos: 0 0\n"
        "    size: 100 20\n";

    const auto err = must_fail(input);

    CHECK(err.line == 4);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: Button as direct child of Window, line 4", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Button \"btn\"\n"  // line 4
        "    pos: 0 0\n"
        "    size: 80 30\n";

    const auto err = must_fail(input);

    CHECK(err.line == 4);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: unknown widget type inside Panel, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: 400 300\n"
        "    Slider \"s\"\n"  // line 7 — unknown widget type
        "      pos: 0 0\n"
        "      size: 200 20\n";

    const auto err = must_fail(input);

    CHECK(err.line == 7);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 7 - Missing quoted id on widget declarations
// ===========================================================================

TEST_CASE("WidgetParser error: Panel without quoted id, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel no_quotes\n"  // line 4 — id not quoted
        "    pos: 0 0\n"
        "    size: 200 100\n";

    const auto err = must_fail(input);

    CHECK(err.line == 4);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: Label without quoted id, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: 400 300\n"
        "    Label no_quotes\n"  // line 7
        "      pos: 0 0\n"
        "      size: 100 20\n";

    const auto err = must_fail(input);

    CHECK(err.line == 7);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

TEST_CASE("WidgetParser error: Button without quoted id, correct line reported", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "  Panel \"p\"\n"
        "    pos: 0 0\n"
        "    size: 400 300\n"
        "    Button no_quotes\n"  // line 7
        "      pos: 0 0\n"
        "      size: 80 30\n";

    const auto err = must_fail(input);

    CHECK(err.line == 7);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 8 - Trailing content after Window definition
// ===========================================================================

TEST_CASE("WidgetParser error: extra content after Window definition returns ParseError", "[io][parser][error]")
{
    constexpr std::string_view input =
        "Window \"w\"\n"
        "  pos: 0 0\n"
        "  size: 800 600\n"
        "Window \"second\"\n"  // line 4 — second Window not supported
        "  pos: 0 0\n"
        "  size: 640 480\n";

    const auto err = must_fail(input);

    CHECK(err.line == 4);
    CHECK(err.column >= 1);
    CHECK_FALSE(err.message.empty());
}

// ===========================================================================
// Section 9 - ParseError fields are always well-formed
// ===========================================================================

TEST_CASE("WidgetParser error: ParseError always has line >= 1", "[io][parser][error]")
{
    // Verify the invariant across several distinct malformed inputs
    const std::string_view inputs[] = {
        "",
        "garbage\n",
        "Window\n",
        "Window \"w\"\n  pos: bad\n",
        "  Window \"w\"\n",
    };

    zk::io::WidgetParser parser;
    for (auto input : inputs) {
        auto result = parser.parse(input);
        REQUIRE_FALSE(result.has_value());
        INFO("Input: \"" << input << "\"");
        CHECK(result.error().line >= 1);
    }
}

TEST_CASE("WidgetParser error: ParseError always has column >= 1", "[io][parser][error]")
{
    const std::string_view inputs[] = {
        "",
        "garbage\n",
        "Window\n",
        "Window \"w\"\n  size: bad\n",
        "  Window \"w\"\n",
    };

    zk::io::WidgetParser parser;
    for (auto input : inputs) {
        auto result = parser.parse(input);
        REQUIRE_FALSE(result.has_value());
        INFO("Input: \"" << input << "\"");
        CHECK(result.error().column >= 1);
    }
}

TEST_CASE("WidgetParser error: ParseError message is always non-empty", "[io][parser][error]")
{
    const std::string_view inputs[] = {
        "",
        "garbage\n",
        "Window\n",
        "Window \"w\"\n  pos: bad\n",
        "  Window \"w\"\n",
    };

    zk::io::WidgetParser parser;
    for (auto input : inputs) {
        auto result = parser.parse(input);
        REQUIRE_FALSE(result.has_value());
        INFO("Input: \"" << input << "\"");
        CHECK_FALSE(result.error().message.empty());
    }
}
