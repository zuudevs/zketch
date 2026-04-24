#pragma once

#include <string_view>
#include <type_traits>

#include <zk/error/error.hpp>
#include <zk/log/logger.hpp>

namespace zk::util {

// ---------------------------------------------------------------------------
// to_underlying — Explicit conversion from enum class to underlying type
// ---------------------------------------------------------------------------

/// Converts any enum class to its underlying numeric type.
/// Satisfies Requirement 13.4: explicit conversion via a well-defined helper.
///
/// Usage:
///   auto n = zk::util::to_underlying(ErrorCode::InitFailed); // uint32_t
template <typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

// ---------------------------------------------------------------------------
// to_string — String conversion for leaf-layer enum classes
// (error and log layers are below util in the dependency graph)
// ---------------------------------------------------------------------------

/// Returns a human-readable string_view for an ErrorCode value.
/// Satisfies Requirement 13.6.
[[nodiscard]] constexpr std::string_view to_string(error::ErrorCode code) noexcept {
    using error::ErrorCode;
    switch (code) {
        case ErrorCode::InitFailed:                    return "InitFailed";
        case ErrorCode::WindowClassRegistrationFailed: return "WindowClassRegistrationFailed";
        case ErrorCode::WindowCreationFailed:          return "WindowCreationFailed";
        case ErrorCode::WindowInvalidState:            return "WindowInvalidState";
        case ErrorCode::RenderTargetCreationFailed:    return "RenderTargetCreationFailed";
        case ErrorCode::RenderTargetLost:              return "RenderTargetLost";
        case ErrorCode::EventHandlerException:         return "EventHandlerException";
        case ErrorCode::ParseError:                    return "ParseError";
        case ErrorCode::WrongThread:                   return "WrongThread";
        case ErrorCode::Unknown:                       return "Unknown";
        default:                                       return "Unknown";
    }
}

/// Returns a human-readable string_view for a log Level value.
/// Satisfies Requirement 13.6.
[[nodiscard]] constexpr std::string_view to_string(log::Level level) noexcept {
    using log::Level;
    switch (level) {
        case Level::Trace: return "Trace";
        case Level::Debug: return "Debug";
        case Level::Info:  return "Info";
        case Level::Warn:  return "Warn";
        case Level::Error: return "Error";
        case Level::Fatal: return "Fatal";
        default:           return "Unknown";
    }
}

/// Returns a human-readable string_view for a log Domain value.
/// Satisfies Requirement 13.6.
[[nodiscard]] constexpr std::string_view to_string(log::Domain domain) noexcept {
    using log::Domain;
    switch (domain) {
        case Domain::Core:   return "Core";
        case Domain::UI:     return "UI";
        case Domain::Render: return "Render";
        case Domain::Event:  return "Event";
        case Domain::IO:     return "IO";
        case Domain::Parser: return "Parser";
        default:             return "Unknown";
    }
}

} // namespace zk::util

// ---------------------------------------------------------------------------
// Note on PumpMode, ButtonState, EventType
// ---------------------------------------------------------------------------
// These enums are defined in higher-level modules (core, ui, event) which
// depend on util — including them here would create a cyclic dependency.
// Their to_string overloads are provided in their respective headers:
//   - zk::core::PumpMode     → include/zk/core/application.hpp
//   - zk::ui::ButtonState    → include/zk/ui/button.hpp
//   - zk::event::EventType   → include/zk/event/event_types.hpp
// Each of those headers declares its own to_string in the zk::util namespace
// via an inline overload, keeping the dependency graph acyclic.
