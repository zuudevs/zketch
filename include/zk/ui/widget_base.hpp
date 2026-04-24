#pragma once

#include "zk/metric/pos.hpp"
#include "zk/metric/size.hpp"

// Forward declaration — avoids pulling in the full Renderer header here.
// Renderer is in a higher layer (render → ui in the dependency graph),
// so we use a forward declaration to keep the dependency acyclic.
namespace zk::render {
class Renderer;
} // namespace zk::render

namespace zk::ui {

/// Abstract base class for all zketch widgets.
///
/// Provides the common state shared by every widget:
///   - Position (`pos_`)  — relative to parent, type Pos<int>
///   - Size    (`size_`)  — widget dimensions, type Size<uint16_t>
///   - Visibility flag    (`visible_`)
///   - Dirty flag         (`dirty_`)  — set when the widget needs re-rendering
///
/// The `render()` interface uses a limited virtual dispatch (non-hot-path
/// in the sense that the number of widgets is bounded and the call is not
/// inside a tight inner loop). This enables heterogeneous storage of widget
/// subtypes in `std::vector<std::unique_ptr<WidgetBase>>` inside Panel/Window.
///
/// Satisfies Requirements: 6.1, 19.1, 20.1
class WidgetBase {
public:
    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------
    WidgetBase() noexcept = default;

    explicit WidgetBase(metric::Pos<int> pos, metric::Size<uint16_t> size) noexcept
        : pos_(pos), size_(size) {}

    // Non-copyable — widget ownership is always unique (Req 17.2).
    WidgetBase(const WidgetBase&)            = delete;
    WidgetBase& operator=(const WidgetBase&) = delete;

    // Movable — supports transfer into parent containers.
    WidgetBase(WidgetBase&&) noexcept            = default;
    WidgetBase& operator=(WidgetBase&&) noexcept = default;

    virtual ~WidgetBase() noexcept = default;

    // -----------------------------------------------------------------------
    // Render interface — limited virtual dispatch (non-hot-path per design).
    // Derived classes override this to draw themselves via the Renderer.
    // -----------------------------------------------------------------------
    virtual void render(render::Renderer& renderer) = 0;

    // -----------------------------------------------------------------------
    // Position
    // -----------------------------------------------------------------------
    [[nodiscard]] metric::Pos<int> pos() const noexcept { return pos_; }

    /// Sets the widget's position and marks it dirty for re-render.
    void set_pos(metric::Pos<int> new_pos) noexcept {
        pos_ = new_pos;
        mark_dirty();
    }

    // -----------------------------------------------------------------------
    // Size
    // -----------------------------------------------------------------------
    [[nodiscard]] metric::Size<uint16_t> size() const noexcept { return size_; }

    /// Sets the widget's size and marks it dirty for re-render.
    void set_size(metric::Size<uint16_t> new_size) noexcept {
        size_ = new_size;
        mark_dirty();
    }

    // -----------------------------------------------------------------------
    // Visibility
    // -----------------------------------------------------------------------
    [[nodiscard]] bool is_visible() const noexcept { return visible_; }

    void set_visible(bool visible) noexcept {
        visible_ = visible;
        mark_dirty();
    }

    // -----------------------------------------------------------------------
    // Dirty flag — used by the render loop to skip unchanged widgets.
    // -----------------------------------------------------------------------
    [[nodiscard]] bool is_dirty() const noexcept { return dirty_; }

    /// Marks this widget as needing re-render on the next frame.
    void mark_dirty() noexcept { dirty_ = true; }

    /// Clears the dirty flag after the widget has been rendered.
    void clear_dirty() noexcept { dirty_ = false; }

protected:
    metric::Pos<int>        pos_     {};        ///< Position relative to parent
    metric::Size<uint16_t>  size_    {};        ///< Widget dimensions in pixels
    bool                    visible_ { true };  ///< Whether the widget is rendered
    bool                    dirty_   { true };  ///< Whether a re-render is needed
};

} // namespace zk::ui
