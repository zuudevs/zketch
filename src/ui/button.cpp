#include "zk/ui/button.hpp"
#include "zk/render/renderer.hpp"
#include "zk/ui/color.hpp"
#include "zk/ui/font_config.hpp"

namespace zk::ui {

// Color palette per state
namespace {
    constexpr Color kBgNormal   { 70,  130, 180, 255 }; // steel blue
    constexpr Color kBgHovered  { 100, 160, 210, 255 }; // lighter blue
    constexpr Color kBgDisabled { 160, 160, 160, 255 }; // gray
    constexpr Color kTextColor  { 255, 255, 255, 255 }; // white
} // namespace

Button::Button(std::string_view label,
               metric::Pos<int>        pos,
               metric::Size<uint16_t>  size)
    : WidgetBase(pos, size)
    , label_(label)
{}

void Button::render(render::Renderer& renderer)
{
    if (!is_visible()) return;

    // Background color based on state
    Color bg = kBgNormal;
    if (state_ == ButtonState::Hovered)  bg = kBgHovered;
    if (state_ == ButtonState::Disabled) bg = kBgDisabled;

    renderer.draw_rect(pos_, size_, bg);

    if (!label_.empty()) {
        FontConfig font{ "Segoe UI", 12.0f, false, false };
        renderer.draw_text(label_, pos_, size_, font, kTextColor);
    }

    clear_dirty();
}

void Button::set_on_click(std::function<void()> handler)
{
    on_click_ = std::move(handler);
}

void Button::set_disabled(bool disabled)
{
    if (disabled) {
        state_ = ButtonState::Disabled;
    } else {
        state_ = ButtonState::Normal;
    }
    mark_dirty();
}

void Button::on_mouse_enter()
{
    // Only transition to Hovered from Normal; Disabled stays Disabled (Req 8.3, 8.7).
    if (state_ == ButtonState::Normal) {
        state_ = ButtonState::Hovered;
        mark_dirty();
    }
}

void Button::on_mouse_leave()
{
    // Return to Normal from Hovered; Disabled stays Disabled (Req 8.4, 8.7).
    if (state_ == ButtonState::Hovered) {
        state_ = ButtonState::Normal;
        mark_dirty();
    }
}

void Button::on_click()
{
    // Fire handler only when not Disabled; missing handler is silently ignored (Req 8.2, 8.6, 8.7).
    if (state_ != ButtonState::Disabled && on_click_) {
        on_click_();
    }
}

} // namespace zk::ui
