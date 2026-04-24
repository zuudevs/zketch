#include "zk/io/widget_parser.hpp"

#include "zk/ui/button.hpp"
#include "zk/ui/label.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/widget_base.hpp"
#include "zk/ui/window.hpp"

#include <charconv>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace zk::io {

// ===========================================================================
// Internal parser state
// ===========================================================================

namespace {

// ---------------------------------------------------------------------------
// Line representation
// ---------------------------------------------------------------------------

struct Line {
    int          number{0};   ///< 1-based line number in the source
    int          indent{0};   ///< Number of leading spaces
    std::string_view content; ///< Trimmed content (no leading/trailing whitespace)
};

// ---------------------------------------------------------------------------
// Tokenised lines from the input
// ---------------------------------------------------------------------------

/// Splits `input` into non-empty, non-comment lines and records their
/// indentation level and 1-based line number.
std::vector<Line> tokenize(std::string_view input)
{
    std::vector<Line> lines;
    int line_num = 0;
    std::size_t pos = 0;

    while (pos <= input.size()) {
        // Find end of current line
        std::size_t end = input.find('\n', pos);
        if (end == std::string_view::npos) end = input.size();

        ++line_num;
        std::string_view raw = input.substr(pos, end - pos);

        // Strip trailing \r (Windows line endings)
        if (!raw.empty() && raw.back() == '\r') raw.remove_suffix(1);

        // Count leading spaces (indentation)
        int indent = 0;
        std::size_t i = 0;
        while (i < raw.size() && raw[i] == ' ') { ++indent; ++i; }

        std::string_view trimmed = raw.substr(i);

        // Skip empty lines and comment lines
        if (!trimmed.empty() && trimmed[0] != '#') {
            lines.push_back(Line{ line_num, indent, trimmed });
        }

        pos = end + 1;
    }

    return lines;
}

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------

/// Returns a ParseError at the given line/column.
ParseError make_parse_error(int line, int col, std::string msg)
{
    return ParseError{ line, col, std::move(msg) };
}

/// Attempts to parse a quoted string `"..."` from `sv`.
/// Returns the unquoted content, or nullopt on failure.
std::optional<std::string_view> parse_quoted(std::string_view sv)
{
    if (sv.size() < 2 || sv.front() != '"') return std::nullopt;
    std::size_t close = sv.find('"', 1);
    if (close == std::string_view::npos) return std::nullopt;
    return sv.substr(1, close - 1);
}

/// Splits `content` into a keyword and the remainder after the first space.
/// e.g. `"Window \"main\""` → keyword=`"Window"`, rest=`"\"main\""`
std::pair<std::string_view, std::string_view>
split_keyword(std::string_view content)
{
    std::size_t sp = content.find(' ');
    if (sp == std::string_view::npos) return { content, {} };
    return { content.substr(0, sp), content.substr(sp + 1) };
}

/// Parses a `key: value` property line.
/// Returns {key, value} or nullopt if the format is wrong.
std::optional<std::pair<std::string_view, std::string_view>>
parse_property(std::string_view content)
{
    std::size_t colon = content.find(':');
    if (colon == std::string_view::npos) return std::nullopt;

    std::string_view key = content.substr(0, colon);
    // Trim trailing spaces from key
    while (!key.empty() && key.back() == ' ') key.remove_suffix(1);

    std::string_view val = content.substr(colon + 1);
    // Trim leading spaces from value
    while (!val.empty() && val.front() == ' ') val.remove_prefix(1);

    return std::pair{ key, val };
}

/// Parses two integers separated by a space from `sv`.
/// Returns {a, b} or nullopt on failure.
std::optional<std::pair<int, int>> parse_two_ints(std::string_view sv)
{
    // Find the space separator
    std::size_t sp = sv.find(' ');
    if (sp == std::string_view::npos) return std::nullopt;

    std::string_view a_sv = sv.substr(0, sp);
    std::string_view b_sv = sv.substr(sp + 1);
    // Trim leading spaces from second value
    while (!b_sv.empty() && b_sv.front() == ' ') b_sv.remove_prefix(1);

    int a{}, b{};
    auto [pa, ea] = std::from_chars(a_sv.data(), a_sv.data() + a_sv.size(), a);
    if (ea != std::errc{}) return std::nullopt;
    auto [pb, eb] = std::from_chars(b_sv.data(), b_sv.data() + b_sv.size(), b);
    if (eb != std::errc{}) return std::nullopt;

    return std::pair{ a, b };
}

/// Parses four integers separated by spaces from `sv`.
/// Returns {r,g,b,a} or nullopt on failure.
std::optional<std::array<int, 4>> parse_four_ints(std::string_view sv)
{
    std::array<int, 4> vals{};
    std::string_view rest = sv;
    for (int i = 0; i < 4; ++i) {
        // Trim leading spaces
        while (!rest.empty() && rest.front() == ' ') rest.remove_prefix(1);
        if (rest.empty()) return std::nullopt;

        auto [ptr, ec] = std::from_chars(rest.data(), rest.data() + rest.size(), vals[i]);
        if (ec != std::errc{}) return std::nullopt;
        rest = rest.substr(static_cast<std::size_t>(ptr - rest.data()));
    }
    return vals;
}

// ---------------------------------------------------------------------------
// Forward declarations for recursive descent
// ---------------------------------------------------------------------------

struct ParseContext {
    const std::vector<Line>& lines;
    std::size_t              cursor{0};

    bool at_end() const noexcept { return cursor >= lines.size(); }
    const Line& current() const  { return lines[cursor]; }
};

using ParseResult = std::expected<std::unique_ptr<ui::WidgetBase>, ParseError>;
using WindowResult = std::expected<std::unique_ptr<ui::Window>, ParseError>;

// Forward declarations
ParseResult parse_panel (ParseContext& ctx, int expected_indent);
ParseResult parse_label (ParseContext& ctx, int expected_indent);
ParseResult parse_button(ParseContext& ctx, int expected_indent);

// ---------------------------------------------------------------------------
// Property application helpers — kept for potential future use
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Panel parser
// ---------------------------------------------------------------------------

ParseResult parse_panel(ParseContext& ctx, int expected_indent)
{
    const Line& decl_line = ctx.current();

    // Validate: `Panel "id"`
    auto [kw, rest] = split_keyword(decl_line.content);
    if (kw != "Panel") {
        return std::unexpected(make_parse_error(
            decl_line.number, decl_line.indent + 1,
            "Expected 'Panel' keyword"));
    }

    auto id = parse_quoted(rest);
    if (!id) {
        return std::unexpected(make_parse_error(
            decl_line.number,
            decl_line.indent + static_cast<int>(kw.size()) + 2,
            "Expected quoted id after 'Panel'"));
    }

    ++ctx.cursor;

    // Default pos/size
    metric::Pos<int>        pos{};
    metric::Size<uint16_t>  sz{};

    // Consume property lines at depth (expected_indent + 2).
    // A line is a property if it contains ':' before any space.
    // A line is a child widget if it starts with a known keyword.
    const int child_indent = expected_indent + 2;
    while (!ctx.at_end() && ctx.current().indent == child_indent) {
        const Line& prop_line = ctx.current();

        // Peek: is this a widget declaration or a property?
        auto [peek_kw, peek_rest] = split_keyword(prop_line.content);
        const bool is_widget_decl = (peek_kw == "Panel"  ||
                                     peek_kw == "Label"  ||
                                     peek_kw == "Button");
        if (is_widget_decl) break;  // hand off to child loop below

        auto kv = parse_property(prop_line.content);
        if (!kv) {
            return std::unexpected(make_parse_error(
                prop_line.number, prop_line.indent + 1,
                "Malformed property line"));
        }

        auto [key, val] = *kv;
        if (key == "pos") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'pos' value — expected two integers"));
            }
            pos = metric::Pos<int>{ pair->first, pair->second };
        } else if (key == "size") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'size' value — expected two integers"));
            }
            sz = metric::Size<uint16_t>{
                static_cast<uint16_t>(pair->first),
                static_cast<uint16_t>(pair->second)
            };
        }
        // Unknown properties are silently ignored for forward compatibility
        ++ctx.cursor;
    }

    auto panel = std::make_unique<ui::Panel>(pos, sz);

    // Consume child widgets at depth (expected_indent + 2)
    // (child_indent already defined above)
    while (!ctx.at_end() && ctx.current().indent == child_indent) {
        const Line& child_line = ctx.current();
        auto [child_kw, child_rest] = split_keyword(child_line.content);

        ParseResult child_result = std::unexpected(make_parse_error(
            child_line.number, child_line.indent + 1,
            std::string("Unknown widget type: ").append(child_kw)));

        if (child_kw == "Panel") {
            child_result = parse_panel(ctx, child_indent);
        } else if (child_kw == "Label") {
            child_result = parse_label(ctx, child_indent);
        } else if (child_kw == "Button") {
            child_result = parse_button(ctx, child_indent);
        } else {
            return std::unexpected(make_parse_error(
                child_line.number, child_line.indent + 1,
                std::string("Unknown widget type: ").append(child_kw)));
        }

        if (!child_result) return std::unexpected(child_result.error());
        panel->add_child(std::move(*child_result));
    }

    return panel;
}

// ---------------------------------------------------------------------------
// Label parser
// ---------------------------------------------------------------------------

ParseResult parse_label(ParseContext& ctx, int expected_indent)
{
    const Line& decl_line = ctx.current();

    auto [kw, rest] = split_keyword(decl_line.content);
    if (kw != "Label") {
        return std::unexpected(make_parse_error(
            decl_line.number, decl_line.indent + 1,
            "Expected 'Label' keyword"));
    }

    auto id = parse_quoted(rest);
    if (!id) {
        return std::unexpected(make_parse_error(
            decl_line.number,
            decl_line.indent + static_cast<int>(kw.size()) + 2,
            "Expected quoted id after 'Label'"));
    }

    ++ctx.cursor;

    // Defaults
    metric::Pos<int>        pos{};
    metric::Size<uint16_t>  sz{};
    std::string             text;
    ui::Color               color{ 0, 0, 0, 255 };
    ui::FontConfig          font{};

    const int prop_indent = expected_indent + 2;
    while (!ctx.at_end() && ctx.current().indent == prop_indent) {
        const Line& prop_line = ctx.current();
        auto kv = parse_property(prop_line.content);
        if (!kv) {
            return std::unexpected(make_parse_error(
                prop_line.number, prop_line.indent + 1,
                "Malformed property line"));
        }

        auto [key, val] = *kv;
        if (key == "pos") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'pos' value"));
            }
            pos = metric::Pos<int>{ pair->first, pair->second };
        } else if (key == "size") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'size' value"));
            }
            sz = metric::Size<uint16_t>{
                static_cast<uint16_t>(pair->first),
                static_cast<uint16_t>(pair->second)
            };
        } else if (key == "text") {
            // Value may be quoted: `"Hello"` or unquoted
            auto quoted = parse_quoted(val);
            text = quoted ? std::string(*quoted) : std::string(val);
        } else if (key == "color") {
            auto rgba = parse_four_ints(val);
            if (!rgba) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'color' value — expected four integers (r g b a)"));
            }
            color = ui::Color{
                static_cast<uint8_t>((*rgba)[0]),
                static_cast<uint8_t>((*rgba)[1]),
                static_cast<uint8_t>((*rgba)[2]),
                static_cast<uint8_t>((*rgba)[3])
            };
        } else if (key == "font_family") {
            font.family = std::string(val);
        } else if (key == "font_size") {
            float sz_pt = 12.0f;
            // std::from_chars for float requires C++17; use a simple fallback
            std::string tmp(val);
            try { sz_pt = std::stof(tmp); } catch (...) {}
            font.size_pt = sz_pt;
        } else if (key == "font_bold") {
            font.bold = (val == "true");
        } else if (key == "font_italic") {
            font.italic = (val == "true");
        }
        // Unknown properties silently ignored
        ++ctx.cursor;
    }

    auto label = std::make_unique<ui::Label>(text, pos, sz);
    label->set_color(color);
    label->set_font(font);
    return label;
}

// ---------------------------------------------------------------------------
// Button parser
// ---------------------------------------------------------------------------

ParseResult parse_button(ParseContext& ctx, int expected_indent)
{
    const Line& decl_line = ctx.current();

    auto [kw, rest] = split_keyword(decl_line.content);
    if (kw != "Button") {
        return std::unexpected(make_parse_error(
            decl_line.number, decl_line.indent + 1,
            "Expected 'Button' keyword"));
    }

    auto id = parse_quoted(rest);
    if (!id) {
        return std::unexpected(make_parse_error(
            decl_line.number,
            decl_line.indent + static_cast<int>(kw.size()) + 2,
            "Expected quoted id after 'Button'"));
    }

    ++ctx.cursor;

    metric::Pos<int>        pos{};
    metric::Size<uint16_t>  sz{};
    std::string             label_text;

    const int prop_indent = expected_indent + 2;
    while (!ctx.at_end() && ctx.current().indent == prop_indent) {
        const Line& prop_line = ctx.current();
        auto kv = parse_property(prop_line.content);
        if (!kv) {
            return std::unexpected(make_parse_error(
                prop_line.number, prop_line.indent + 1,
                "Malformed property line"));
        }

        auto [key, val] = *kv;
        if (key == "pos") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'pos' value"));
            }
            pos = metric::Pos<int>{ pair->first, pair->second };
        } else if (key == "size") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'size' value"));
            }
            sz = metric::Size<uint16_t>{
                static_cast<uint16_t>(pair->first),
                static_cast<uint16_t>(pair->second)
            };
        } else if (key == "label") {
            auto quoted = parse_quoted(val);
            label_text = quoted ? std::string(*quoted) : std::string(val);
        }
        ++ctx.cursor;
    }

    return std::make_unique<ui::Button>(label_text, pos, sz);
}

// ---------------------------------------------------------------------------
// Window parser
// ---------------------------------------------------------------------------

WindowResult parse_window(ParseContext& ctx)
{
    if (ctx.at_end()) {
        return std::unexpected(make_parse_error(1, 1, "Empty input"));
    }

    const Line& decl_line = ctx.current();

    // Must start at indent 0
    if (decl_line.indent != 0) {
        return std::unexpected(make_parse_error(
            decl_line.number, 1,
            "Window declaration must start at column 1 (no leading spaces)"));
    }

    auto [kw, rest] = split_keyword(decl_line.content);
    if (kw != "Window") {
        return std::unexpected(make_parse_error(
            decl_line.number, 1,
            std::string("Expected 'Window' keyword, got: ").append(kw)));
    }

    auto id = parse_quoted(rest);
    if (!id) {
        return std::unexpected(make_parse_error(
            decl_line.number,
            static_cast<int>(kw.size()) + 2,
            "Expected quoted title after 'Window'"));
    }

    std::string title(*id);
    ++ctx.cursor;

    // Window properties at indent 2
    metric::Pos<int>        pos{};
    metric::Size<uint16_t>  sz{ 800, 600 };  // default size per Req 4.3

    const int prop_indent = 2;
    while (!ctx.at_end() && ctx.current().indent == prop_indent) {
        const Line& prop_line = ctx.current();

        // Peek: is this a widget declaration or a property?
        auto [peek_kw, peek_rest] = split_keyword(prop_line.content);
        const bool is_widget_decl = (peek_kw == "Panel"  ||
                                     peek_kw == "Label"  ||
                                     peek_kw == "Button");
        if (is_widget_decl) break;

        auto kv = parse_property(prop_line.content);
        if (!kv) {
            return std::unexpected(make_parse_error(
                prop_line.number, prop_line.indent + 1,
                "Malformed property line"));
        }

        auto [key, val] = *kv;
        if (key == "pos") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'pos' value — expected two integers"));
            }
            pos = metric::Pos<int>{ pair->first, pair->second };
        } else if (key == "size") {
            auto pair = parse_two_ints(val);
            if (!pair) {
                return std::unexpected(make_parse_error(
                    prop_line.number, prop_line.indent + 1,
                    "Invalid 'size' value — expected two integers"));
            }
            sz = metric::Size<uint16_t>{
                static_cast<uint16_t>(pair->first),
                static_cast<uint16_t>(pair->second)
            };
        }
        ++ctx.cursor;
    }

    // Build the Window in-memory (no Win32 call — parser constructs a
    // logical representation; the caller is responsible for creating the
    // actual OS window if needed).
    auto win = std::make_unique<ui::Window>(
        ui::Window::create_from_config(title, pos, sz));

    // Consume top-level Panel children at indent 2
    while (!ctx.at_end() && ctx.current().indent == 2) {
        const Line& child_line = ctx.current();
        auto [child_kw, child_rest] = split_keyword(child_line.content);

        if (child_kw != "Panel") {
            return std::unexpected(make_parse_error(
                child_line.number, child_line.indent + 1,
                std::string("Expected 'Panel' as direct child of Window, got: ")
                    .append(child_kw)));
        }

        auto panel_result = parse_panel(ctx, /*expected_indent=*/2);
        if (!panel_result) return std::unexpected(panel_result.error());

        // panel_result holds a WidgetBase*; we need a Panel*
        auto* raw_panel = dynamic_cast<ui::Panel*>(panel_result->get());
        if (!raw_panel) {
            return std::unexpected(make_parse_error(
                child_line.number, child_line.indent + 1,
                "Internal error: parse_panel returned non-Panel widget"));
        }

        // Transfer ownership into the Window
        std::unique_ptr<ui::Panel> panel_owned(
            static_cast<ui::Panel*>(panel_result->release()));
        win->add_panel(std::move(panel_owned));
    }

    // Any remaining lines at indent 0 would be a second Window — not supported
    if (!ctx.at_end()) {
        const Line& extra = ctx.current();
        return std::unexpected(make_parse_error(
            extra.number, extra.indent + 1,
            "Unexpected content after Window definition"));
    }

    return win;
}

} // anonymous namespace

// ===========================================================================
// WidgetParser public API
// ===========================================================================

std::expected<std::unique_ptr<ui::Window>, ParseError>
WidgetParser::parse(std::string_view input) const
{
    auto lines = tokenize(input);
    ParseContext ctx{ lines, 0 };
    return parse_window(ctx);
}

} // namespace zk::io
