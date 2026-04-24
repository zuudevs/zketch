#include "zk/ui/panel.hpp"

#include <algorithm>  // std::find_if
#include <utility>    // std::move

namespace zk::ui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Panel::Panel(metric::Pos<int>       pos,
             metric::Size<uint16_t> size) noexcept
    : WidgetBase(pos, size)
    , abs_pos_(pos)   // root-level assumption: abs == rel until reparented
{}

// ---------------------------------------------------------------------------
// Render (stub)
// ---------------------------------------------------------------------------

void Panel::render(render::Renderer& renderer)
{
    if (!is_visible()) return;

    // Render all visible children
    for (auto& child : children_) {
        if (child && child->is_visible()) {
            child->render(renderer);
        }
    }
    clear_dirty();
}

// ---------------------------------------------------------------------------
// Position management
// ---------------------------------------------------------------------------

void Panel::set_pos(metric::Pos<int> new_pos) noexcept
{
    // Update the relative position in WidgetBase::pos_ (also marks dirty).
    WidgetBase::set_pos(new_pos);

    // When called directly (not via set_abs_origin), treat the new relative
    // position as the new absolute position.  A parent Panel will correct
    // abs_pos_ via set_abs_origin() if this Panel is nested.
    abs_pos_ = new_pos;

    update_children_abs_pos();
}

void Panel::set_abs_origin(metric::Pos<int> parent_abs_pos) noexcept
{
    // Derive our absolute position from the parent's absolute position and
    // our own relative position (pos_).
    abs_pos_ = compute_child_abs_pos(parent_abs_pos, pos_);
    update_children_abs_pos();
    mark_dirty();
}

// ---------------------------------------------------------------------------
// Child management
// ---------------------------------------------------------------------------

void Panel::add_child(std::unique_ptr<WidgetBase> child)
{
    if (!child) return;

    // Capture the child's relative position before we store it.
    metric::Pos<int> rel_pos = child->pos();

    // Compute the child's initial absolute screen position (Req 6.3).
    metric::Pos<int> child_abs = compute_child_abs_pos(abs_pos_, rel_pos);

    if (auto* sub_panel = dynamic_cast<Panel*>(child.get())) {
        // For nested Panels, use set_abs_origin so the sub-panel can
        // propagate the absolute origin to its own children (Req 6.4).
        sub_panel->set_abs_origin(abs_pos_);
    } else {
        // For leaf widgets, set_pos stores the absolute coordinate so the
        // renderer can use it directly without a separate layout pass.
        child->set_pos(child_abs);
    }

    child_rel_positions_.push_back(rel_pos);
    children_.push_back(std::move(child));
    mark_dirty();
}

std::unique_ptr<WidgetBase> Panel::remove_child(WidgetBase* child)
{
    auto it = std::find_if(children_.begin(), children_.end(),
                           [child](const std::unique_ptr<WidgetBase>& ptr) {
                               return ptr.get() == child;
                           });

    if (it == children_.end()) {
        return nullptr;  // not found — caller gets nullptr (Req 6.6)
    }

    // Remove the parallel relative-position entry.
    const auto idx = static_cast<std::size_t>(it - children_.begin());
    child_rel_positions_.erase(child_rel_positions_.begin() +
                               static_cast<std::ptrdiff_t>(idx));

    std::unique_ptr<WidgetBase> released = std::move(*it);
    children_.erase(it);
    mark_dirty();
    return released;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Panel::update_children_abs_pos() noexcept
{
    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        if (!child) continue;

        const metric::Pos<int>& rel = child_rel_positions_[i];
        metric::Pos<int> child_abs  = compute_child_abs_pos(abs_pos_, rel);

        if (auto* sub_panel = dynamic_cast<Panel*>(child.get())) {
            // Recurse: let the sub-panel recompute its own abs_pos_ and
            // propagate further down the tree (Req 6.4).
            sub_panel->set_abs_origin(abs_pos_);
        } else {
            // Leaf widget: update its stored position to the new absolute value.
            child->set_pos(child_abs);
        }
    }
}

metric::Pos<int>
Panel::compute_child_abs_pos(metric::Pos<int> parent_abs,
                              metric::Pos<int> child_rel) noexcept
{
    // Sum the parent's absolute position and the child's relative position,
    // then apply the screen-coordinate floor of 0 (Req 6.5).
    // We use plain int arithmetic to avoid Pos<int> clamping interfering with
    // the intermediate sum before we apply the floor.
    const int raw_x = static_cast<int>(parent_abs.x) + static_cast<int>(child_rel.x);
    const int raw_y = static_cast<int>(parent_abs.y) + static_cast<int>(child_rel.y);

    return metric::Pos<int>{ raw_x < 0 ? 0 : raw_x,
                             raw_y < 0 ? 0 : raw_y };
}

} // namespace zk::ui
