#pragma once

#include "zk/metric/pos.hpp"
#include <string>

// Forward declarations — avoids pulling in full widget headers.
namespace zk::ui {
class Window;
class Panel;
class WidgetBase;
} // namespace zk::ui

namespace zk::io {

/// Converts a widget hierarchy rooted at a Window into a human-readable
/// Widget_Config_Format string.
///
/// ## Format rules
///   - Each widget opens with `TypeName "id"` on its own line.
///   - Properties follow as `key: value` lines, indented 2 spaces deeper than
///     the widget declaration.
///   - Child widgets are indented 2 spaces deeper than their parent.
///   - Comments start with `#` (not emitted by the serializer, but respected
///     by the parser).
///
/// ## Example output
/// ```
/// Window "main"
///   pos: 100 100
///   size: 800 600
///   Panel "sidebar"
///     pos: 0 0
///     size: 200 600
///     Label "title"
///       pos: 10 10
///       size: 180 30
///       text: "Hello, zketch!"
///       color: 0 0 0 255
///     Button "ok_btn"
///       pos: 10 50
///       size: 80 30
///       label: "OK"
/// ```
///
/// Satisfies Requirements: 12.1, 12.4
class WidgetSerializer {
public:
    WidgetSerializer() noexcept = default;

    /// Serializes the entire widget hierarchy rooted at `root` into a
    /// Widget_Config_Format string.
    ///
    /// The returned string always ends with a newline character.
    [[nodiscard]] std::string serialize(const ui::Window& root) const;

private:
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// Appends the serialized form of a generic WidgetBase child to `out`.
    /// Dispatches to the concrete type (Panel, Label, Button) via dynamic_cast.
    /// `rel_pos` overrides the position stored in the widget (which may be
    /// the absolute screen coordinate after Panel::add_child transforms it).
    void serialize_widget(const ui::WidgetBase& widget,
                          std::string&          out,
                          int                   depth,
                          metric::Pos<int>      rel_pos) const;

    /// Appends `depth * 2` spaces to `out`.
    static void indent(std::string& out, int depth);

    /// Appends a `key: value` property line at the given depth.
    static void prop(std::string& out, int depth,
                     std::string_view key, std::string_view value);
};

} // namespace zk::io
