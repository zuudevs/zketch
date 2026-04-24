#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>

// Forward declaration
namespace zk::ui {
class Window;
} // namespace zk::ui

namespace zk::io {

/// Describes a parse failure with the exact source location.
///
/// Satisfies Requirement 12.3
struct ParseError {
    int         line{0};    ///< 1-based line number where the error occurred
    int         column{0};  ///< 1-based column number where the error occurred
    std::string message;    ///< Human-readable description of the error
};

/// Reads a Widget_Config_Format string and reconstructs the widget hierarchy.
///
/// ## Grammar (informal)
/// ```
/// document    ::= window_decl
/// window_decl ::= 'Window' quoted_id NL props children
/// panel_decl  ::= INDENT 'Panel' quoted_id NL props children
/// label_decl  ::= INDENT 'Label' quoted_id NL props
/// button_decl ::= INDENT 'Button' quoted_id NL props
/// props       ::= (INDENT key ':' value NL)*
/// children    ::= (panel_decl | label_decl | button_decl)*
/// ```
///
/// - Indentation is 2 spaces per nesting level.
/// - Lines starting with `#` (after optional leading spaces) are comments and
///   are silently skipped.
/// - Empty lines are silently skipped.
/// - Quoted strings use `"..."` and may contain any character except `"`.
///
/// ## Error handling
/// Returns a `ParseError` with the 1-based line and column of the first
/// malformed token encountered (Req 12.3).
///
/// Satisfies Requirements: 12.2, 12.3
class WidgetParser {
public:
    WidgetParser() noexcept = default;

    /// Parses `input` and returns the reconstructed Window hierarchy, or a
    /// ParseError describing the first syntax problem found.
    [[nodiscard]] std::expected<std::unique_ptr<ui::Window>, ParseError>
    parse(std::string_view input) const;
};

} // namespace zk::io
