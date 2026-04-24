#include "zk/event/event_dispatcher.hpp"

#include <algorithm>

// ---------------------------------------------------------------------------
// Platform-specific thread-id assertion
// ---------------------------------------------------------------------------
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace zk::event::detail {

/// Captures the thread ID of the first call (assumed to be Main_Thread) and
/// asserts that every subsequent call originates from the same thread.
///
/// In release builds the function is a no-op so there is zero overhead on the
/// hot dispatch path.
void assert_main_thread() noexcept {
#if !defined(NDEBUG)
#  ifdef _WIN32
    static const DWORD main_thread_id = ::GetCurrentThreadId();
    if (::GetCurrentThreadId() != main_thread_id) {
        // Terminate with a descriptive message rather than silently corrupting
        // state.  This mirrors the design requirement (Req 11.2) that wrong-
        // thread access triggers an assertion failure in debug mode.
        ::OutputDebugStringA(
            "[zketch] FATAL: EventDispatcher called from non-main thread!\n");
        __debugbreak();
    }
#  endif
#endif
}

} // namespace zk::event::detail

// ---------------------------------------------------------------------------
// EventDispatcher — non-template method implementations
// ---------------------------------------------------------------------------

namespace zk::event {

void EventDispatcher::unsubscribe(EventType type, HandlerId id) noexcept {
    detail::assert_main_thread();

    const auto idx = to_index(type);
    auto& list = handlers_[idx];

    auto it = std::find_if(list.begin(), list.end(),
        [id](const HandlerEntry& entry) { return entry.first == id; });

    if (it != list.end()) {
        list.erase(it);
    }
}

std::size_t EventDispatcher::handler_count(EventType type) const noexcept {
    return handlers_[to_index(type)].size();
}

} // namespace zk::event
