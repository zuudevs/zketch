#pragma once

#include "zk/ui/widget_base.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"

#include <string>
#include <string_view>

namespace zk::ui {

/// A widget that displays a single line of configurable UTF-8 text.
///
/// Satisfies Requirements: 7.1, 7.2, 7.3, 7.5
class Label : public WidgetBase {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    Label(std::string_view text,
          metric::Pos<int>        pos,
          metric::Size<uint16_t>  size);

    // -----------------------------------------------------------------------
    // Render interface (stub — actual Direct2D rendering is task 12)
    // -----------------------------------------------------------------------
    void render(render::Renderer& renderer) override;

    // -----------------------------------------------------------------------
    // Text
    // -----------------------------------------------------------------------
    /// Returns the current text content.
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /// Sets the text and marks the widget dirty for re-render (Req 7.2).
    void set_text(std::string_view text);

    // -----------------------------------------------------------------------
    // Font
    // -----------------------------------------------------------------------
    /// Returns the current font configuration.
    [[nodiscard]] const FontConfig& font() const noexcept { return font_; }

    /// Sets the font configuration and marks the widget dirty (Req 7.3).
    void set_font(const FontConfig& font);

    // -----------------------------------------------------------------------
    // Color
    // -----------------------------------------------------------------------
    /// Returns the current text color.
    [[nodiscard]] Color color() const noexcept { return color_; }

    /// Sets the text color and marks the widget dirty (Req 7.5).
    void set_color(Color color);

private:
    std::string text_;                  ///< UTF-8 text content (Req 7.1)
    FontConfig  font_;                  ///< Font family, size, style (Req 7.3)
    Color       color_{ 0, 0, 0, 255 }; ///< RGBA text color (Req 7.5)
};

} // namespace zk::ui
