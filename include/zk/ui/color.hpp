#pragma once

#include <cstdint>

namespace zk::ui {

/// RGBA color value — each channel is an 8-bit unsigned integer [0, 255].
/// Satisfies Requirement 7.5: configurable RGBA 32-bit text color.
struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    constexpr bool operator==(const Color&) const noexcept = default;
};

// ---------------------------------------------------------------------------
// Common color constants
// ---------------------------------------------------------------------------
namespace colors {

inline constexpr Color Black       { 0,   0,   0,   255 };
inline constexpr Color White       { 255, 255, 255, 255 };
inline constexpr Color Red         { 255, 0,   0,   255 };
inline constexpr Color Green       { 0,   255, 0,   255 };
inline constexpr Color Blue        { 0,   0,   255, 255 };
inline constexpr Color Yellow      { 255, 255, 0,   255 };
inline constexpr Color Cyan        { 0,   255, 255, 255 };
inline constexpr Color Magenta     { 255, 0,   255, 255 };
inline constexpr Color Transparent { 0,   0,   0,   0   };
inline constexpr Color Gray        { 128, 128, 128, 255 };
inline constexpr Color LightGray   { 211, 211, 211, 255 };
inline constexpr Color DarkGray    { 64,  64,  64,  255 };

} // namespace colors

} // namespace zk::ui
