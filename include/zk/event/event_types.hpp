#pragma once

#include "zk/metric/size.hpp"
#include <cstdint>
#include <string_view>

namespace zk::event {

/// All event types dispatched by the framework in v1.0.
/// Backed by uint32_t for stable numeric identity (Req 18.2, 13.1).
enum class EventType : uint32_t {
    Click,
    Resize,
    Close,
    KeyDown,
    KeyUp,
    MouseEnter,
    MouseLeave,

    _Count  // sentinel — must remain last; used to size the handler array
};

// ---------------------------------------------------------------------------
// Payload types — one struct per event that carries data (Req 9.3, 9.4, 9.5)
// ---------------------------------------------------------------------------

/// Payload for EventType::Click — no additional data.
struct ClickPayload {};

/// Payload for EventType::Resize — carries the new window size.
struct ResizePayload {
    metric::Size<uint16_t> size;
};

/// Payload for EventType::KeyDown and EventType::KeyUp — carries the Win32
/// virtual key code.
struct KeyPayload {
    uint32_t vkey;
};

/// Payload for EventType::Close — no additional data.
struct ClosePayload {};

/// Payload for EventType::MouseEnter — no additional data.
struct MouseEnterPayload {};

/// Payload for EventType::MouseLeave — no additional data.
struct MouseLeavePayload {};

} // namespace zk::event

namespace zk::util {

/// Returns a human-readable string_view for an EventType value.
/// Satisfies Requirement 13.6.
[[nodiscard]] constexpr std::string_view to_string(event::EventType type) noexcept {
    using event::EventType;
    switch (type) {
        case EventType::Click:       return "Click";
        case EventType::Resize:      return "Resize";
        case EventType::Close:       return "Close";
        case EventType::KeyDown:     return "KeyDown";
        case EventType::KeyUp:       return "KeyUp";
        case EventType::MouseEnter:  return "MouseEnter";
        case EventType::MouseLeave:  return "MouseLeave";
        default:                     return "Unknown";
    }
}

} // namespace zk::util
