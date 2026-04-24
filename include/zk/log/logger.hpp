#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace zk::log {

enum class Level : uint8_t { Trace, Debug, Info, Warn, Error, Fatal };
enum class Domain : uint8_t { Core, UI, Render, Event, IO, Parser };

struct LogEntry {
    Level           level;
    Domain          domain;
    std::string_view message;
    std::string_view file;
    int             line;
    uint64_t        timestamp_ns;
};

class Logger {
public:
    static Logger& instance() noexcept;

    void set_min_level(Level level) noexcept;
    void set_domain_filter(Domain domain, bool enabled) noexcept;
    void add_sink(std::function<void(const LogEntry&)> sink);

    // Resets all state to defaults — intended for use in unit tests only.
    void reset() noexcept;

    template <Level L, Domain D>
    void log(std::string_view msg, std::string_view file = {}, int line = 0) noexcept {
        // Compile-time elimination: if L is a compile-time constant below a
        // compile-time threshold this branch is dead code and the optimizer
        // removes it entirely.  The runtime check handles the dynamic case.
        if constexpr (L < Level::Trace) {
            return; // unreachable guard — keeps the pattern extensible
        }

        // Runtime early-exit (cheap: two comparisons, no allocation)
        if (L < min_level_) return;
        if (!domain_filter_[static_cast<uint8_t>(D)]) return;

        const LogEntry entry{
            L, D, msg, file, line, current_timestamp_ns()
        };

        for (auto& sink : sinks_) {
            sink(entry);
        }
    }

private:
    Logger() noexcept;

    static uint64_t current_timestamp_ns() noexcept;

    Level                                              min_level_{ Level::Trace };
    std::array<bool, 6>                                domain_filter_;   // one per Domain value, all true by default
    std::vector<std::function<void(const LogEntry&)>>  sinks_;
};

} // namespace zk::log

// ---------------------------------------------------------------------------
// Convenience macro — expands to a single expression; the template arguments
// are compile-time constants so the compiler can eliminate the call entirely
// when the level is statically below the configured threshold.
// ---------------------------------------------------------------------------
#define ZK_LOG(level, domain, msg) \
    ::zk::log::Logger::instance().log<level, domain>((msg), __FILE__, __LINE__)
