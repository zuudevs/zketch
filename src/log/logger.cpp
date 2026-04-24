#include <zk/log/logger.hpp>

#include <chrono>

namespace zk::log {

// ---------------------------------------------------------------------------
// Meyer's singleton — constructed on first call, destroyed at program exit.
// Single-threaded UI per design: no mutex needed.
// ---------------------------------------------------------------------------
Logger& Logger::instance() noexcept {
    static Logger s_instance;
    return s_instance;
}

Logger::Logger() noexcept {
    domain_filter_.fill(true); // all domains enabled by default
}

void Logger::set_min_level(Level level) noexcept {
    min_level_ = level;
}

void Logger::set_domain_filter(Domain domain, bool enabled) noexcept {
    domain_filter_[static_cast<uint8_t>(domain)] = enabled;
}

void Logger::add_sink(std::function<void(const LogEntry&)> sink) {
    sinks_.push_back(std::move(sink));
}

void Logger::reset() noexcept {
    min_level_ = Level::Trace;
    domain_filter_.fill(true);
    sinks_.clear();
}

uint64_t Logger::current_timestamp_ns() noexcept {
    using namespace std::chrono;
    const auto now = steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(duration_cast<nanoseconds>(now).count());
}

} // namespace zk::log
