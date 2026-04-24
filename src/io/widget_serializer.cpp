#include "zk/io/widget_serializer.hpp"

#include "zk/ui/button.hpp"
#include "zk/ui/label.hpp"
#include "zk/ui/panel.hpp"
#include "zk/ui/widget_base.hpp"
#include "zk/ui/window.hpp"

#include <format>
#include <string>
#include <string_view>

namespace zk::io {

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string WidgetSerializer::serialize(const ui::Window& root) const
{
    std::string out;
    out.reserve(512);

    // Window header line
    out += "Window \"";
    out += root.title();
    out += "\"\n";

    // Window properties
    prop(out, 1, "pos",
         std::format("{} {}", root.pos().x, root.pos().y));
    prop(out, 1, "size",
         std::format("{} {}", root.size().w, root.size().h));

    // Serialize each top-level Panel
    for (std::size_t i = 0; i < root.panels().size(); ++i) {
        const auto& panel_ptr = root.panels()[i];
        if (panel_ptr) {
            // Top-level panels: use their own pos() as the relative position
            // (they are direct children of Window, so rel == abs at root level).
            serialize_widget(*panel_ptr, out, /*depth=*/1,
                             panel_ptr->pos());
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void WidgetSerializer::serialize_widget(const ui::WidgetBase& widget,
                                        std::string&          out,
                                        int                   depth,
                                        metric::Pos<int>      rel_pos) const
{
    // Dispatch to concrete type
    if (const auto* panel = dynamic_cast<const ui::Panel*>(&widget)) {
        // For panels, use rel_pos as the panel's position
        indent(out, depth);
        out += "Panel \"panel\"\n";

        prop(out, depth + 1, "pos",
             std::format("{} {}", rel_pos.x, rel_pos.y));
        prop(out, depth + 1, "size",
             std::format("{} {}", panel->size().w, panel->size().h));

        for (std::size_t i = 0; i < panel->children().size(); ++i) {
            const auto& child = panel->children()[i];
            if (child) {
                serialize_widget(*child, out, depth + 1,
                                 panel->child_rel_pos(i));
            }
        }
        return;
    }

    if (const auto* label = dynamic_cast<const ui::Label*>(&widget)) {
        indent(out, depth);
        out += "Label \"label\"\n";

        prop(out, depth + 1, "pos",
             std::format("{} {}", rel_pos.x, rel_pos.y));
        prop(out, depth + 1, "size",
             std::format("{} {}", label->size().w, label->size().h));

        // Escape the text value in double quotes
        out.append(std::string(static_cast<std::size_t>(depth + 1) * 2, ' '));
        out += "text: \"";
        out += label->text();
        out += "\"\n";

        const auto& c = label->color();
        prop(out, depth + 1, "color",
             std::format("{} {} {} {}", c.r, c.g, c.b, c.a));

        const auto& f = label->font();
        prop(out, depth + 1, "font_family", f.family);
        prop(out, depth + 1, "font_size",   std::format("{}", f.size_pt));
        prop(out, depth + 1, "font_bold",   f.bold   ? "true" : "false");
        prop(out, depth + 1, "font_italic", f.italic ? "true" : "false");
        return;
    }

    if (const auto* button = dynamic_cast<const ui::Button*>(&widget)) {
        indent(out, depth);
        out += "Button \"button\"\n";

        prop(out, depth + 1, "pos",
             std::format("{} {}", rel_pos.x, rel_pos.y));
        prop(out, depth + 1, "size",
             std::format("{} {}", button->size().w, button->size().h));

        // Escape the label value in double quotes
        out.append(std::string(static_cast<std::size_t>(depth + 1) * 2, ' '));
        out += "label: \"";
        out += button->label();
        out += "\"\n";
        return;
    }

    // Unknown widget type — emit a comment so the format stays valid
    indent(out, depth);
    out += "# unknown widget\n";
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

void WidgetSerializer::indent(std::string& out, int depth)
{
    out.append(static_cast<std::size_t>(depth) * 2, ' ');
}

void WidgetSerializer::prop(std::string&     out,
                            int              depth,
                            std::string_view key,
                            std::string_view value)
{
    indent(out, depth);
    out += key;
    out += ": ";
    out += value;
    out += '\n';
}

} // namespace zk::io
