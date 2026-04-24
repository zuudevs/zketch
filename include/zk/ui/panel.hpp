#pragma once

#include "zk/ui/widget_base.hpp"
#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

#include <memory>
#include <vector>

namespace zk::ui {

/// A rectangular container widget that owns and lays out child widgets.
///
/// ## Position model
///
/// Each child is added with a *relative* position (relative to this Panel's
/// top-left corner).  Panel maintains its own `abs_pos_` — the absolute screen
/// coordinate of its top-left corner — and derives each child's absolute
/// screen position as:
///
///     child_abs = clamp(abs_pos_ + child_rel_pos, min=0)
///
/// `WidgetBase::pos_` on a Panel stores the *relative* position (relative to
/// the Panel's own parent).  `abs_pos_` is the derived screen coordinate.
///
/// When the Panel is moved via `set_pos()`, or when a parent Panel calls
/// `set_abs_origin()`, the absolute positions of all children are recomputed
/// recursively so every widget's screen coordinate is always current.
///
/// ## Ownership (Req 17.5, 17.6)
///   - `add_child()` transfers unique ownership into the Panel's children list.
///   - `remove_child()` releases ownership back to the caller.
///
/// Satisfies Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6, 17.5, 17.6
class Panel : public WidgetBase {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /// Constructs a Panel with the given relative position and size.
    /// `abs_pos_` is initialised equal to `pos` (root-level assumption:
    /// the Panel is a direct child of Window until reparented by a parent Panel).
    Panel(metric::Pos<int>       pos,
          metric::Size<uint16_t> size) noexcept;

    // -----------------------------------------------------------------------
    // Render interface (stub — actual Direct2D rendering is task 12)
    // -----------------------------------------------------------------------
    void render(render::Renderer& renderer) override;

    // -----------------------------------------------------------------------
    // Position
    // -----------------------------------------------------------------------

    /// Sets the Panel's *relative* position (relative to its parent) and
    /// recomputes the absolute position of all children recursively (Req 6.4).
    ///
    /// Note: if this Panel is a child of another Panel, the parent Panel will
    /// call `set_abs_origin()` to supply the correct absolute origin; this
    /// method is the public API for direct callers.
    void set_pos(metric::Pos<int> new_pos) noexcept;

    /// Returns the Panel's absolute screen position.
    [[nodiscard]] metric::Pos<int> abs_pos() const noexcept { return abs_pos_; }

    /// Called by a parent Panel to propagate a new absolute origin.
    /// Updates `abs_pos_` from `parent_abs + pos_` and recurses into children.
    void set_abs_origin(metric::Pos<int> parent_abs_pos) noexcept;

    // -----------------------------------------------------------------------
    // Child management — Req 6.2, 6.3, 6.6, 17.5, 17.6
    // -----------------------------------------------------------------------

    /// Transfers ownership of `child` into this Panel.
    /// The child's `pos()` is treated as its *relative* position; its absolute
    /// screen position is computed immediately (Req 6.3).
    void add_child(std::unique_ptr<WidgetBase> child);

    /// Releases ownership of `child` back to the caller.
    /// Returns `nullptr` if `child` is not found in the children list (Req 6.6).
    [[nodiscard]] std::unique_ptr<WidgetBase> remove_child(WidgetBase* child);

    // -----------------------------------------------------------------------
    // Children access (read-only)
    // -----------------------------------------------------------------------
    [[nodiscard]] const std::vector<std::unique_ptr<WidgetBase>>& children() const noexcept {
        return children_;
    }

    /// Returns the relative position of the child at `index`.
    /// The relative position is the original position passed to add_child(),
    /// before the panel computes the absolute screen coordinate.
    /// Used by WidgetSerializer to produce round-trip-stable output.
    [[nodiscard]] metric::Pos<int> child_rel_pos(std::size_t index) const noexcept {
        if (index < child_rel_positions_.size()) {
            return child_rel_positions_[index];
        }
        return {};
    }

private:
    metric::Pos<int> abs_pos_{};  ///< Absolute screen position (Req 6.3, 6.5)

    /// Relative positions of each child, parallel to `children_`.
    /// Stored separately so that `update_children_abs_pos()` can always
    /// recompute from the original relative value without loss of information.
    std::vector<metric::Pos<int>> child_rel_positions_;

    std::vector<std::unique_ptr<WidgetBase>> children_;  ///< Owned child widgets

    /// Recomputes the absolute position of every direct child and recurses
    /// into any child that is itself a Panel (Req 6.4).
    void update_children_abs_pos() noexcept;

    /// Computes the clamped absolute screen coordinate for a child.
    /// Screen coordinates are clamped to a minimum of 0 (Req 6.5).
    [[nodiscard]] static metric::Pos<int>
    compute_child_abs_pos(metric::Pos<int> parent_abs,
                          metric::Pos<int> child_rel) noexcept;
};

} // namespace zk::ui
