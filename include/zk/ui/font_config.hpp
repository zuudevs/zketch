#pragma once

#include <string>

namespace zk::ui {

/// Font configuration for text-rendering widgets (Label, Button).
/// Satisfies Requirement 7.3: configurable font and size via font property.
struct FontConfig {
    std::string family   = "Segoe UI";  ///< Font family name (UTF-8)
    float       size_pt  = 12.0f;       ///< Font size in points
    bool        bold     = false;       ///< Bold weight
    bool        italic   = false;       ///< Italic style

    constexpr bool operator==(const FontConfig&) const noexcept = default;
};

} // namespace zk::ui
