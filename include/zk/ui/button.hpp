#pragma once

#include "zk/ui/widget_base.hpp"
#include "zk/ui/color.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace zk::ui {

/// Visual/interaction state of a Button widget.
/// Satisfies Requirements: 8.3, 8.4, 8.5, 8.7
enum class ButtonState : uint8_t {
    Normal,
    Hovered,
    Disabled,
};

/// A clickable button widget with configurable label and click handler.
///
/// State transitions:
///   - on_mouse_enter() : Normal  → Hovered
///   - on_mouse_leave() : Hovered → Normal
///   - on_click()       : calls handler if state != Disabled
///   - set_disabled(true)  : any state → Disabled
///   - set_disabled(false) : Disabled  → Normal
///
/// Satisfies Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6, 8.7
class Button : public WidgetBase {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    Button(std::string_view label,
           metric::Pos<int>        pos,
           metric::Size<uint16_t>  size);

    // -----------------------------------------------------------------------
    // Render interface (stub — actual Direct2D rendering is task 12)
    // -----------------------------------------------------------------------
    void render(render::Renderer& renderer) override;

    // -----------------------------------------------------------------------
    // Label
    // -----------------------------------------------------------------------
    /// Returns the button's label text.
    [[nodiscard]] const std::string& label() const noexcept { return label_; }

    // -----------------------------------------------------------------------
    // Click handler — Req 8.2, 8.6
    // -----------------------------------------------------------------------
    /// Registers a callback invoked when the button is clicked while not Disabled.
    /// Passing an empty function clears the handler.
    void set_on_click(std::function<void()> handler);

    // -----------------------------------------------------------------------
    // Disabled state — Req 8.7
    // -----------------------------------------------------------------------
    /// Enables or disables the button.
    /// set_disabled(true)  → state becomes Disabled
    /// set_disabled(false) → state becomes Normal
    void set_disabled(bool disabled);

    // -----------------------------------------------------------------------
    // State query — Req 8.3, 8.4, 8.5, 8.7
    // -----------------------------------------------------------------------
    /// Returns the current ButtonState.
    [[nodiscard]] ButtonState state() const noexcept { return state_; }

    // -----------------------------------------------------------------------
    // Internal event handlers — called by Window's WndProc
    // -----------------------------------------------------------------------
    /// Called when the mouse cursor enters the button area (Req 8.3).
    void on_mouse_enter();

    /// Called when the mouse cursor leaves the button area (Req 8.4).
    void on_mouse_leave();

    /// Called when a left-click is completed over the button area (Req 8.2, 8.6, 8.7).
    void on_click();

private:
    std::string            label_;                          ///< Button face text (Req 8.1)
    ButtonState            state_{ ButtonState::Normal };  ///< Current visual/interaction state
    std::function<void()>  on_click_;                      ///< Optional click handler (Req 8.2)
};

} // namespace zk::ui

namespace zk::util {

/// Returns a human-readable string_view for a ButtonState value.
/// Satisfies Requirement 13.6.
[[nodiscard]] constexpr std::string_view to_string(ui::ButtonState state) noexcept {
    using ui::ButtonState;
    switch (state) {
        case ButtonState::Normal:   return "Normal";
        case ButtonState::Hovered:  return "Hovered";
        case ButtonState::Disabled: return "Disabled";
        default:                    return "Unknown";
    }
}

} // namespace zk::util
