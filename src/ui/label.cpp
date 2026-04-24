#include "zk/ui/label.hpp"
#include "zk/render/renderer.hpp"

namespace zk::ui {

Label::Label(std::string_view text,
             metric::Pos<int>        pos,
             metric::Size<uint16_t>  size)
    : WidgetBase(pos, size)
    , text_(text)
{}

void Label::render(render::Renderer& renderer)
{
    if (!is_visible() || text_.empty()) return;
    renderer.draw_text(text_, pos_, size_, font_, color_);
    clear_dirty();
}

void Label::set_text(std::string_view text)
{
    text_ = text;
    mark_dirty(); // Req 7.2 — schedule re-render on next frame
}

void Label::set_font(const FontConfig& font)
{
    font_ = font;
    mark_dirty();
}

void Label::set_color(Color color)
{
    color_ = color;
    mark_dirty();
}

} // namespace zk::ui
